#include<cstdint>
#include<cstddef>
#include<cstdio>

#include"frame_buffer_config.hpp"
#include"graphics.hpp"
#include"font.hpp"
#include"console.hpp"
#include"color.hpp"
#include"pci.hpp"

#define noreturn

void noreturn hlt(){
  while(1==1){
    __asm__("hlt");
  }
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

  // list all the PCI devices
  auto err = pci::ScanAllBus();
  printk("ScanAllBus: %s\n", err.Name());
  for(int ix=0; ix!=pci::num_device; ++ix){
    const auto& dev = pci::devices[ix];
    auto vendor_id = pci::ReadVendorId(dev.bus, dev.device, dev.function);
    auto class_code = pci::ReadClassCode(dev.bus, dev.device, dev.function);
    printk("%d.%d.%d: vend=%04x, class=%08x, head=%02x\n", dev.bus, dev.device, dev.function, vendor_id, class_code, dev.header_type);
  }

  hlt();
}