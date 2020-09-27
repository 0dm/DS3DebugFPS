.macro loadnjmp sym
.section .text
.global \sym
\sym:
pushq %r9;pushq %r8;pushq %rdx;pushq %rcx
subq $0x28,%rsp
call LoadGenuineDll
addq $0x28,%rsp
popq %rcx;popq %rdx;popq %r8;popq %r9
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

