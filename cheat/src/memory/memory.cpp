#include "memory.h"
#include <iostream>
#include <vector>
#include "../output/output.h"

std::wstring to_wstring(const std::string& str)
{
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

std::uint32_t memory_interface::find_process_id(const std::string& process_name)
{
    process_id = driver->get_process_id(to_wstring(process_name).c_str());
    if (process_id == 0)
        logger->log<e_level::ERORR>("Process '{}' not found", process_name);
    else
        logger->log<e_level::DEBUG>("Found process '{}' -> ID: {}", process_name, process_id);
    return process_id;
}

std::uint64_t memory_interface::find_module_address(const std::string& module_name)
{
    base_address = driver->get_module_base(process_id, to_wstring(module_name).c_str());
    if (base_address == 0)
        logger->log<e_level::ERORR>("Module '{}' not found in process {}", module_name, process_id);
    else
        logger->log<e_level::DEBUG>("Found module '{}' -> Base: 0x{:X}", module_name, base_address);
    return base_address;
}

bool memory_interface::attach_to_process(const std::string& process_name)
{
    process_id = find_process_id(process_name);
    if (process_id != 0)
    {
        driver->set_process_id(process_id);
        return true;
    }
    return false;
}

std::string memory_interface::read_string(std::uint64_t address)
{
    if (!is_valid_address(address)) return "Unknown";

    std::int32_t string_length = read<std::int32_t>(address + 0x10);
    if (string_length <= 0 || string_length > 255)
    {
        return "Unknown";
    }

    std::uint64_t string_address = (string_length >= 16) ? read<std::uint64_t>(address) : address;
    if (!is_valid_address(string_address)) return "Unknown";

    std::vector<char> buffer(string_length + 1, 0);
    driver->read_memory(process_id, string_address, buffer.data(), (ULONG)buffer.size());

    return std::string(buffer.data(), string_length);
}

void memory_interface::write_string(std::uint64_t address, const std::string& value)
{
}

std::uint32_t memory_interface::get_process_id()
{
    return process_id;
}

std::uint64_t memory_interface::get_module_address()
{
    return base_address;
}

