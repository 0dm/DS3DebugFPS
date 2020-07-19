#include <stdio.h>
#include <windows.h>
#define importD3D(...) __VA_ARGS__;__VA_ARGS__;
#define IMPORT(a) a##_=GetProcAddress(BaseAddressGenuine,#a)

typedef enum {false=0, true=1} bool; 
bool useDebug = true;
DWORD64 SprjFlipper = 0x144888440;

void setFps(float FPS) {
   
    // Find Process
    DWORD PID;
    HWND hWnd = FindWindowA(NULL, "DARK SOULS III");
    GetWindowThreadProcessId(hWnd, &PID);
    HANDLE pHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, PID);
    
    // Skip Intros (Credit to bladecoding, pawREP, hallcristobal for memory region)
    static unsigned char SkipIntro[20] = { 0x48, 0x31, 0xC0, 0x48, 0x89, 0x02, 0x49, 0x89, 0x04, 0x24, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90};
    WriteProcessMemory(pHandle, (void*)0x0000000140BE9D0F, SkipIntro, 20, 0);

    // Borderless
    RECT final;
    final.right = GetSystemMetrics(SM_CXSCREEN);
    final.bottom = GetSystemMetrics(SM_CYSCREEN);
    final.left = 0;
    final.top = 0;
    SetWindowLong(hWnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
    AdjustWindowRect(&final, GetWindowLong(hWnd, GWL_STYLE), false);
    SetWindowLong(hWnd, GWL_EXSTYLE, (GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_TOPMOST));
    MoveWindow(hWnd, final.left, final.top, final.right - final.left, final.bottom - final.top, true);

    // GRAPHICS -> GFX
    ReadProcessMemory(pHandle, (LPVOID)SprjFlipper, &SprjFlipper, sizeof(SprjFlipper), NULL);
    WriteProcessMemory(pHandle, (LPVOID)(SprjFlipper + 0x354), &FPS, sizeof(DWORD), NULL); // Debug FPS | デバッグFPS
    WriteProcessMemory(pHandle, (LPVOID)(SprjFlipper + 0x358), &useDebug, sizeof(bool), NULL); // Use Debug FPS | デバッグFPSを利用するか
}

float readFps() {
	FILE* fs = fopen("fps.txt", "r");
	float read;
	if (fs == NULL){fs = fopen("fps.txt", "w"); fprintf(fs,"%d",1000);} // default to 1000
	fscanf(fs, "%f", &read);
	fclose(fs);
	return read > 0 ? read: 1000;
	
}

HINSTANCE BaseAddress, BaseAddressGenuine; wchar_t* BaseFileName, FullFilePath[512];
BOOL WINAPI DllMain(HINSTANCE baseaddr, DWORD reason, BOOL isstatic) {
    switch (reason) {
    case DLL_PROCESS_ATTACH:
        BaseFileName = FullFilePath + GetModuleFileNameW(baseaddr, FullFilePath, _countof(FullFilePath));
        while (BaseFileName-- > FullFilePath)if (*BaseFileName == L'\\')break;
    case DLL_THREAD_ATTACH:
        break;
    }
    return 1;
}

importD3D(FARPROC D3DAssemble_, DebugSetMute_, D3DCompile_, D3DCompressShaders_, D3DCreateBlob_, D3DDecompressShaders_, D3DDisassemble_, D3DDisassemble10Effect_, D3DGetBlobPart_, D3DGetDebugInfo_, D3DGetInputAndOutputSignatureBlob_, D3DGetInputSignatureBlob_,
    D3DGetOutputSignatureBlob_, D3DPreprocess_, D3DReflect_, D3DReturnFailure1_, D3DStripShader_)
    void LoadGenuineDll() {
    if (!BaseAddressGenuine) {
        static wchar_t filename[512];
        GetSystemDirectoryW(filename, _countof(filename));
        BaseAddressGenuine = LoadLibraryW(wcscat(filename, BaseFileName));
        IMPORT(D3DAssemble);
        IMPORT(DebugSetMute);
        IMPORT(D3DCompile);
        IMPORT(D3DCompressShaders);
        IMPORT(D3DCreateBlob);
        IMPORT(D3DDecompressShaders);
        IMPORT(D3DDisassemble);
        IMPORT(D3DDisassemble10Effect);
        IMPORT(D3DGetBlobPart);
        IMPORT(D3DGetDebugInfo);
        IMPORT(D3DGetInputAndOutputSignatureBlob);
        IMPORT(D3DGetInputSignatureBlob);
        IMPORT(D3DGetOutputSignatureBlob);
        IMPORT(D3DPreprocess);
        IMPORT(D3DReflect);
        IMPORT(D3DReturnFailure1);
        IMPORT(D3DStripShader);
        setFps(readFps());
    }
}
