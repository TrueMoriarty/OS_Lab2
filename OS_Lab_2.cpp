#include <iostream>
#include <windows.h>
#include <map>
#include <vector>


void PrintMenu() {
    std::cout << "+---+---------------------------------------------------------------------------------------+\n"
              << "| 1 | получение информации о вычислительной системе                                         |\n"
              << "+---+---------------------------------------------------------------------------------------+\n"
              << "| 2 | определение статуса виртуальной памяти                                                |\n"
              << "+---+---------------------------------------------------------------------------------------+\n"
              << "| 3 | определение состояния конкретного участка памяти по заданному с клавиатуры адресу     |\n"
              << "+---+---------------------------------------------------------------------------------------+\n"
              << "| 4 | резервирование региона в автоматическом режиме и в режиме ввода адреса начала региона |\n"
              << "+---+---------------------------------------------------------------------------------------+\n"
              << "| 5 | резервирование региона и передача ему физической памяти в автоматическом режиме       |\n"
              << "|   |                  и в режиме ввода адреса начала региона                               |\n"
              << "+---+---------------------------------------------------------------------------------------+\n"
              << "| 6 | Выделить автоматически                                                                |\n"
              << "+---+---------------------------------------------------------------------------------------+\n"
              << "| 7 | Выделить автоматически и передать физический регион                                   |\n"
              << "+---+---------------------------------------------------------------------------------------+\n"
              << "| 8 | Выделить по ptr                                                                       |\n"
              << "+---+---------------------------------------------------------------------------------------+\n"
              << "| 9 | Выделить по ptr физически                                                             |\n"
              << "+---+---------------------------------------------------------------------------------------+\n"
              << "| 0 | Выход из программы                                                                    |\n"
              << "+---+---------------------------------------------------------------------------------------+\n";
}

std::string ArchitectureToString(const WORD& arch) {
    switch (arch) {
    case PROCESSOR_ARCHITECTURE_INTEL: return "x86";
    case PROCESSOR_ARCHITECTURE_ARM: return "ARM";
    case PROCESSOR_ARCHITECTURE_IA64: return "Intel Itanium-based";
    case PROCESSOR_ARCHITECTURE_AMD64: return "x64";
    case PROCESSOR_ARCHITECTURE_ARM64: return "ARM64";
    default: return "Unknown architecture";
    }
}

void SystemInfo() {
    SYSTEM_INFO SysInfo;

    // Copy the hardware information to the SYSTEM_INFO structure. 

    GetSystemInfo(&SysInfo);

    // Display the contents of the SYSTEM_INFO structure.

    std::cout << "Hardware information: \n"
              << "  Processor architecture: " << ArchitectureToString(SysInfo.wProcessorArchitecture) << '\n'
              << "  Number of processors: " << SysInfo.dwNumberOfProcessors << "\n"
              << "  Allocation granularity: " << SysInfo.dwAllocationGranularity << '\n'
              << "  Page size: " << SysInfo.dwPageSize << "\n"
              << "  Processor type: " << SysInfo.dwProcessorType << "\n"
              << "  Minimum application address: " << SysInfo.lpMinimumApplicationAddress << "\n"
              << "  Maximum application address: " << SysInfo.lpMaximumApplicationAddress << "\n"
              << "  Active processor mask: " << SysInfo.dwActiveProcessorMask << "\n"
              << "  Processor level: " << SysInfo.wProcessorLevel << '\n'											// ok
              << "  Processor revision: " << SysInfo.wProcessorRevision << '\n';
}

void getGlobalMemoryStatus() {
    int DIV = 1024;

    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    if (!GlobalMemoryStatusEx(&status)) {
        std::cout << "ERROR: " << GetLastError() << '\n';
        return;
    }

    std::cout << "The MemoryStatus structure is bytes long " << status.dwLength << "\n"
         << "Memory load: " << status.dwMemoryLoad << "%\n"
         << "Available phys memory: " << status.ullAvailPhys / DIV << "/" << status.ullTotalPhys / DIV << " Kb\n"
         << "Available page memory: " << status.ullAvailPageFile / DIV << "/" << status.ullTotalPageFile / DIV << " Kb\n"
         << "Available virt memory: " << status.ullAvailPhys / DIV << "/" << status.ullTotalPhys / DIV << " Kb\n";
}

static const std::map<DWORD, std::string> state_str{
        {MEM_COMMIT, "committed"},
        {MEM_FREE, "free pages"},
        {MEM_RESERVE, "reserved pages"}
};

static const std::map<DWORD, std::string> type_str{
        {MEM_IMAGE, "mapped into the view of an image section"},
        {MEM_MAPPED, "mapped into the view of a section"},
        {MEM_PRIVATE, "private"}
};

static const std::vector<std::pair<DWORD, std::string>> mem_protect{
        {PAGE_EXECUTE,          "execute"},
        {PAGE_EXECUTE_READ,     "execute or read-only"},
        {PAGE_EXECUTE_READWRITE,"execute, read-only, or read/write"},
        {PAGE_EXECUTE_WRITECOPY,"execute, read-only, or copy-on-write"},
        {PAGE_NOACCESS,         "none"},
        {PAGE_READONLY,         "read-only"},
        {PAGE_READWRITE,        "read-only or read/write"},
        {PAGE_WRITECOPY,        "read-only or copy-on-write"},
        {PAGE_TARGETS_INVALID,  "invalid targets"},
        {PAGE_TARGETS_NO_UPDATE,"no update"}
};

static const std::vector<std::pair<DWORD, std::string>> mod_mem_protect{
        {PAGE_GUARD, "guard"},
        {PAGE_NOCACHE, "non-cachable"},
        {PAGE_WRITECOMBINE, "write-combined"},
        {0x00000000, ""}
};

void printMemProtect(const DWORD& protect) {
    for (const auto& base : mem_protect) {
        for (const auto& mod : mod_mem_protect) {
            if ((base.first | mod.first) == protect) {
                std::cout << "Memory protection: " << base.second << ' ' << mod.second << '\n';
                return;
            }
        }
    }
}

void getMemRegState() {
    void* ptr;
    MEMORY_BASIC_INFORMATION info;

    std::cout << "Enter pointer ";
    std::cin >> ptr;
    if (!(ptr)) {
        std::cout << "Invalid input\n";
        return;
    }
    if (!VirtualQuery(ptr, &info, sizeof(info))) {
        std::cout << "ERROR: " << GetLastError();
        return;
    }

    std::cout << "Base address: " << info.BaseAddress << '\n'
              << "Allocation base address: " << info.AllocationBase << '\n';
    if (info.Protect) printMemProtect(info.Protect);
    std::cout << "Region size: " << info.RegionSize << '\n'
              << "Type: pages within region are " << type_str.find(info.Type)->second << '\n'
              << "State: " << state_str.find(info.State)->second << '\n';
}

void VirtAlloc(void* ptr, const bool& phys) {
    SYSTEM_INFO info;
    GetSystemInfo(&info);

    DWORD type = MEM_RESERVE | ((phys) ? MEM_COMMIT : 0);
    void* a_ptr = nullptr;

    unsigned ch = 0;
    do {
        std::cout << "Enter amount of pages to allocate\n";
        if (!(std::cin >> ch) || ch < 1) {
            std::cout << "Invalid input: ";
            std::cout << '\n';
        }
    } while ((!ch) || ch < 1);

    a_ptr = VirtualAlloc(ptr, ch * info.dwPageSize, type, PAGE_EXECUTE_READWRITE);
    if (!a_ptr) {
        std::cout << "ERROR: " << GetLastError();
        return;
    }
    std::cout << "Allocated, base address: " << a_ptr << '\n';
}

void VirtAllocPtr() {
    void* ptr;
    std::cout << "Enter pointer ";
    if (!(std::cin >> ptr)) {
        std::cout << "Invalid input:\n";
        return;
    }
    VirtAlloc(ptr, false);
}

void VirtAllocPtrPhys() {
    void* ptr;
    std::cout << "Enter pointer ";
    if (!(std::cin >> ptr)) {
        std::cout << "Invalid input:\n";
        return;
    }
    VirtAlloc(ptr, true);
}

static const std::vector<std::pair<DWORD, std::string>> mem_protect_FOR_CHANGING{
        {PAGE_EXECUTE,          "execute"},
        {PAGE_EXECUTE_READ,     "execute or read-only"},
        {PAGE_EXECUTE_READWRITE,"execute, read-only, or read/write"},
        {PAGE_NOACCESS,         "none"},
        {PAGE_READONLY,         "read-only"},
        {PAGE_READWRITE,        "read-only or read/write"},
        {PAGE_TARGETS_NO_UPDATE,"no update"}
};

bool inputMemProtect(DWORD& protect) {
    unsigned ch = 0;

    do {
        system("cls");
        std::cout << "\tChoose main memory protection constant\n";
        unsigned i = 1;
        for (const auto& p : mem_protect_FOR_CHANGING) {
            std::cout << i << ". " << p.second << '\n';
            ++i;
        }
        std::cout << i << ". Exit\n";
        if (!(std::cin >> ch) || ch < 1 || ch > mem_protect_FOR_CHANGING.size() + 1) {
            std::cout << "Invalid input: ";
            std::cout << std::endl;
        }
    } while (!ch || ch < 1 || ch > mem_protect_FOR_CHANGING.size() + 1);

    if (ch == mem_protect_FOR_CHANGING.size() + 1) return false;

    protect |= mem_protect_FOR_CHANGING[ch - 1].first;
    ch = 0;

    do {
        system("cls");
        std::cout << "\tChoose secondary memory protection constant\n";
        unsigned i = 1;
        for (; i < mod_mem_protect.size(); ++i) {
            std::cout << i << ". " << mod_mem_protect[i - 1].second << '\n';
        }
        std::cout << i << ". No secondary\n";
        std::cout << i + 1 << ". Exit without setting protection constant\n";
        if (!(std::cin >> ch) || ch < 1 || ch > mod_mem_protect.size() + 2) {
            std::cout << "Invalid input: ";
            std::cout << std::endl;
        }
    } while (!ch || ch < 1 || ch > mod_mem_protect.size() + 2);

    if (ch == mod_mem_protect.size() + 1) return false;
    if (ch != mod_mem_protect.size()) protect |= mod_mem_protect[ch - 1].first;
    return true;

}

void VirtProt() {
    SYSTEM_INFO info;
    GetSystemInfo(&info);

    void* ptr;
    DWORD protect = 0, n_protect = 0;

    std::cout << "Enter pointer ";
    if (!(std::cin >> ptr)) {
        std::cout << "Invalid input: \n";
        return;
    }

    if (!inputMemProtect(n_protect)) {
        return;
    }

    if (!VirtualProtect(ptr, info.dwPageSize, n_protect, &protect)) {
        std::cout << "ERROR: " << GetLastError() << '\n';
        return;
    }

    std::cout << "Protection changed successfully.\nNew protection: ";
    printMemProtect(n_protect);
    std::cout << "Old protection: ";
    printMemProtect(protect);
}

void VirtFree() {
    void* ptr;

    std::cout << "Enter pointer ";
    if (!(std::cin >> ptr)) {
        std::cout << "Invalid input:\n";
        return;
    }

    if (!VirtualFree(ptr, 0, MEM_RELEASE)) {
        std::cout << "ERROR: " << GetLastError() << '\n';
        return;
    }
    std::cout << "Memory freed successfully\n";
}

int main()
{
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);
        
    int menu;
    void* ptr = nullptr;
    const bool& phys = false;
    const bool& phys_t = true;

    do
    {
        PrintMenu();
        std::cin >> menu;

        switch (menu)
        {
        case 1: {
            SystemInfo();
            break;
        }
        case 2: {
            getGlobalMemoryStatus();
            break;
        }
        case 3: {
            getMemRegState();
            break;
        }
        case 4:
            VirtAlloc(ptr, phys);
            break;
        case 5:
            VirtAlloc(ptr,phys_t);
            break;
        case 6:
            VirtAllocPtr();
            break;
        case 7:
            VirtAllocPtrPhys();
            break;
        case 8:
            VirtProt();
            break;
        case 9:
            VirtFree();
            break;
        default:
            break;
        }
    } while (menu != 0);
}

