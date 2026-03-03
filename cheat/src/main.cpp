#include <thread>
#include <iostream>

#include <memory/memory.h>
#include <output/output.h>
#include <globals.h>
#include <render/render.h>
#include <features/sdk/offsets/offsets.h>
#include <features/cache/cache.h>
#include <features/cheats/misc/misc.h>
#include <features/settings/config.h>
#include "features/initalize/initalize.h"
#include "manager/manager.h"

auto main(int argc, char* argv[], char* envp[]) -> int
{
	SetConsoleTitleA("Ud.gg");
    manager::crash_manager::init();

    if (!initialize::runservice())
    {
        logger->log<e_level::ERORR>("Initialization failed. Press any key to exit...");
        std::cin.get();
        return 0;
    }

    manager::thread_manager::create(cache::run, "cache_thread");
    cheats::misc::start_threads();

    config::init();
    config::load_auto_config();

    if (!render->create_window())
    {
        logger->log<e_level::ERORR>("failed to create window");
        globals::running = false;
        std::cin.get();
        return 0;
    }

    if (!render->create_device())
    {
        logger->log<e_level::ERORR>("failed to create device");
        globals::running = false;
        std::cin.get();
        return 0;
    }

    if (!render->create_imgui())
    {
        logger->log<e_level::ERORR>("failed to create imgui");
        globals::running = false;
        std::cin.get();
        return 0;
    }


    while (globals::running)
    {
        render->start_render();

        render->render_visuals();

        render->render_menu();

        render->end_render();
    }

    globals::running = false;

    return 0;
}

