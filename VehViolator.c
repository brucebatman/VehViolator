#include <windows.h>
#include <winternl.h>
#include <stdio.h>

#pragma comment(lib, "user32.lib")

// Prototype for NtQueryInformationProcess
typedef NTSTATUS(NTAPI* pNtQueryInformationProcess)(
    HANDLE ProcessHandle,
    PROCESSINFOCLASS ProcessInformationClass,
    PVOID ProcessInformation,
    ULONG ProcessInformationLength,
    PULONG ReturnLength);

LONG NTAPI CustomVEH(EXCEPTION_POINTERS* ExceptionInfo) {
    printf("[+] VEH handler caught exception: 0x%0.8X\n", ExceptionInfo->ExceptionRecord->ExceptionCode);
    if (ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_INT_DIVIDE_BY_ZERO) {
#if defined(_M_X64) || defined(__amd64__)
        // Adjust instruction pointer for x64
        ExceptionInfo->ContextRecord->Rip += 2;
#elif defined(_M_IX86) || defined(__i386__)
        // Adjust instruction pointer for x86
        ExceptionInfo->ContextRecord->Eip += 2;
#endif
        // Spawn a process from VEH handler
        STARTUPINFO si = { sizeof(si) };
        PROCESS_INFORMATION pi;
        if (CreateProcessA(NULL, "calc.exe", NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
            printf("[+] Successfully spawned calc.exe\n");
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        } else {
            printf("[-] Failed to spawn calc.exe\n");
        }
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

void OverwriteFirstVEH() {
    // Register our custom VEH handler
    PVOID handler = AddVectoredExceptionHandler(1, CustomVEH);
    if (handler == NULL) {
        printf("[-] Failed to register VEH handler.\n");
    } else {
        printf("[+] VEH handler registered successfully.\n");
    }
}

int main() {
    OverwriteFirstVEH();
    __try {
        int zero = 0;
        int result = 1 / zero; // Trigger divide by zero exception
    } __except (CustomVEH(GetExceptionInformation()), EXCEPTION_EXECUTE_HANDLER) {
        // Handle non-divide by zero exceptions here, if needed
    }
    return 0;
}
