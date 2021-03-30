#include"interrupt.hpp"

std::array<InterruptDescriptor, 256> idt;

void NotifyEndOfInterrupt(void)
{
  // 0xFEE00000-0xFEE00400 is assigned as CPU registers
  // 0xFEE0B0 is End of Interrupt register
  volatile auto end_of_interrupt = reinterpret_cast<uint32_t*>(0xFEE000B0);
  *end_of_interrupt = 0;
}

void SetIDTEntry(InterruptDescriptor &desc, InterruptDescriptorAttribute attr, uint64_t offset, uint16_t segment_selector) 
{
  desc.attr = attr;
  desc.offset_low = offset & 0xFFFFU;
  desc.offset_middle = (offset >> 16) & 0xFFFFU;
  desc.offset_high = (offset >> 32);
  desc.segment_selector = segment_selector;
}
