#ifndef PATTERN_SCAN_H
#define PATTERN_SCAN_H

#include <windows.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Pattern should be a byte array, mask is a string with 'x' for match, '?' for wildcard
DWORD64 AobScan(HANDLE pHandle, const BYTE* pattern, const char* mask, const char* moduleName);

#ifdef __cplusplus
}
#endif

#endif // PATTERN_SCAN_H
