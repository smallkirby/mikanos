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
#include"queue.hpp"
#include"memory_map.hpp"
#include"segment.hpp"
#include"paging.hpp"
#include"memory_manager.hpp"
#include"window.hpp"
#include"layer.hpp"

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

// memory manager
char memory_manager_buf[sizeof(BitmapMemoryManager)];
BitmapMemoryManager *memory_manager;

char console_buf[sizeof(Console)];
Console *console;

char pixel_writer_buf[sizeof(RGBResv8BitPerColorPixelWriter)];
PixelWriter *pixel_writer = nullptr;

unsigned mouse_layer_id;

usb::xhci::Controller *xhc;

// holds a type of interruption
struct Message{
  enum Type{
    kInterruptXHCI,
  } type;
};

// interruption message queue
ArrayQueue<Message> *main_queue;

/*** (END globals) ***********/

void MouseObserver(int8_t displacement_x, int8_t displacement_y)
{
  if(displacement_x == 0 && displacement_y == 0){
    return;
  }
  layer_manager->MoveRelative(mouse_layer_id, {displacement_x, displacement_y});
  layer_manager->Draw();
}

int printk(const char *format, ...)
{
  if(console == nullptr){
    return -1;
  }

  va_list ap;
  int result;
  char s[0x400];

  va_start(ap, format);
  result = vsprintf(s, format, ap);
  va_end(ap);

  console->PutString(s);
  return result;
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

// interrupt handler of xHCI device.
// just put the message onto the message queue, and process it lazily.
__attribute__((interrupt))
void IntHandlerXHCI(InterruptFrame *frame)
{
  main_queue->Push(Message{Message::kInterruptXHCI});
  NotifyEndOfInterrupt();

}

alignas(0x10) uint8_t kernel_main_stack[0x400 * 0x400];

extern "C" void KernelMainNewStack(const FrameBufferConfig &frame_buffer_config_ref, const MemoryMap &memory_map_ref)
{
  // save arguments into new stack, cuz EFI area is regarded as free and can be overwritten by this kernel itself.
  FrameBufferConfig frame_buffer_config{frame_buffer_config_ref};
  MemoryMap memory_map{memory_map_ref};

  // configure writer
  switch(frame_buffer_config.pixel_format){
    case kPixelRGBResv8BitPerColor:
      pixel_writer = new(pixel_writer_buf) RGBResv8BitPerColorPixelWriter{frame_buffer_config};
      break;
    case kPixelBGRResv8BitPerColor:
      pixel_writer = new(pixel_writer_buf) BGRResv8BitPerColorPixelWriter{frame_buffer_config};
      break;
  }

  // setup console for early printk
  DrawDesktop(*pixel_writer);
  console = new(console_buf) Console{C_NORMAL_FG, C_NORMAL_BG};
  console->SetWriter(pixel_writer);

  // set-up segments and change CS/SS
  SetupSegments();

  const uint16_t kernel_cs = 1 << 3;
  const uint16_t kernel_ss = 2 << 3;
  SetDSAll(0);
  SetCSSS(kernel_cs, kernel_ss);

  // init paging
  SetupIdentityPageTable();

  // init memory-map manager
  ::memory_manager = new(memory_manager_buf) BitmapMemoryManager;
  const auto memory_map_base = reinterpret_cast<uintptr_t>(memory_map.buffer);
  uintptr_t available_end = 0;
  for(uintptr_t iter = memory_map_base; iter  < memory_map_base + memory_map.map_size; iter += memory_map.descriptor_size){
    auto desc = reinterpret_cast<const MemoryDescriptor*>(iter);
    // if there is a gap between `physical_start` and `available_end`, mark it as "ALLOCATED".
    if(available_end < desc->physical_start){
      memory_manager->MarkAllocated(FrameID{available_end / kBytesPerFrame}, (desc->physical_start - available_end) / kBytesPerFrame);
    }

    const auto physical_end = desc->physical_start + desc->number_of_pages * kUEFIPageSize;
    if(IsAvailable(static_cast<MemoryType>(desc->type))){
      available_end = physical_end;
    }else{
      memory_manager->MarkAllocated(FrameID{desc->physical_start / kBytesPerFrame}, desc->number_of_pages * kUEFIPageSize / kBytesPerFrame);
    }
    // not processed frames are treated as AVAILABLE(not allocated) as default.
  }
  memory_manager->SetMemoryRange(FrameID{1}, FrameID{available_end / kBytesPerFrame});

  // init heap
  if(auto err = InitializeHeap(*memory_manager)){
    Log(kError, "[!] Failed to allocate pages: %s at %s:%d\n", err.Name(), err.File(), err.Line());
    exit(1);
  }

  const std::array available_memory_types{
    MemoryType::kEfiBootServicesCode,
    MemoryType::kEfiBootServicesData,
    MemoryType::kEfiConventionalMemory,
  };


  // parse memory map
  SetLogLevel(kDebug);
  Log(kDebug, "[+] memory_map: %p\n", &memory_map);
  for(uintptr_t iter = reinterpret_cast<uintptr_t>(memory_map.buffer); iter < reinterpret_cast<uintptr_t>(memory_map.buffer) + memory_map.map_size; iter += memory_map.descriptor_size){
    auto desc = reinterpret_cast<MemoryDescriptor*>(iter);
    for(int ix=0; ix!=available_memory_types.size(); ++ix){
      if(desc->type == available_memory_types[ix]){
        Log(kDebug, "mm: type=%x, phys=%08lx - %08lx, pages=%x, attr=%08llx\n", desc->type, desc->physical_start, desc->physical_start + desc->number_of_pages * 4096 - 1, desc->number_of_pages, desc->attribute);
      }
    }
  }
  SetLogLevel(kWarn);

  // prepare message queue
  std::array<Message, 32> main_queue_data;
  ArrayQueue<Message> main_queue(main_queue_data);
  ::main_queue = &main_queue;

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


  const int kFrameWidth = frame_buffer_config.horizontal_resolution;
  const int kFrameHeight = frame_buffer_config.vertical_resolution;
  // setup desktop layer and draw desktop
  auto bgwindow = std::make_shared<Window>(kFrameWidth, kFrameHeight);
  auto bgwriter = bgwindow->Writer();
  DrawDesktop(*bgwriter);
  // setup mouse layer
  auto mouse_window = std::make_shared<Window>(kMouseCursorWidth, kMouseCursorHeight);
  mouse_window->SetTransparentColor(kMouseTransparentColor);
  DrawMouseCursor(mouse_window->Writer(), {0,0});
  // setup console layer
  auto console_window = std::make_shared<Window>(80*16, 25*16);
  auto console_writer = console_window->Writer();
  console->SetWriter(console_writer);
  console->Clear();

  // setup layer-manager
  layer_manager = new LayerManager;
  layer_manager->SetWriter(pixel_writer);

  auto bglayer_id = layer_manager->NewLayer()
    .SetWindow(bgwindow)
    .Move({0,0})
    .ID();
  mouse_layer_id = layer_manager->NewLayer()
    .SetWindow(mouse_window)
    .Move({200, 200})
    .ID();
  auto console_layer_id = layer_manager->NewLayer()
    .SetWindow(console_window)
    .Move({10,10})
    .ID();
  Log(kWarn, "[+] console_layer is setup.\n");
  
  layer_manager->UpDown(bglayer_id, 0);
  layer_manager->UpDown(console_layer_id, 1);
  layer_manager->UpDown(mouse_layer_id, 99);
  layer_manager->Draw();

  // poll message queue and process it
  while(1==1){
    __asm__("cli"); // ignore all the maskable interrupts.
    if(main_queue.Count() == 0){
      __asm__("sti");
      __asm__("hlt");
      continue;
    }

    Log(kDebug, "Message Queue: 0x%x/0x%x\n", main_queue.Count(), main_queue.Capacity());
    Log(kDebug, "Handling message...\n");
    Message msg = main_queue.Front();
    main_queue.Pop();
    Log(kDebug, "Message: 0x%x\n", msg.type);
    Log(kDebug, "Message Queue: 0x%x/0x%x\n", main_queue.Count(), main_queue.Capacity());
    __asm__("sti");

    switch(msg.type){
      case Message::kInterruptXHCI:
        while(xhc.PrimaryEventRing()->HasFront()){
          if(auto err = ProcessEvent(xhc)){
            Log(kError, "Error while ProcessEvent: %s at %s: %d\n", err.Name(), err.File(), err.Line());
          }
        }
        break;

      default:
        Log(kError, "Unknown message type: 0x%x\n", msg.type);
        break;
    }
  }

  Log(kError, "[!] Reached unreachable point!\n");
}

extern "C" void __cxa_pure_virtual() {
  while(1==1){
    __asm__("hlt");
  }
}