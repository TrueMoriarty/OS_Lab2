#include <iostream>
#include <string>
#include <windows.h>
#include <map>

void PrintMenu() {
    std::cout << "+---+--------------+\n"
        << "| 1 | Запись       |\n"
        << "+---+--------------+\n"
        << "| 2 | Чтение       |\n"
        << "+---+--------------+\n"
        << "| 0 | Выход        |\n"
        << "+---+--------------+\n";
}

void FileMapping() {
    std::string nameFile, nameMap, data;
    HANDLE file, fileMap;
    LPVOID adressMappingFile;

    std::cout << "Введите имя файла: ";
    std::cin.ignore();
    std::getline(std::cin, nameFile);

    file = CreateFile(nameFile.data(), GENERIC_WRITE | GENERIC_READ, NULL, nullptr, CREATE_ALWAYS, NULL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        std::cout << "Ошибка при создании файла, код: " << GetLastError() << std::endl;
        return;
    }

    std::cout << "Введите название отображения: ";
    std::getline(std::cin, nameMap);

    fileMap = CreateFileMapping(file, nullptr, PAGE_READWRITE, NULL, 128, nameMap.data());
    if (!fileMap) {
        std::cout << "Ошибка про создании отображения, код: " << GetLastError();
        CloseHandle(file);
        return;
    }

    adressMappingFile = MapViewOfFile(fileMap, FILE_MAP_ALL_ACCESS, NULL, NULL, NULL);
    if (!adressMappingFile) {
        std::cout << "Ошибка отображения файла, код: " << GetLastError();
        CloseHandle(file);
        CloseHandle(fileMap);
        return;
    }

    std::cout << "Файл успешно спроецирован" << std::endl
        << "Введите данные: ";
    std::getline(std::cin, data);
    memcpy(adressMappingFile, data.c_str(), data.size());
    std::cout << "Данные успешно записаны" << std::endl;
    return;
}

void read() {
    std::string map_name;

    std::cout << "Введите название отображения:" << '\n';
    std::cin.ignore();
    std::getline(std::cin, map_name);
    auto mapping = OpenFileMapping(FILE_MAP_ALL_ACCESS, false, map_name.data());
    if (mapping == nullptr || mapping == INVALID_HANDLE_VALUE) {
        std::cout << "Ошибка отображения файла, код: " << GetLastError();
        return;
    }

    auto h = MapViewOfFile(mapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);

    if (h == INVALID_HANDLE_VALUE) {
        std::cout << "Файл не спроецирован: " << GetLastError();
        return;
    }

    std::string data(reinterpret_cast<char*>(h));
    std::cout << "Read data:\n" << data << '\n';
}

int main()
{
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);

    int menu;
    bool result;

    do
    {
        PrintMenu();
        std::cin >> menu;

        switch (menu)
        {
        case 1:
            FileMapping();
            break;
        case 2:
            read();
            break;
        default:
            break;
        }
    } while (menu != 0);
}

