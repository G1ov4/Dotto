// Copyright (c) 2021 LibreSprite Authors (cf. AUTHORS.md)
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include <app/App.hpp>
#include <common/PropertySet.hpp>
#include <common/System.hpp>
#include <fs/FileSystem.hpp>
#include <log/Log.hpp>

class AppImpl : public App {
public:
    bool running = true;
    inject<Log> log;
    inject<System> system;
    inject<FileSystem> fs{"new"};

    PropertySet appSettings;

    void boot(int argc, const char* argv[]) override {
        bootLogger();
        bootFS();
        system->boot();
    }

    bool run() override {
        if (!system->run())
            return false;
        return running;
    }

    void bootLogger() {
        log->setGlobal();
        log->setLevel(Log::Level::VERBOSE); // TODO: Configure level using args
    }

    void bootFS() {
        auto file = fs->open("%userdata/LICENSE.txt", {.write=false});
        if (!file->isOpen()) file = fs->open("%appdir/LICENSE.txt", {.write=false});
        if (file->isOpen()) {
            Log::write(Log::Level::VERBOSE, file->readTextFile());
        }
    }
};

static App::Shared<AppImpl> app{"dotto"};
