#pragma once
#include <windows.h>
#include <TlHelp32.h>
#include <vector>
#include <string>
#include <memory>
#include "driver_interface.h"

class memory_interface final
{
public:
	memory_interface() = default;
	~memory_interface() = default;

	std::uint32_t find_process_id(const std::string& process_name);
	std::uint64_t find_module_address(const std::string& module_name);

	bool attach_to_process(const std::string& process_name);

	std::string read_string(std::uint64_t address);
	void write_string(std::uint64_t address, const std::string& value);

	template <typename T>
	T read(std::uint64_t address);

	template <typename T>
	bool read(std::uint64_t address, T* buffer, std::size_t size = sizeof(T));

	template <typename T>
	void write(std::uint64_t address, T value);

	std::uint32_t get_process_id();
	std::uint64_t get_module_address();

    bool is_valid_address(std::uint64_t address)
    {
        return (address > 0x1000 && address < 0x7FFFFFFFFFFF);
    }
private:
	std::uint32_t process_id;
	std::uint64_t base_address;
};

template <typename T>
T memory_interface::read(uint64_t address)
{
	T buffer{};
    if (!is_valid_address(address)) return buffer;
	driver->read_memory(process_id, address, &buffer, sizeof(T));
	return buffer;
}

template <typename T>
bool memory_interface::read(uint64_t address, T* buffer, std::size_t size)
{
    if (!is_valid_address(address) || !buffer) return false;
	return driver->read_memory(process_id, address, buffer, (ULONG)size);
}

template <typename T>
void memory_interface::write(uint64_t address, T value)
{
    if (!is_valid_address(address)) return;
	driver->write_memory(process_id, address, &value, sizeof(T));
}

inline std::unique_ptr<memory_interface> memory = std::make_unique<memory_interface>();

