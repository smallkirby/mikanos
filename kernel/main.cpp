#include<cstdint>
#include<cstddef>

#include"frame_buffer_config.hpp"
#include"graphics.hpp"
#include"font.hpp"

#define noreturn

void noreturn hlt(){
  while(1==1){
    __asm__("hlt");
  }
}

void* operator new(size_t size, void *buf){
  return buf;
}
void operator delete(void *obj) noexcept{
}

char pixel_writer_buf[sizeof(RGBResv8BitPerColorPixelWriter)];
PixelWriter *pixel_writer = nullptr;

extern "C" void KernelMain(const FrameBufferConfig &frame_buffer_config)
{
  switch(frame_buffer_config.pixel_format){
    case kPixelRGBResv8BitPerColor:
      pixel_writer = new(pixel_writer_buf) RGBResv8BitPerColorPixelWriter{frame_buffer_config};
      break;
    case kPixelBGRResv8BitPerColor:
      pixel_writer = new(pixel_writer_buf) BGRResv8BitPerColorPixelWriter{frame_buffer_config};
      break;
  }

  if(pixel_writer != nullptr){
    for(int x=0; x!=frame_buffer_config.horizontal_resolution; ++x){
      for(int y=0; y!=frame_buffer_config.vertical_resolution; ++y){
        pixel_writer->Write(x, y, {0xff, 0xff, 0xff});
      }
    }
    for(int x=frame_buffer_config.horizontal_resolution/3; x!=frame_buffer_config.horizontal_resolution/3 * 2; ++x){
      for(int y=frame_buffer_config.vertical_resolution/4; y!=frame_buffer_config.vertical_resolution/4*3; ++y){
          pixel_writer->Write(x, y, {0xff, 0x00, 0x00});
      }
    }
  }

  char *mystr = "ASCII chars.\x00", *ptr;
  for(ptr = mystr; *ptr != 0; ++ptr){
    WriteAscii(*pixel_writer, 0x100 + (ptr-mystr)*8, 0x100, *ptr, {0,0,0});
  }

  hlt();
}