#include "AsphaltDLL.h"

BOOL APIENTRY DllMain(HMODULE hmodule, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hmodule);
        AsphaltDLL::Initialize(hmodule);
    }

    return TRUE;
}

