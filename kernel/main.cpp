#include<cstdint>
#include"frame_buffer_config.hpp"

#define noreturn

void noreturn hlt(){
  while(1==1){
    __asm__("hlt");
  }
}

struct PixelColor{
  uint8_t r,g,b;
};

int WritePixel(const FrameBufferConfig &config, int x, int y, const PixelColor &c)
{
  const int pixel_position = config.pixels_per_scan_line * y + x;
  uint8_t *p;
  switch(config.pixel_format){
    case kPixelRGBResv8BitPerColor:
      p = &config.frame_buffer[4 * pixel_position]; // one pixel is 4bytes (1byte is reserved)
      p[0] = c.r;
      p[1] = c.g;
      p[2] = c.b;
      break;
    case kPixelBGRResv8BitPerColor:
      p = &config.frame_buffer[4 * pixel_position];
      p[0] = c.b;
      p[1] = c.g;
      p[2] = c.r;
      break;
    default:
      return -1;
  }
  return 0;
}

extern "C" void KernelMain(const FrameBufferConfig &frame_buffer_config)
{
  for(int x=0; x!=frame_buffer_config.horizontal_resolution; ++x){
    for(int y=0; y!=frame_buffer_config.vertical_resolution; ++y){
      WritePixel(frame_buffer_config, x, y, {0xff, 0xff, 0xff});
    }
  }
  for(int x=frame_buffer_config.horizontal_resolution/3; x!=frame_buffer_config.horizontal_resolution/3 * 2; ++x){
    for(int y=frame_buffer_config.vertical_resolution/4; y!=frame_buffer_config.vertical_resolution/4*3; ++y){
      WritePixel(frame_buffer_config, x, y, {0xff, 0x00, 0x00});
    }
  }

  hlt();
}