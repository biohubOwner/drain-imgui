#pragma once
#include <thread>
#include <vector>
#include <functional>
#include <string>
#include <mutex>
#include <iostream>
#include "../output/output.h"

namespace manager {

    class thread_manager {
    public:
        static void create(std::function<void()> func, const std::string& name = "unknown") {
            std::thread([func, name]() {
                logger->log<e_level::DEBUG>("Thread '{}' started", name);
                try {
                    func();
                }
                catch (const std::exception& e) {
                    logger->log<e_level::ERORR>("Thread '{}' crashed with exception: {}", name, e.what());
                }
                catch (...) {
                    logger->log<e_level::ERORR>("Thread '{}' crashed with unknown exception", name);
                }
                logger->log<e_level::DEBUG>("Thread '{}' finished", name);
            }).detach();
        }
    };

    class crash_manager {
    public:
        static void init();
        static long __stdcall exception_handler(struct _EXCEPTION_POINTERS* exceptionInfo);
    };

}

