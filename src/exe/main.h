#ifndef _main_h
#define _main_h

#include <windows.h>

#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

char useDebug = TRUE;
DWORD64 SprjFlipper = 0x144888440;
RECT final;

NTSYSAPI NTSTATUS NTAPI NtReadVirtualMemory(HANDLE ProcessHandle, void *BaseAddress, const void *Buffer,
                                            SIZE_T NumberOfBytesToRead, PSIZE_T NumberOfBytesReaded); // yes, "readed"...

NTSYSAPI NTSTATUS NTAPI NtWriteVirtualMemory(HANDLE ProcessHandle, void *BaseAddress, const void *Buffer,
                                             SIZE_T NumberOfBytesToWrite, PSIZE_T NumberOfBytesWritten);

void setFps(float rFPS);
float readFile();
#endif