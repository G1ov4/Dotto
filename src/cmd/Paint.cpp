// Copyright (c) 2021 LibreSprite Authors (cf. AUTHORS.md)
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include <cmd/Command.hpp>
#include <common/Messages.hpp>
#include <common/PubSub.hpp>
#include <doc/Cell.hpp>
#include <doc/Selection.hpp>
#include <log/Log.hpp>
#include <tools/Tool.hpp>

static std::shared_ptr<Surface> backup;
static Surface::PixelType* backupSurface = nullptr;
static std::shared_ptr<Selection> backupSelection;

class Paint : public Command {
    Property<std::shared_ptr<Selection>> selection{this, "selection"};
    Property<Color> color{this, "color", Tool::color.toString()};
    Property<bool> preview{this, "preview", false};
    inject<Cell> cell{"activecell"};
    Vector<U32> undoData;

public:
    void setupPreview() {
        auto surface = cell->getComposite();
        if (!backup) {
            backup = std::make_shared<Surface>();
            backupSelection = inject<Selection>{"new"};
        }

        backup->resize(surface->width(), surface->height());
        auto surfaceData = surface->data();
        if (surfaceData == backupSurface)
            return;

        backupSelection->clear();
        backupSurface = surfaceData;
        auto backupData = backup->data();
        for (std::size_t i = 0, size = surface->width() * surface->height(); i < size; ++i) {
            backupData[i] = surfaceData[i];
        }
    }

    void run() override {
        auto selection = this->selection->get();
        auto& color = *this->color;
        if (!selection || !color.a) {
            return;
        }

        auto maskRect = selection->getBounds();
        if (maskRect.empty())
            return;

        auto surface = cell->getComposite();
        auto writeData = surface->data();
        auto readData = writeData;

        if (!preview) {
            if (backupSurface == readData) {
                readData = backup->data();
                selection = backupSelection.get();
                std::swap(*this->selection, backupSelection);
            }
            backupSurface = nullptr;
        } else {
            setupPreview();
        }

        F32 alpha = color.a / 255.0f / 255.0f;
        undoData = selection->read(surface);
        auto& mask = selection->getData();
        auto commonRect = maskRect;
        auto maskStride = maskRect.width;
        auto surfaceStride = surface->width();
        Color tmp;
        commonRect.intersect({0, 0, surface->width(), surface->height()});

        U32 maskOffsetY = commonRect.y > 0 ? 0 : -commonRect.y;
        U32 maskOffsetX = commonRect.x > 0 ? 0 : -commonRect.x;
        U32 surfaceOffsetY = commonRect.y > 0 ? commonRect.y : 0;
        U32 surfaceOffsetX = commonRect.x > 0 ? commonRect.x : 0;

        for (U32 y = 0; y < commonRect.height; ++y) {
            U32 maskIndex = (y + maskOffsetY) * maskStride + maskOffsetX;
            U32 surfaceIndex = (y + surfaceOffsetY) * surfaceStride + surfaceOffsetX;
            for (U32 x = 0; x < commonRect.width; ++x, ++maskIndex, ++surfaceIndex) {
                auto amount = mask[maskIndex] * alpha;
                tmp.fromU32(readData[surfaceIndex]);
                tmp.r = tmp.r * (1 - amount) + color.r * amount;
                tmp.g = tmp.g * (1 - amount) + color.g * amount;
                tmp.b = tmp.b * (1 - amount) + color.b * amount;
                tmp.a = tmp.a + amount * (255 - tmp.a);
                writeData[surfaceIndex] = tmp.toU32();
            }
        }

        surface->setDirty();
        if (preview)
            this->selection->get()->clear();
    }
};

static Command::Shared<Paint> cmd{"paint"};