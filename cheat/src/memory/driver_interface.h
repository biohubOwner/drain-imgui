#pragma once
#include <windows.h>
#include <string>
#include <memory>
#include <vector>
typedef struct _CLIENT_ID {
    HANDLE UniqueProcess;
    HANDLE UniqueThread;
} CLIENT_ID, *PCLIENT_ID;

typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _OBJECT_ATTRIBUTES {
    ULONG Length;
    HANDLE RootDirectory;
    PUNICODE_STRING ObjectName;
    ULONG Attributes;
    PVOID SecurityDescriptor;
    PVOID SecurityQualityOfService;
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

#ifndef InitializeObjectAttributes
#define InitializeObjectAttributes( p, n, a, r, s ) { \
    (p)->Length = sizeof( OBJECT_ATTRIBUTES );          \
    (p)->RootDirectory = r;                             \
    (p)->Attributes = a;                                \
    (p)->ObjectName = n;                                \
    (p)->SecurityDescriptor = s;                        \
    (p)->SecurityQualityOfService = NULL;               \
    }
#endif

typedef LONG NTSTATUS;
typedef NTSTATUS(NTAPI* pNtReadVirtualMemory)(
    HANDLE ProcessHandle,
    PVOID BaseAddress,
    PVOID Buffer,
    SIZE_T NumberOfBytesToRead,
    PSIZE_T NumberOfBytesReaded
);

typedef NTSTATUS(NTAPI* pNtWriteVirtualMemory)(
    HANDLE ProcessHandle,
    PVOID BaseAddress,
    PVOID Buffer,
    SIZE_T NumberOfBytesToWrite,
    PSIZE_T NumberOfBytesWritten
);

typedef NTSTATUS(NTAPI* pNtOpenProcess)(
    PHANDLE ProcessHandle,
    ACCESS_MASK DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes,
    PCLIENT_ID ClientId
);

#include <mutex>

class driver_interface
{
private:
    HANDLE driver_handle;
    HANDLE process_handle;
    ULONG process_id;
    std::recursive_mutex mtx;
    pNtReadVirtualMemory NtReadVirtualMemory;
    pNtWriteVirtualMemory NtWriteVirtualMemory;
    pNtOpenProcess NtOpenProcess;

    bool resolve_nt_functions();

public:
    driver_interface();
    ~driver_interface();

    bool initialize();
    void set_process_id(ULONG pid);


    std::string decrypt_string(const std::vector<unsigned char>& encrypted, unsigned char key);
    std::wstring decrypt_wstring(const std::vector<unsigned char>& encrypted, unsigned char key);

    std::string to_string(const std::wstring& wstr);

    bool read_memory(ULONG pid, ULONG64 address, PVOID buffer, ULONG size);
    bool write_memory(ULONG pid, ULONG64 address, PVOID buffer, ULONG size);

    ULONG64 get_module_base(ULONG pid, const wchar_t* module_name);
    ULONG get_process_id(const wchar_t* process_name);

    bool get_key_state(int key);
    bool mouse_move(long x, long y, unsigned short flags = 0);
};

inline std::unique_ptr<driver_interface> driver = std::make_unique<driver_interface>();

