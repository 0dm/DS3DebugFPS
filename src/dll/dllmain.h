#ifndef _dllmain_h
#define _dllmain_h

#include <windows.h>

#define importD3D(...) __VA_ARGS__;
#define IMPORT(a) a##_ = GetProcAddress(BaseAddressGenuine, #a)
#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

char useDebug = TRUE;
DWORD64 SprjFlipper;

RECT final;

NTSYSAPI NTSTATUS NTAPI NtReadVirtualMemory(HANDLE ProcessHandle, void *BaseAddress, const void *Buffer,
                                            SIZE_T NumberOfBytesToRead, PSIZE_T NumberOfBytesReaded);

NTSYSAPI NTSTATUS NTAPI NtWriteVirtualMemory(HANDLE ProcessHandle, void *BaseAddress, const void *Buffer,
                                             SIZE_T NumberOfBytesToWrite, PSIZE_T NumberOfBytesWritten);

void setFps(float rFPS);
float readFile();
void containCursor(void *args);
#endif
