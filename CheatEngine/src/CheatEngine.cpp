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

void ScanVirtualPages(HANDLE handleProcess) {
    SYSTEM_INFO sSysInfo = {};
    GetSystemInfo(&sSysInfo);

    MEMORY_BASIC_INFORMATION memInfo;

    void* currentScanAdress = 0;

    while (true) {
        SIZE_T bytes = VirtualQueryEx(handleProcess, currentScanAdress, &memInfo, sizeof(MEMORY_BASIC_INFORMATION));

        if (!bytes) {
            return;
        }

        currentScanAdress = (char*)memInfo.BaseAddress + memInfo.RegionSize;

        if (memInfo.State == MEM_COMMIT) {
            if (memInfo.Protect == PAGE_READWRITE) {
                std::cout << "Found READWRITE page at base address: " << memInfo.BaseAddress << " Size: " << memInfo.RegionSize
                    << " = pages count: " << memInfo.RegionSize / sSysInfo.dwPageSize << std::endl;
            }
        }

    }
}

void ScanVirtualPagesForValue(HANDLE handleProcess, int targetValue, std::vector<void*> &test) {
    int addresses_found = 0;
    SYSTEM_INFO sSysInfo = {};
    GetSystemInfo(&sSysInfo);

    MEMORY_BASIC_INFORMATION memInfo;
    void* currentScanAddress = nullptr;
    while (true) {
        SIZE_T bytes = VirtualQueryEx(handleProcess, currentScanAddress, &memInfo, sizeof(MEMORY_BASIC_INFORMATION));

        if (!bytes) {
            std::cout << "Address with value " << targetValue << " found: " << addresses_found << std::endl;
            return;
        }

        currentScanAddress = (char*)memInfo.BaseAddress + memInfo.RegionSize;

        if (memInfo.State == MEM_COMMIT && (memInfo.Protect & PAGE_READWRITE || memInfo.Protect & PAGE_READONLY)) {
            SIZE_T regionSize = memInfo.RegionSize;
            //std::shared_ptr<char> shared_buffer = std::make_shared<char>(regionSize);
            SIZE_T bytesRead;
            char* buffer = new char[regionSize];
            if (ReadProcessMemory(handleProcess, memInfo.BaseAddress, buffer, regionSize, &bytesRead)) {
                for (SIZE_T i = 0; i < bytesRead - sizeof(int); ++i) {
                    int* intPtr = (int*)(buffer + i);
                    if (*intPtr == targetValue) {
                        //std::cout << "Found value at address: " << (void*)((char*)memInfo.BaseAddress + i) << std::endl;
                        //std::cout << *(int*)(buffer + i) << std::endl;
                        test.push_back((void*)((char*)memInfo.BaseAddress + i));
                        ++addresses_found;
                    }
                }
            }
            
            /*
            char* buffer = new char[regionSize];
            SIZE_T bytesRead;
            if (ReadProcessMemory(handleProcess, memInfo.BaseAddress, buffer, regionSize, &bytesRead)) {
                for (SIZE_T i = 0; i < bytesRead - sizeof(int); i++) {
                    int* intPtr = (int*)(buffer + i);
                    if (*intPtr == targetValue) {
                        std::cout << "Found value at address: " << (void*)((char*)memInfo.BaseAddress + i) << std::endl;
                        std::cout << *(int*)(buffer + i) << std::endl;
                        test.push_back(buffer + i);
                    }
                }
            }
            delete[] buffer;
            */
        }
    }
}

void ReScanVirtualPagesForValue(HANDLE handleProcess, int targetValue, std::vector<void*>& test) {
    std::vector<void*> temp;
    
    SYSTEM_INFO sSysInfo = {};
    GetSystemInfo(&sSysInfo);

    MEMORY_BASIC_INFORMATION memInfo;
    void* currentScanAddress = nullptr;
    while (true) {
        SIZE_T bytes = VirtualQueryEx(handleProcess, currentScanAddress, &memInfo, sizeof(MEMORY_BASIC_INFORMATION));

        if (!bytes) {
            std::cout << "Address with value " << targetValue << " found: " << temp.size() << std::endl;
            test.clear();
            test = temp;
            return;
        }

        currentScanAddress = (char*)memInfo.BaseAddress + memInfo.RegionSize;

        if (memInfo.State == MEM_COMMIT && (memInfo.Protect & PAGE_READWRITE || memInfo.Protect & PAGE_READONLY)) {
            SIZE_T regionSize = memInfo.RegionSize;
            //std::shared_ptr<char> shared_buffer = std::make_shared<char>(regionSize);
            SIZE_T bytesRead;
            char* buffer = new char[regionSize];
            if (ReadProcessMemory(handleProcess, memInfo.BaseAddress, buffer, regionSize, &bytesRead)) {
                for (SIZE_T i = 0; i < bytesRead - sizeof(int); ++i) {
                    int* intPtr = (int*)(buffer + i);
                    if (*intPtr == targetValue && (std::count(test.begin(), test.end(), (void*)((char*)memInfo.BaseAddress + i)) != 0)) {
                        temp.push_back((void*)((char*)memInfo.BaseAddress + i));
                       //std::cout << "Found value at address: " << (void*)((char*)memInfo.BaseAddress + i) << std::endl;
                       //std::cout << *(int*)(buffer + i) << std::endl;
                        /*
                        int newData = 546;
                        if (WriteProcessMemory(handleProcess, (void*)((char*)memInfo.BaseAddress + i), &newData, sizeof(newData), nullptr)) {
                            std::cout << "Succesfully written the new data!" << std::endl;
                        }
                        else {
                            std::cout << "Failed to open process" << std::endl;
                        }
                        */
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
    std::string input;
    std::cout << "Enter a string: ";
    std::getline(std::cin, input);
    const char* to_return = input.c_str();
    return to_return;
}

void SearchValue(const DWORD &pid, std::vector<void*> &addr, const int &value) {
    HANDLE handleProcess;
    if (pid) {
        std::cout << "Process found with PID: " << pid << std::endl;
        handleProcess = OpenProcess(PROCESS_ALL_ACCESS, 0, pid);
        if (handleProcess) {
            ScanVirtualPagesForValue(handleProcess, value, addr);
            CloseHandle(handleProcess);
        } else {
            std::cerr << "Failed to open process." << std::endl;
        }
    } else {
        std::cerr << "Process not found." << std::endl;
    }
}

void ReSearchValue(const DWORD& pid, std::vector<void*> &addr, const int& value){
    HANDLE handleProcess;
    if (pid) {
        std::cout << "Process found with PID: " << pid << std::endl;
        handleProcess = OpenProcess(PROCESS_ALL_ACCESS, 0, pid);
        if (handleProcess) {
            ReScanVirtualPagesForValue(handleProcess, value, addr);
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

void EngineLoop() {
    const char* process_name;
    int value, command = 1;
    std::vector<void*> addresses;
    //std::cout << "Enter a program to find pid: ";
    //process_name = GetProcessName();
    auto pid = FindPidByName("javaw.exe");

    while (true) {
        std::cout << "-----------------";
        std::cout << "Type a command:" << std::endl;
        std::cout << "1) Choose a value to search" << std::endl;
        std::cout << "2) Search the value in memory" << std::endl;
        std::cout << "3) Research the value in memory" << std::endl;
        std::cout << "4) Write memory address" << std::endl;
        std::cout << "-----------------" << std::endl;
        std::cin >> command;
        switch (command) {
        case 1:
            std::cin >> value;
            break;
        case 2:
            SearchValue(pid, addresses, value);
            break;
        case 3:
            ReSearchValue(pid, addresses, value);
            break;
        case 4:
            WriteInMemory(pid, addresses);
            break;
        default:
            std::cout << "Invalid command." << std::endl;
        }
    }

}

int main() {
    EngineLoop();
    /*
    std::vector<void *> test;
    const int valueToFind = 5; 
    HANDLE handleProcess;
    auto pid = FindPidByName("testeCheatEngine.exe");
    if (pid) {
        std::cout << "Process found with PID: " << pid << std::endl;

        handleProcess = OpenProcess(PROCESS_ALL_ACCESS, 0, pid);

        if (handleProcess) {
            
            std::cout << "Enter memory address:\n";
            void* address = 0;
            std::cin >> address;
            int data = 0;

            ReadProcessMemory(handleProcess, address, &data, sizeof(data), nullptr);

            std::cout << data << std::endl;

            int newData = 546;
            if (WriteProcessMemory(handleProcess, address, &newData, sizeof(newData), nullptr)) {
                std::cout << "Succesfully written the new data!" << std::endl;
            } else {
                std::cout << "Failed to open process" << std::endl;
            }
            
            ScanVirtualPages(handleProcess);
            
            ScanVirtualPagesForValue(handleProcess, 5, test);
            CloseHandle(handleProcess);
        } else {
            std::cerr << "Failed to open process." << std::endl;
        }
    }
    else {
        std::cerr << "Process not found." << std::endl;
    }

    std::cout << std::endl;

    for (int i = 0; i < test.size(); i++) {
        std::cout << test[i] << std::endl;
    }

    std::cin.ignore();
    std::cin.get();

    handleProcess = OpenProcess(PROCESS_ALL_ACCESS, 0, pid);
    ReScanVirtualPagesForValue(handleProcess, 69, test);
    */

    return 0;
}
