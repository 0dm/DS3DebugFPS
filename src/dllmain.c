#include "dllmain.h"
#include "lib/inih/ini.h"

typedef struct {
    float fps;
    int UseCustomScreenDimensions;
    int ScreenWidth;
    int ScreenHeight;
    int EnableCursorClip;
    int CursorClipHotkey;
} config;
config configFile;

DWORD WINAPI containCursor(void* args) {
    while (TRUE) {
        if (GetAsyncKeyState(configFile.CursorClipHotkey)) {
            if (GetForegroundWindow() == args)
                SetCapture(args);
                ClipCursor(&final);
                
        }
    }
}


static int handler(void* Settings, const char* section, const char* name, const char* value) {
    config* modconfig = (config*)Settings;
    if (MATCH("Settings", "fps")) {
        modconfig->fps = atof(value);
    }
    if (MATCH("Settings", "UseCustomScreenDimensions")){
        modconfig->UseCustomScreenDimensions = atoi(value);
    }
    else if (MATCH("Settings", "ScreenWidth")){
        modconfig->ScreenWidth = atoi(value);
    }
    else if (MATCH("Settings", "ScreenHeight")){
        modconfig->ScreenHeight = atoi(value);
    }
    else if (MATCH("Settings", "EnableCursorClip")){
        modconfig->EnableCursorClip = atoi(value);
    }
    else if (MATCH("Settings", "CursorClipHotkey")){
        modconfig->CursorClipHotkey = strtol(value, NULL, 16);
    }
     else 
        return 0; // error
    return 1;

}


float readFile() {   
    return ini_parse("FPSconfig.ini", handler, &configFile) < 0 ? 1000: configFile.fps; // will set framerate limit to 1000 if the ini file is missing
}

void setFps(float rFPS) {
   
    // Find Process
    DWORD PID;
    HWND hWnd = FindWindowA(NULL, "DARK SOULS III");
    GetWindowThreadProcessId(hWnd, &PID);
    HANDLE pHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, PID);
    
    // Skip Intros (Credit to bladecoding, pawREP, hallcristobal for memory region)
    unsigned char SkipIntro[20] = {0x48, 0x31, 0xC0, 0x48, 0x89, 0x02, 0x49, 0x89, 0x04, 0x24, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90};
    WriteProcessMemory(pHandle, (LPVOID)0x140BE9D0F, SkipIntro, 20, 0);

    // Borderless
    if (configFile.UseCustomScreenDimensions != 1) {
        final.right = GetSystemMetrics(SM_CXSCREEN);
        final.bottom = GetSystemMetrics(SM_CYSCREEN);
    } 
    else {
        final.right = configFile.ScreenWidth;
        final.bottom = configFile.ScreenHeight;
    }
    final.left = 0;
    final.top = 0;
    SetWindowLong(hWnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
    AdjustWindowRect(&final, GetWindowLong(hWnd, GWL_STYLE), FALSE);
    SetWindowLong(hWnd, GWL_EXSTYLE, (GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_TOPMOST));
    MoveWindow(hWnd, final.left, final.top, final.right - final.left, final.bottom - final.top, TRUE);

    // GRAPHICS -> GFX
    ReadProcessMemory(pHandle, (LPVOID)SprjFlipper, &SprjFlipper, sizeof(SprjFlipper), NULL);
    WriteProcessMemory(pHandle, (LPVOID)(SprjFlipper + 0x354), &rFPS, sizeof(DWORD), NULL); // Debug FPS | デバッグFPS
    WriteProcessMemory(pHandle, (LPVOID)(SprjFlipper + 0x358), &useDebug, sizeof(char), NULL); // Use Debug FPS | デバッグFPSを利用するか

    if (configFile.EnableCursorClip != 0) {
        HANDLE thread = CreateThread(NULL, 0, containCursor, &hWnd, 0, NULL); // createthread for an asynchronous loop
    }
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
        setFps(readFile());
    }
}
