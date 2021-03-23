#include<cstdint>

extern "C" void KernelMain(uint64_t frame_buffer_base, uint64_t frame_buffer_size)
{
  uint8_t *frame_buffer = reinterpret_cast<uint8_t*>(frame_buffer_base);
  for(uint64_t ix=0; ix!=frame_buffer_size; ++ix){
    frame_buffer[ix] = ix % 0x100;
  }

  while(1==1){
    __asm__("hlt");
  }
}