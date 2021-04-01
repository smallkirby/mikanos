bits 64
section .text

global IoOut32      ; void IoOut32(uint16_t addr, uint32_t data)
IoOut32:
  mov dx, di
  mov eax, esi
  out dx, eax
  ret

global IoIn32
IoIn32:
  mov dx, di
  in eax, dx
  ret

global GetCS       ; void GetCS(void)
GetCS:
  xor eax, eax
  mov ax, cs
  ret

global LoadIDT      ; void LoadIDT(uint16_t limit, uint64_t offset)
LoadIDT:
  push rbp
  mov rbp, rsp
  sub rsp, 10
  mov [rsp], di
  mov [rsp + 2], rsi
  lidt [rsp]
  mov rsp, rbp
  pop rbp
  ret

extern kernel_main_stack
extern KernelMainNewStack

global KernelMain
KernelMain:
  mov rsp, kernel_main_stack + 0x400 * 0x400
  call KernelMainNewStack

.fin:
  hlt
  jmp .fin