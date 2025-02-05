#include "../CheatEngine.h"

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

void ScanVirtualPagesForValue(HANDLE handleProcess, int targetValue, std::vector<void*>& test, bool& first_find) {
    int addresses_found = 0;
    std::vector<void*> temp;
    SYSTEM_INFO sSysInfo = {};
    GetSystemInfo(&sSysInfo);

    MEMORY_BASIC_INFORMATION memInfo;
    void* currentScanAddress = nullptr;
    while (true) {
        SIZE_T bytes = VirtualQueryEx(handleProcess, currentScanAddress, &memInfo, sizeof(MEMORY_BASIC_INFORMATION));

        if (!bytes) {
            if (first_find) {
                std::cout << "Address with value1 " << targetValue << " found: " << addresses_found << std::endl;
                first_find = false;
            } else {
                std::cout << "Address with value " << targetValue << " found: " << temp.size() << std::endl;
                test.clear();
                test = temp;
            }
            return;
        }

        currentScanAddress = (char*)memInfo.BaseAddress + memInfo.RegionSize;

        if (memInfo.State == MEM_COMMIT && (memInfo.Protect & PAGE_READWRITE || memInfo.Protect & PAGE_READONLY)) {
            SIZE_T regionSize = memInfo.RegionSize;

            SIZE_T bytesRead;
            char* buffer = new char[regionSize];
            if (ReadProcessMemory(handleProcess, memInfo.BaseAddress, buffer, regionSize, &bytesRead)) {
                for (SIZE_T i = 0; i < bytesRead - sizeof(int); ++i) {
                    int* intPtr = (int*)(buffer + i);
                    if (*intPtr == targetValue) {
                        if (first_find) {
                            test.push_back((void*)((char*)memInfo.BaseAddress + i));
                            ++addresses_found;
                        } else {
                            if ((std::count(test.begin(), test.end(), (void*)((char*)memInfo.BaseAddress + i)) != 0)) {
                                temp.push_back((void*)((char*)memInfo.BaseAddress + i));
                            }
                        }
                    }
                }
            }
        }
    }
}


void WriteInMemory(const DWORD& pid, std::vector<void*>& addr) {
    int newData;
    std::cout << "Enter new data to write in memory: ";
    std::cin >> newData;
    std::cout << std::endl;
    HANDLE handleProcess;

    if (pid) {
        std::cout << "Process found with PID: " << pid << std::endl;
        handleProcess = OpenProcess(PROCESS_ALL_ACCESS, 0, pid);
        if (handleProcess) {
            if (WriteProcessMemory(handleProcess, addr[0], &newData, sizeof(newData), nullptr)) {
                std::cout << "Succesfully written the new data!" << std::endl;
            }
            else {
                std::cout << "Failed to open process" << std::endl;
            }
            CloseHandle(handleProcess);
        }
        else {
            std::cerr << "Failed to open process." << std::endl;
        }
    }
    else {
        std::cerr << "Process not found." << std::endl;
    }
}

const char* GetProcessName() {
    /*
    std::string input;
    std::getline(std::cin, input);
    const char* str = input.c_str();
    return str;
    */
    static std::string input;  // Static ensures it persists after function returns
    std::getline(std::cin, input);
    return input.c_str();
}

void EngineLoop() {
    bool first_find = true;
    HANDLE handleProcess = nullptr;
    int value = 0, command = 1;
    std::vector<void*> addresses;
    std::cout << "Enter a program to find pid: ";
    const char* process_name = GetProcessName();
    auto pid = FindPidByName(process_name);

    if (pid) {
        std::cout << "Process found with PID: " << pid << std::endl;
        handleProcess = OpenProcess(PROCESS_ALL_ACCESS, 0, pid);
        if (handleProcess) {
            std::cout << "Process opened succesfully." << std::endl;
        } else {
            std::cerr << "Failed to open process." << std::endl;
        }
    } else {
        std::cerr << "Process not found" << std::endl;
    }

    while (true) {
        std::cout << "-----------------";
        std::cout << "Type a command:" << std::endl;;
        std::cout << "1) Choose a value to search" << std::endl;
        std::cout << "2) Search the value in memory" << std::endl;
        std::cout << "3) Write memory address" << std::endl;
        std::cout << "-----------------" << std::endl;
        std::cout << "Enter command: ";
        std::cin >> command;
        std::cout << std::endl;
        switch (command) {
        case 1:
            std::cout << "Enter new value to search: ";
            std::cin >> value;
            std::cout << std::endl;
            break;
        case 2:
            ScanVirtualPagesForValue(handleProcess, value, addresses, first_find);
            break;
        case 3:
            WriteInMemory(pid, addresses);
            break;
        default:
            std::cout << "Invalid command." << std::endl;
        }
    }

}

int main() {
    EngineLoop();
    return 0;
}
