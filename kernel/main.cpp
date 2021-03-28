#include<cstdint>
#include<cstddef>
#include<cstdio>

#include"frame_buffer_config.hpp"
#include"graphics.hpp"
#include"font.hpp"
#include"console.hpp"
#include"color.hpp"

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

/******* globals *************/
char console_buf[sizeof(Console)];
Console *console;

char pixel_writer_buf[sizeof(RGBResv8BitPerColorPixelWriter)];
PixelWriter *pixel_writer = nullptr;

const int kMouseCursorWidth = 15;
const int kMouseCursorHeight = 24;
const char mouse_cursor_shape[kMouseCursorHeight][kMouseCursorWidth + 1] = {
  "@              ",
  "@@             ",
  "@.@            ",
  "@..@           ",
  "@...@          ",
  "@....@         ",
  "@.....@        ",
  "@......@       ",
  "@.......@      ",
  "@........@     ",
  "@.........@    ",
  "@..........@   ",
  "@...........@  ",
  "@............@ ",
  "@......@@@@@@@@",
  "@......@       ",
  "@....@@.@      ",
  "@...@ @.@      ",
  "@..@   @.@     ",
  "@.@    @.@     ",
  "@@      @.@    ",
  "@       @.@    ",
  "         @.@   ",
  "         @@@   ",
};

static int kFrameWidth;
static int kFrameHeight;

/*** (END globals) ***********/


int printk(const char *format, ...)
{
  va_list ap;
  int result;
  char s[0x400];

  va_start(ap, format);
  result = vsprintf(s, format, ap);
  va_end(ap);

  console->PutString(s);
  return result;
}

void drawDesktop(PixelWriter &writer)
{
  FillRectangle(writer, {0,0}, {kFrameWidth, kFrameHeight}, C_DESKTOP_BG);
  FillRectangle(writer, {0, kFrameHeight-50}, {kFrameWidth, 50}, C_DESKTOP_FOOTER);
  FillRectangle(writer, {0, kFrameHeight-50 + 3}, {kFrameHeight/5, 50-3-3}, C_DESKTOP_ACCENT);
  DrawRectangle(writer, {0, kFrameHeight-50 + 3}, {kFrameHeight/5, 50-3-3}, C_DESKTOP_EDGE);

}

extern "C" void KernelMain(const FrameBufferConfig &frame_buffer_config)
{
  kFrameWidth = frame_buffer_config.horizontal_resolution;
  kFrameHeight = frame_buffer_config.vertical_resolution;
  switch(frame_buffer_config.pixel_format){
    case kPixelRGBResv8BitPerColor:
      pixel_writer = new(pixel_writer_buf) RGBResv8BitPerColorPixelWriter{frame_buffer_config};
      break;
    case kPixelBGRResv8BitPerColor:
      pixel_writer = new(pixel_writer_buf) BGRResv8BitPerColorPixelWriter{frame_buffer_config};
      break;
  }

  drawDesktop(*pixel_writer);
  console = new(console_buf) Console{*pixel_writer, C_NORMAL_FG, C_NORMAL_BG};

  // draw string
  char strbuf[0x200];
  int year = 2021;
  sprintf(strbuf, "Hello, world %d", year);
  WriteString(*pixel_writer, 0x100, 0x100, strbuf, {30,30,30}, {0xFF,0xFF,0xFF});

  for (int ix = 0; ix != 7; ++ix){
    printk("LINE: %d\n", ix);
  }
  printk("In my younger and more vulnerable years my father gave me some advice that I've been turning over in my mind ever since. \"Whenever you feel like criticizing any one,\" he told me, \"just remember that all the people in this world haven't had the advantages that you've had.\"");

  // render mouse cursor
  for(int dy=0; dy!=kMouseCursorHeight; ++dy){
    for(int dx=0; dx!=kMouseCursorWidth; ++dx){
      if(mouse_cursor_shape[dy][dx] == '@'){
        pixel_writer->Write(200 + dx, 100 + dy, C_MOUSE_GRID);
      }else if(mouse_cursor_shape[dy][dx] == '.'){
        pixel_writer->Write(200 + dx, 100 + dy, C_MOUSE_NORMAL);
      }
    }
  }

  hlt();
}