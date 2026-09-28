.macro loadnjmp sym
.section .text
.global \sym
\sym:
pushq %r9
pushq %r8
pushq %rdx
pushq %rcx
subq $0x28,%rsp
call LoadGenuineDll
addq $0x28,%rsp
popq %rcx
popq %rdx
popq %r8
popq %r9
jmp *\sym\()_(%rip)
.section .drectve
.ascii "-export:\sym\n"
.endm

.section .text
loadnjmp D3DAssemble
loadnjmp DebugSetMute
loadnjmp D3DCompile
loadnjmp D3DCompressShaders
loadnjmp D3DCreateBlob
loadnjmp D3DDecompressShaders
loadnjmp D3DDisassemble
loadnjmp D3DDisassemble10Effect
loadnjmp D3DGetBlobPart
loadnjmp D3DGetDebugInfo
loadnjmp D3DGetInputAndOutputSignatureBlob
loadnjmp D3DGetInputSignatureBlob
loadnjmp D3DGetOutputSignatureBlob
loadnjmp D3DPreprocess
loadnjmp D3DReflect
loadnjmp D3DReturnFailure1
loadnjmp D3DStripShader

.section .text
.global SpeedHook
SpeedHook:
pushq %r9
pushq %r8
pushq %rdx
pushq %rcx
subq $0x68,%rsp
movdqu %xmm0,0x20(%rsp)
movdqu %xmm1,0x30(%rsp)
movdqu %xmm2,0x40(%rsp)
movdqu %xmm3,0x50(%rsp)
call UpdateSpeedFactors
movdqu 0x20(%rsp),%xmm0
movdqu 0x30(%rsp),%xmm1
movdqu 0x40(%rsp),%xmm2
movdqu 0x50(%rsp),%xmm3
addq $0x68,%rsp
popq %rcx
popq %rdx
popq %r8
popq %r9
movq %rsp,%r11
pushq %rdi
subq $0x70,%rsp
jmp *SpeedHookReturn(%rip)

