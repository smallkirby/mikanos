#pragma once

#include<array>
#include<cstdint>

#include"x86_descriptor.hpp"

union SegmentDescriptor{
  uint64_t data;
  struct {
    uint64_t limit_low: 16;       // limit: ignored in x86
    uint64_t base_low: 16;        // base: ignored in x86
    uint64_t base_middle: 8;
    DescriptorType type: 4;
    uint64_t system_segment: 1;   // 1 if code-segment or data-segment
    uint64_t descriptor_privilege_level: 2; // DPL
    uint64_t present: 1;          // 1 if enabled
    uint64_t limit_high: 4;
    uint64_t available: 1;        // you can use as you like
    uint64_t long_mode: 1;        // 1 if 64bit code-segment
    uint64_t default_operation_size: 1;   // 0 if long_mode is 1
    uint64_t granularity: 1;      // 1 if 'limit' is regarded as 4KiB granuarity
    uint64_t base_high: 8;
  } __attribute__((packed)) bits;
} __attribute__((packed));

void SetCodeSegment(SegmentDescriptor &desc, DescriptorType type, unsigned descriptor_privilege_level, uint32_t base, uint32_t limit);

void SetDataSegment(SegmentDescriptor &desc, DescriptorType type, unsigned descriptor_privilege_level, uint32_t base, uint32_t limit);

void SetupSegments();
