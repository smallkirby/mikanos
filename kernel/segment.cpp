#include"segment.hpp"
#include"asmfunc.h"

namespace{
  std::array<SegmentDescriptor, 3> gdt;
}

void SetCodeSegment(SegmentDescriptor &desc, DescriptorType type, unsigned descriptor_privilege_level, uint32_t base, uint32_t limit) 
{
  desc.data = 0;

  desc.bits.base_low = base & 0xFFFFU;
  desc.bits.base_middle = (base >> 16) & 0xFFU;
  desc.bits.base_high = (base >> 24) & 0xFFU;

  desc.bits.limit_low = limit & 0xFFFFU;
  desc.bits.limit_high = (limit >> 16) & 0xFU;

  desc.bits.type = type;
  desc.bits.system_segment = 1;
  desc.bits.descriptor_privilege_level = descriptor_privilege_level;
  desc.bits.present = 1;
  desc.bits.available = 0;
  desc.bits.long_mode = 1;
  desc.bits.default_operation_size = 0;
  desc.bits.granularity = 1;
}

void SetDataSegment(SegmentDescriptor &desc, DescriptorType type, unsigned descriptor_privilege_level, uint32_t base, uint32_t limit) 
{
  SetCodeSegment(desc, type, descriptor_privilege_level, base, limit);
  desc.bits.long_mode = 0;
  desc.bits.default_operation_size = 1;
}

void SetupSegments() 
{
  gdt[0].data = 0;    // NULL descriptor
  SetCodeSegment(gdt[1], DescriptorType::kExecuteRead, 0, 0, 0xFFFFF);
  SetCodeSegment(gdt[2], DescriptorType::kReadWrite, 0, 0, 0xFFFFF);
  LoadGDT(sizeof(gdt) - 1, reinterpret_cast<uintptr_t>(&gdt[0]));
}
