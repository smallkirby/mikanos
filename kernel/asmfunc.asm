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