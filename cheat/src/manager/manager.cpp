#include "manager.h"
#include <windows.h>
#include <DbgHelp.h>
#include <filesystem>

#pragma comment(lib, "dbghelp.lib")

namespace manager {

    void crash_manager::init() {
        SetUnhandledExceptionFilter(exception_handler);
        logger->log<e_level::DEBUG>("Crash manager initialized");
    }

    long __stdcall crash_manager::exception_handler(struct _EXCEPTION_POINTERS* exceptionInfo) {
        logger->log<e_level::ERORR>("CRITICAL CRASH DETECTED!");
        logger->log<e_level::ERORR>("Exception Code: 0x{:X}", exceptionInfo->ExceptionRecord->ExceptionCode);
        logger->log<e_level::ERORR>("Exception Address: 0x{:X}", (uintptr_t)exceptionInfo->ExceptionRecord->ExceptionAddress);
        HANDLE hFile = CreateFileA("crash_dump.dmp", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile != INVALID_HANDLE_VALUE) {
            MINIDUMP_EXCEPTION_INFORMATION mei;
            mei.ThreadId = GetCurrentThreadId();
            mei.ExceptionPointers = exceptionInfo;
            mei.ClientPointers = FALSE;

            MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &mei, NULL, NULL);
            CloseHandle(hFile);
            logger->log<e_level::DEBUG>("Minidump created: crash_dump.dmp");
        }
        return EXCEPTION_CONTINUE_SEARCH;
    }

}

