#include "pattern_scan.h"
#include <psapi.h>
#include <stdio.h>

DWORD64 AobScan(HANDLE pHandle, const BYTE* pattern, const char* mask, const char* moduleName) {
    HMODULE hModule = GetModuleHandleA(moduleName);
    if (!hModule) {
        printf("Failed to get handle for module: %s\n", moduleName);
        return 0;
    }

    MODULEINFO moduleInfo;
    if (!GetModuleInformation(pHandle, hModule, &moduleInfo, sizeof(moduleInfo))) {
        printf("Failed to get module information for: %s\n", moduleName);
        return 0;
    }

    BYTE* moduleBase = (BYTE*)moduleInfo.lpBaseOfDll;
    DWORD moduleSize = moduleInfo.SizeOfImage;
    size_t patternLen = strlen(mask);

    for (DWORD i = 0; i <= moduleSize - patternLen; ++i) {
        BOOL found = TRUE;
        for (size_t j = 0; j < patternLen; ++j) {
            if (mask[j] == 'x' && pattern[j] != moduleBase[i + j]) {
                found = FALSE;
                break;
            }
        }
        if (found) {
            return (DWORD64)(moduleBase + i);
        }
    }

    printf("AOB pattern not found in module: %s\n", moduleName);
    return 0;
}
