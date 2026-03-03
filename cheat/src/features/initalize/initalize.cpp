#include "initalize.h"
#include <iostream>
#include <memory/memory.h>
#include "features/sdk/offsets/offsets.h"
#include "output/output.h"
#include "globals.h"
#include "features/cache/cache.h"

struct handle_data {
    unsigned long process_id;
    HWND window_handle;
};

BOOL CALLBACK enum_windows_callback(HWND handle, LPARAM lParam) {
    handle_data& data = *(handle_data*)lParam;
    unsigned long process_id = 0;
    GetWindowThreadProcessId(handle, &process_id);
    if (data.process_id != process_id || !(GetWindow(handle, GW_OWNER) == (HWND)0 && IsWindowVisible(handle)))
        return TRUE;
    data.window_handle = handle;
    return FALSE;
}

HWND find_main_window(unsigned long process_id) {
    handle_data data;
    data.process_id = process_id;
    data.window_handle = 0;
    EnumWindows(enum_windows_callback, (LPARAM)&data);
    return data.window_handle;
}

auto initialize::runservice() -> bool
{
    if (!driver->initialize())
    {
        logger->log<e_level::ERORR>("Failed to initialize stealth interface.");
        return false;
    }
    std::vector<unsigned char> enc_beta = { 0x08, 0x35, 0x38, 0x36, 0x35, 0x22, 0x0a, 0x36, 0x3b, 0x23, 0x3f, 0x28, 0x18, 0x3f, 0x2e, 0x3b, 0x74, 0x3f, 0x22, 0x3f };
    std::vector<unsigned char> enc_player = { 0x08, 0x35, 0x38, 0x36, 0x35, 0x22, 0x0a, 0x36, 0x3b, 0x23, 0x3f, 0x28, 0x74, 0x3f, 0x22, 0x3f };

    std::string process_name = driver->decrypt_string(enc_beta, 0x5A);
    if (!memory->find_process_id(process_name))
    {
        process_name = driver->decrypt_string(enc_player, 0x5A);
        if (!memory->find_process_id(process_name))
        {
            logger->log<e_level::ERORR>("Game process not found.");
            return false;
        }
    }

    if (!memory->attach_to_process(process_name))
    {
        logger->log<e_level::ERORR>("Failed to establish stealth link.");
        return false;
    }

    logger->log<e_level::INFO>("successfully attached to {} ({})", process_name, memory->get_process_id());

    std::string module_name = process_name;
    if (!memory->find_module_address(module_name))
    {
        logger->log<e_level::ERORR>("failed to find module address.");
        return false;
    }

    logger->log<e_level::INFO>("successfully found module base: 0x{:X}", memory->get_module_address());

    globals::roblox_window = find_main_window(memory->get_process_id());
    if (globals::roblox_window == nullptr)
    {
        logger->log<e_level::WARN>("unable to find roblox window");
    }

    logger->log<e_level::DEBUG>("Initializing runservice...");
    logger->log<e_level::DEBUG>("Scanning for datamodel pointer...");
    std::uint64_t fake_datamodel{ memory->read<std::uint64_t>(memory->get_module_address() + Offsets::FakeDataModel::Pointer) };
    if (!fake_datamodel) {
        logger->log<e_level::ERORR>("Failed to find FakeDataModel pointer at 0x{:X}", memory->get_module_address() + Offsets::FakeDataModel::Pointer);
        return false;
    }
    logger->log<e_level::DEBUG>("Found FakeDataModel: 0x{:X}", fake_datamodel);

    globals::datamodel = rbx::instance_t(memory->read<std::uint64_t>(fake_datamodel + Offsets::FakeDataModel::RealDataModel));
    if (!globals::datamodel.address) {
        logger->log<e_level::ERORR>("Failed to find RealDataModel address in FakeDataModel");
        return false;
    }
    logger->log<e_level::DEBUG>("Found RealDataModel: 0x{:X}", globals::datamodel.address);

    globals::visualengine = { memory->read<std::uint64_t>(memory->get_module_address() + Offsets::VisualEngine::Pointer) };
    if (globals::visualengine.address) {
        logger->log<e_level::DEBUG>("Found VisualEngine: 0x{:X}", globals::visualengine.address);
        globals::renderview = { memory->read<std::uint64_t>(globals::visualengine.address + Offsets::VisualEngine::RenderView_Offset) };
        logger->log<e_level::DEBUG>("Found RenderView: 0x{:X}", globals::renderview.address);
    }

    logger->log<e_level::DEBUG>("Searching for Players service...");
    globals::players = { globals::datamodel.find_first_child_by_class("Players") };
    if (!globals::players.address) {
        logger->log<e_level::ERORR>("Failed to find Players service");
        return false;
    }
    logger->log<e_level::DEBUG>("Found Players service: 0x{:X}", globals::players.address);

    globals::local_player = { memory->read<std::uint64_t>(globals::players.address + Offsets::Player::LocalPlayer) };
    if (!globals::local_player.address) {
        logger->log<e_level::ERORR>("Failed to find LocalPlayer");
        return false;
    }

    globals::workspace = { globals::datamodel.find_first_child_by_class("Workspace") };
    globals::lighting = { globals::datamodel.find_first_child_by_class("Lighting") };
    globals::run_service = { globals::datamodel.find_first_child_by_class("RunService") };
    globals::camera = { memory->read<std::uint64_t>(globals::workspace.address + Offsets::Workspace::CurrentCamera) };

    logger->log<e_level::INFO>("Runservice initialized successfully");
    return true;
}

