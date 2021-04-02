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

global LoadGDT     ; void LoadGDT(uint16_t limit, uint64_t offset);
LoadGDT:
  push rbp
  mov rbp, rsp
  sub rsp, 10
  mov [rsp], di
  mov [rsp + 2], rsi
  lgdt [rsp]
  mov rsp, rbp
  pop rbp
  ret

global SetCSSS      ; void SetDSSS(uint16_t cs, uint16_t cs)
SetCSSS:
  push rbp
  mov rbp, rsp
  mov ss, si
  mov rax, .next
  push rdi
  push rax
  o64 retf
.next:
  mov rsp, rbp
  pop rbp
  ret

global SetCR3       ; void SetCR3(uint64_t value)
SetCR3:
  mov cr3, rdi
  ret

global SetDSAll     ; void SetDSAll(uint16_t value)
SetDSAll:
  mov ds, di
  mov es, di
  mov fs, di
  mov gs, di
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