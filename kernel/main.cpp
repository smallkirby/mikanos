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
#include"mouse.hpp"
#include"interrupt.hpp"
#include"asmfunc.h"

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


static int kFrameWidth;
static int kFrameHeight;

char mouse_cursor_buf[sizeof(MouseCursor)];
MouseCursor *mouse_cursor;

usb::xhci::Controller *xhc;

/*** (END globals) ***********/

void MouseObserver(int8_t displacement_x, int8_t displacement_y)
{
  mouse_cursor->MoveRelative({displacement_x, displacement_y});
}

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

  mouse_cursor = new(mouse_cursor_buf) MouseCursor{pixel_writer, C_DESKTOP_BG, {300,200}};
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

__attribute__((interrupt))
void IntHandlerXHCI(InterruptFrame *frame)
{
  while(xhc->PrimaryEventRing()->HasFront()){
    if(auto err = ProcessEvent(*xhc)){
      Log(kError, "Error while ProcessEvent: %s at %s: %d\n", err.Name(), err.File(), err.Line());
    }
  }
  NotifyEndOfInterrupt();
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

  // set IDT for xHCI device
  const uint16_t cs = GetCS();
  Log(kDebug, "CS: 0x%x\n", cs);
  SetIDTEntry(idt[InterruptVector::kXHCI], MakeIDTAttr(DescriptorType::kInterruptGate, 0), reinterpret_cast<uint64_t>(IntHandlerXHCI), cs);
  LoadIDT(sizeof(idt)-1, reinterpret_cast<uintptr_t>(&idt[0]));

  // enable interruption for xHCI device
  const uint8_t bsp_local_apic_id = *reinterpret_cast<const uint32_t*>(0xFEE00020) >> 24; // get CPU core number, BSP means Bootstrap Processor
  Log(kDebug, "Bootstrap Processor APIC num: #0x%x\n", bsp_local_apic_id);
  pci::ConfigureMSIFixedDestination(*xhc_dev, bsp_local_apic_id, pci::MSITriggerMode::kLevel, pci::MSIDeliveryMode::kFixed, InterruptVector::kXHCI, 0);

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

  ::xhc = &xhc;
  __asm__("sti");   // enable interruption

  usb::HIDMouseDriver::default_observer = MouseObserver;
  // find connected device and configure
  for(int ix=1; ix<=xhc.MaxPorts(); ++ix){
    auto port = xhc.PortAt(ix);
    Log(kDebug, "Port %d: IsConnected=%d\n", ix, port.IsConnected());
    if(port.IsConnected()){
      if(auto err = ConfigurePort(xhc, port)){
        Log(kError, "failed to configure port: %s at %s: %d\n", err.Name(), err.File(), err.Line());
        continue;
      }else{
        Log(kDebug, "configured port: %s at %s: %d\n", err.Name(), err.File(), err.Line());
      }
    }
  }

  hlt();
}

extern "C" void __cxa_pure_virtual() {
  while(1==1){
    __asm__("hlt");
  }
}