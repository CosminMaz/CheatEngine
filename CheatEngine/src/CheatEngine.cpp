#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <vector>
#include <cstring>

DWORD FindPidByName(const char* name) {
    HANDLE h;
    PROCESSENTRY32 singleProcess;
    h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (h == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to create process snapshot." << std::endl;
        return 0;
    }

    singleProcess.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(h, &singleProcess)) {
        do {
            if (strcmp(singleProcess.szExeFile, name) == 0) {
                DWORD pid = singleProcess.th32ProcessID;
                CloseHandle(h);
                return pid;
            }
        } while (Process32Next(h, &singleProcess));
    }

    CloseHandle(h);
    return 0;
}

void ScanForValue(HANDLE handleProcess, const void* valueToFind, size_t valueSize) {
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);

    MEMORY_BASIC_INFORMATION memoryInfo;
    void* currentScan = nullptr;

    while (VirtualQueryEx(handleProcess, currentScan, &memoryInfo, sizeof(memoryInfo))) {
        currentScan = (char*)memoryInfo.BaseAddress + memoryInfo.RegionSize;

        if (memoryInfo.State == MEM_COMMIT && (memoryInfo.Protect & PAGE_READWRITE)) {
            std::vector<char> buffer(memoryInfo.RegionSize);
            SIZE_T bytesRead;

            if (ReadProcessMemory(handleProcess, memoryInfo.BaseAddress, buffer.data(), memoryInfo.RegionSize, &bytesRead)) {
                for (size_t i = 0; i <= bytesRead - valueSize; ++i) {
                    if (memcmp(buffer.data() + i, valueToFind, valueSize) == 0) {
                        std::cout << "Found value at address: " << (void*)((char*)memoryInfo.BaseAddress + i) << std::endl;
                    }
                }
            }
            else {
                std::cerr << "Failed to read memory at: " << memoryInfo.BaseAddress << std::endl;
            }
        }
    }
}

void EngineLoop() {
    int command = 1;
    while (command) {
        std::cout << "Type a command: \n";
        std::cout << "1 -> Select process.\n";

    }

}

int main() {
    const int valueToFind = 5; 
    auto pid = FindPidByName("testeCheatEngine.exe");
    if (pid) {
        std::cout << "Process found with PID: " << pid << std::endl;

        HANDLE handleProcess = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid);

        if (handleProcess) {
            ScanForValue(handleProcess, &valueToFind, sizeof(valueToFind));
            CloseHandle(handleProcess);
        }
        else {
            std::cerr << "Failed to open process." << std::endl;
        }
    }
    else {
        std::cerr << "Process not found." << std::endl;
    }

    std::cin.ignore();
    std::cin.get();

    return 0;
}
