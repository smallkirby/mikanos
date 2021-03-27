#include<cstdint>
#include<cstddef>
#include<cstdio>

#include"frame_buffer_config.hpp"
#include"graphics.hpp"
#include"font.hpp"
#include"console.hpp"

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

  // draw rectangle
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

  // draw string
  char strbuf[0x200];
  int year = 2021;
  sprintf(strbuf, "Hello, world %d", year);
  WriteString(*pixel_writer, 0x100, 0x100, strbuf, {30,30,30});

  Console console{*pixel_writer, {33, 33, 33}, {00, 0xFF, 00}};
  for (int ix = 0; ix != 7; ++ix){
    sprintf(strbuf, "LINE: %d\n", ix);
    console.PutString(strbuf);
  }
  sprintf(strbuf, "In my younger and more vulnerable years my father gave me some advice that I've been turning over in my mind ever since. \"Whenever you feel like criticizing any one,\" he told me, \"just remember that all the people in this world haven't had the advantages that you've had.\"");
  console.PutString(strbuf);

  hlt();
}