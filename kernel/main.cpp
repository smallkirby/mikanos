#include<cstdint>
#include<cstddef>
#include<cstdio>

#include"frame_buffer_config.hpp"
#include"graphics.hpp"
#include"font.hpp"
#include"console.hpp"
#include"color.hpp"
#include"pci.hpp"
#include"logger.hpp"

#include"usb/memory.hpp"
#include"usb/device.hpp"
#include"usb/classdriver/mouse.hpp"
#include"usb/xhci/xhci.hpp"
#include"usb/xhci/trb.hpp"

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

// Intel Panther Point chipset has both EHCI(USB2.0) and xHCI controllers,
// and uses EHCI by default. Hence, switch to a xHCI controller.
void SwitchEhci2Xhci(const pci::Device &xhc_dev) {
  bool intel_ehc_exist = false;
  for (int ix = 0; ix != pci::num_device; ++ix) {
    if (pci::devices[ix].class_code.Match(0x0cu, 0x03u, 0x20u) /* EHCI */ && 0x8086 == pci::ReadVendorId(pci::devices[ix]) /* Intel */ ) {
      intel_ehc_exist = true;
      break;
    }
  }
  if (!intel_ehc_exist) {
    return;
  }

  uint32_t superspeed_ports = pci::ReadConfReg(xhc_dev, 0xdc);  // USB3PRM
  pci::WriteConfReg(xhc_dev, 0xd8, superspeed_ports);           // USB3_PSSEN
  uint32_t ehci2xhci_ports = pci::ReadConfReg(xhc_dev, 0xd4);   // XUSB2PRM
  pci::WriteConfReg(xhc_dev, 0xd0, ehci2xhci_ports);            // XUSB2PR
  Log(kDebug, "SwitchEhci2Xhci: SS = %02, xHCI = %02x\n", superspeed_ports, ehci2xhci_ports);
}

extern "C" void KernelMain(const FrameBufferConfig &frame_buffer_config)
{
  SetLogLevel(kDebug);

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

  // find xHC device
  pci::Device *xhc_dev = nullptr;
  for(int ix=0; ix!=pci::num_device; ++ix){
    if(pci::devices[ix].class_code.Match(0x0CU, 0x03U, 0x30U)){
      xhc_dev = &pci::devices[ix];
      // prefer Intel device
      if(0x8086 == pci::ReadVendorId(*xhc_dev)){
        break;
      }
    }
  }
  if(xhc_dev){
    Log(kInfo, "xHC has been found: %d.%d.%d\n", xhc_dev->bus, xhc_dev->device, xhc_dev->function);
  }else{
    Log(kError, "xHC not found...\n");
  }

  // read MMIO addr from BAR0 register in PCI configuration space
  const WithError<uint64_t> xhc_bar = pci::ReadBar(*xhc_dev, 0);
  Log(kDebug, "ReadBar: %s\n", xhc_bar.error.Name());
  const uint64_t xhc_mmio_base = xhc_bar.value & ~static_cast<uint64_t>(0xF);
  Log(kDebug, "xHC mmio_base = 0x%lx\n", xhc_mmio_base);

  // reset, init, config xHC
  usb::xhci::Controller xhc{xhc_mmio_base};
  if(0x8086 == pci::ReadVendorId(*xhc_dev)){
    SwitchEhci2Xhci(*xhc_dev);
  }
  {
    auto err = xhc.Initialize();
    Log(kDebug, "xhc.Initialize: %s\n", err.Name());
  }
  Log(kInfo, "xHC starting\n");
  xhc.Run();

  hlt();
}

extern "C" void __cxa_pure_virtual() {
  while(1==1){
    __asm__("hlt");
  }
}