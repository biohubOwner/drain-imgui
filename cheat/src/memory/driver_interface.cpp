#include "driver_interface.h"
#include "../output/output.h"
#include <TlHelp32.h>

driver_interface::driver_interface() :
    process_handle(nullptr),
    process_id(0),
    NtReadVirtualMemory(nullptr),
    NtWriteVirtualMemory(nullptr),
    NtOpenProcess(nullptr)
{}

driver_interface::~driver_interface()
{
    if (process_handle)
        CloseHandle(process_handle);
}

bool driver_interface::resolve_nt_functions()
{
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (!ntdll) return false;

    NtReadVirtualMemory = (pNtReadVirtualMemory)GetProcAddress(ntdll, "NtReadVirtualMemory");
    NtWriteVirtualMemory = (pNtWriteVirtualMemory)GetProcAddress(ntdll, "NtWriteVirtualMemory");
    NtOpenProcess = (pNtOpenProcess)GetProcAddress(ntdll, "NtOpenProcess");

    return (NtReadVirtualMemory && NtWriteVirtualMemory && NtOpenProcess);
}

bool driver_interface::initialize()
{
    if (!resolve_nt_functions())
    {
        logger->log<e_level::ERORR>("Failed to resolve NT functions for stealth.");
        return false;
    }

    logger->log<e_level::DEBUG>("Stealth user-mode interface initialized (NT Syscalls resolved).");
    return true;
}

void driver_interface::set_process_id(ULONG pid)
{
    std::lock_guard<std::recursive_mutex> lock(mtx);
    if (process_handle)
    {
        CloseHandle(process_handle);
        process_handle = nullptr;
    }

    process_id = pid;

    OBJECT_ATTRIBUTES obj_attr;
    InitializeObjectAttributes(&obj_attr, NULL, 0, NULL, NULL);

    CLIENT_ID client_id;
    client_id.UniqueProcess = (HANDLE)pid;
    client_id.UniqueThread = 0;


    ACCESS_MASK access = PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_QUERY_LIMITED_INFORMATION;

    NTSTATUS status = NtOpenProcess(&process_handle, access, &obj_attr, &client_id);

    if (status == 0)
        logger->log<e_level::DEBUG>("Opened stealth handle to process ID: {}", pid);
    else
        logger->log<e_level::ERORR>("Failed to open stealth handle (NT Status: 0x{:X}). Error: {}", (unsigned int)status, GetLastError());
}

std::string driver_interface::decrypt_string(const std::vector<unsigned char>& encrypted, unsigned char key)
{
    std::string decrypted;
    for (unsigned char c : encrypted) decrypted += (char)(c ^ key);
    return decrypted;
}

std::wstring driver_interface::decrypt_wstring(const std::vector<unsigned char>& encrypted, unsigned char key)
{
    std::wstring decrypted;
    for (size_t i = 0; i < encrypted.size(); i += 2) {
        wchar_t wc = (wchar_t)(encrypted[i] | (encrypted[i+1] << 8));
        decrypted += (wchar_t)(wc ^ key);
    }
    return decrypted;
}

std::string driver_interface::to_string(const std::wstring& wstr)
{
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

bool driver_interface::read_memory(ULONG pid, ULONG64 address, PVOID buffer, ULONG size)
{
    std::lock_guard<std::recursive_mutex> lock(mtx);
    if (!process_handle || process_id != pid)
        set_process_id(pid);

    if (!process_handle) return false;

    SIZE_T bytes_read;
    NTSTATUS status = NtReadVirtualMemory(process_handle, (PVOID)address, buffer, size, &bytes_read);
    return (status == 0);
}

bool driver_interface::write_memory(ULONG pid, ULONG64 address, PVOID buffer, ULONG size)
{
    std::lock_guard<std::recursive_mutex> lock(mtx);
    if (!process_handle || process_id != pid)
        set_process_id(pid);

    if (!process_handle) return false;

    SIZE_T bytes_written;
    NTSTATUS status = NtWriteVirtualMemory(process_handle, (PVOID)address, buffer, size, &bytes_written);
    return (status == 0);
}

ULONG64 driver_interface::get_module_base(ULONG pid, const wchar_t* module_name)
{
    ULONG64 base = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (snapshot != INVALID_HANDLE_VALUE)
    {
        MODULEENTRY32W me;
        me.dwSize = sizeof(me);
        if (Module32FirstW(snapshot, &me))
        {
            do
            {
                if (_wcsicmp(me.szModule, module_name) == 0)
                {
                    base = (ULONG64)me.modBaseAddr;
                    break;
                }
            } while (Module32NextW(snapshot, &me));
        }
        CloseHandle(snapshot);
    }
    return base;
}

ULONG driver_interface::get_process_id(const wchar_t* process_name)
{
    ULONG pid = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot != INVALID_HANDLE_VALUE)
    {
        PROCESSENTRY32W pe;
        pe.dwSize = sizeof(pe);
        if (Process32FirstW(snapshot, &pe))
        {
            do
            {
                if (_wcsicmp(pe.szExeFile, process_name) == 0)
                {
                    pid = pe.th32ProcessID;
                    break;
                }
            } while (Process32NextW(snapshot, &pe));
        }
        CloseHandle(snapshot);
    }
    return pid;
}

bool driver_interface::get_key_state(int key)
{
    return (GetAsyncKeyState(key) & 0x8000) != 0;
}

bool driver_interface::mouse_move(long x, long y, unsigned short flags)
{
    INPUT input = { 0 };
    input.type = INPUT_MOUSE;
    input.mi.dx = x;
    input.mi.dy = y;
    input.mi.dwFlags = MOUSEEVENTF_MOVE | flags;
    return SendInput(1, &input, sizeof(INPUT)) != 0;
}

