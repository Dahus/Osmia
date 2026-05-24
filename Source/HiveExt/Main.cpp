#include "pch.h"
#include "../OsmiaLib/ExtStartup.h"
#include <windows.h>
#include <fstream>

BOOL APIENTRY DllMain(HMODULE h, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        std::ofstream f("D:\\Programs\\Steam\\steamapps\\common\\Arma 2 Operation Arrowhead\\Osmia\\test.txt", std::ios::app);
        f << "ATTACH\n";
        f.flush();
    }
    return TRUE;
}

extern "C" __declspec(dllexport)
void __stdcall RVExtension(char* output, int outputSize, const char* function)
{
    std::ofstream f("D:\\Programs\\Steam\\steamapps\\common\\Arma 2 Operation Arrowhead\\Osmia\\test.txt", std::ios::app);
    f << "RV: " << function << "\n";
    f.flush();
    strncpy_s(output, outputSize, "[\"PASS\"]", 8);
}