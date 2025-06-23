@gcc -o D3DCompiler_43.dll asm.s dllmain.c pattern_scan.c -O3 -lmsvcrt -lntdll -shared -Wno-multichar -Wl,--image-base,0x150000000,-e,DllMain
@pause