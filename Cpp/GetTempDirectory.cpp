#include <windows.h>
#include <iostream>
#include <string>

std::string GetTempDirectory()
{
    char buffer[MAX_PATH];
    DWORD len = GetTempPathA(MAX_PATH, buffer);
    if (len == 0 || len > MAX_PATH) {
        throw std::runtime_error("Failed to get temp path");
    }
    return std::string(buffer);
}

int main() {
    try {
        std::string tempPath = GetTempDirectory();
        std::cout << "Temp Path: " << tempPath << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
    }
    return 0;
}
