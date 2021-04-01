#include<Uefi.h>
#include<Library/UefiLib.h>
#include<Library/UefiBootServicesTableLib.h>
#include<Library/PrintLib.h>
#include<Protocol/LoadedImage.h>
#include<Protocol/SimpleFileSystem.h>
#include<Protocol/DiskIo2.h>
#include<Protocol/BlockIo.h>
#include<Guid/FileInfo.h>
#include<Library/MemoryAllocationLib.h>
#include<Library/BaseMemoryLib.h>

#include"frame_buffer_config.hpp"
#include"memory_map.hpp"
#include"elf.hpp"

#define KERN_LOAD_BASE 0x100000

#define PAGE 0x1000

#define noreturn

void noreturn Halt(void)
{
  while(1==1){
    __asm__("hlt");
  }
}

/** ELF **/
// parse program header of ELF and calculate the start/end vaddr.
void CalcLoadAddressRange(Elf64_Ehdr *ehdr, UINT64 *first, UINT64 *last)
{
  Elf64_Phdr *phdr = (Elf64_Phdr*)((UINT64)ehdr + ehdr->e_phoff);
  *first = MAX_UINT64;
  *last = 0;
  for(Elf64_Half ix = 0; ix!= ehdr->e_phnum; ++ix){
    if(phdr[ix].p_type != PT_LOAD)
      continue;
    *first = MIN(*first, phdr[ix].p_vaddr);
    *last = MAX(*last, phdr[ix].p_vaddr + phdr[ix].p_memsz);
  }
}

void CopyLoadSegments(Elf64_Ehdr *ehdr)
{
  Elf64_Phdr *phdr = (Elf64_Phdr*)((UINT64)ehdr + ehdr->e_phoff);
  for(Elf64_Half ix=0; ix!=ehdr->e_phnum; ++ix){
    if(phdr[ix].p_type != PT_LOAD)
      continue;
    // load segment
    UINT64 seg = (UINT64)ehdr + phdr[ix].p_offset;
    CopyMem((VOID*)phdr[ix].p_vaddr, (VOID*)seg, phdr[ix].p_filesz);
    // clear .bss
    UINTN remain_bytes = phdr[ix].p_memsz - phdr[ix].p_filesz;
    SetMem((VOID*)(phdr[ix].p_vaddr + phdr[ix].p_filesz), remain_bytes, 0);
  }
}
/** (END ELF) **/

EFI_STATUS GetMemoryMap(struct MemoryMap* map) {
  if (map->buffer == NULL) {
    return EFI_BUFFER_TOO_SMALL;
  }

  map->map_size = map->buffer_size;
  return gBS->GetMemoryMap(
      &map->map_size,
      (EFI_MEMORY_DESCRIPTOR*)map->buffer,
      &map->map_key,
      &map->descriptor_size,
      &map->descriptor_version);
}

const CHAR16* GetMemoryTypeUnicode(EFI_MEMORY_TYPE type) {
  switch (type) {
    case EfiReservedMemoryType: return L"EfiReservedMemoryType";
    case EfiLoaderCode: return L"EfiLoaderCode";
    case EfiLoaderData: return L"EfiLoaderData";
    case EfiBootServicesCode: return L"EfiBootServicesCode";
    case EfiBootServicesData: return L"EfiBootServicesData";
    case EfiRuntimeServicesCode: return L"EfiRuntimeServicesCode";
    case EfiRuntimeServicesData: return L"EfiRuntimeServicesData";
    case EfiConventionalMemory: return L"EfiConventionalMemory";
    case EfiUnusableMemory: return L"EfiUnusableMemory";
    case EfiACPIReclaimMemory: return L"EfiACPIReclaimMemory";
    case EfiACPIMemoryNVS: return L"EfiACPIMemoryNVS";
    case EfiMemoryMappedIO: return L"EfiMemoryMappedIO";
    case EfiMemoryMappedIOPortSpace: return L"EfiMemoryMappedIOPortSpace";
    case EfiPalCode: return L"EfiPalCode";
    case EfiPersistentMemory: return L"EfiPersistentMemory";
    case EfiMaxMemoryType: return L"EfiMaxMemoryType";
    default: return L"InvalidMemoryType";
  }
}

EFI_STATUS SaveMemoryMap(struct MemoryMap* map, EFI_FILE_PROTOCOL* file) {
  CHAR8 buf[256];
  UINTN len;

  CHAR8* header =
    "Index, Type, Type(name), PhysicalStart, NumberOfPages, Attribute\n";
  len = AsciiStrLen(header);
  file->Write(file, &len, header);

  Print(L"[+] map->buffer = %08lx, map->map_size = %08lx\n",
      map->buffer, map->map_size);

  EFI_PHYSICAL_ADDRESS iter;
  int i;
  for (iter = (EFI_PHYSICAL_ADDRESS)map->buffer, i = 0;
       iter < (EFI_PHYSICAL_ADDRESS)map->buffer + map->map_size;
       iter += map->descriptor_size, i++) {
    EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)iter;
    len = AsciiSPrint(
        buf, sizeof(buf),
        "%u, %x, %-ls, %08lx, %lx, %lx\n",
        i, desc->Type, GetMemoryTypeUnicode(desc->Type),
        desc->PhysicalStart, desc->NumberOfPages,
        desc->Attribute & 0xffffflu);
    file->Write(file, &len, buf);
  }

  return EFI_SUCCESS;
}

EFI_STATUS OpenRootDir(EFI_HANDLE image_handle, EFI_FILE_PROTOCOL** root) {
  EFI_LOADED_IMAGE_PROTOCOL* loaded_image;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs;

  gBS->OpenProtocol(
      image_handle,
      &gEfiLoadedImageProtocolGuid,
      (VOID**)&loaded_image,
      image_handle,
      NULL,
      EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

  gBS->OpenProtocol(
      loaded_image->DeviceHandle,
      &gEfiSimpleFileSystemProtocolGuid,
      (VOID**)&fs,
      image_handle,
      NULL,
      EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

  fs->OpenVolume(fs, root);

  return EFI_SUCCESS;
}

EFI_STATUS OpenGOP(EFI_HANDLE image_handle, EFI_GRAPHICS_OUTPUT_PROTOCOL **gop) {
  UINTN num_gop_handles = 0;
  EFI_HANDLE *gop_handles = NULL;
  gBS->LocateHandleBuffer(
      ByProtocol,
      &gEfiGraphicsOutputProtocolGuid,
      NULL,
      &num_gop_handles,
      &gop_handles);

  gBS->OpenProtocol(
      gop_handles[0],
      &gEfiGraphicsOutputProtocolGuid,
      (VOID**)gop,
      image_handle,
      NULL,
      EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

  FreePool(gop_handles);

  return EFI_SUCCESS;
}

const CHAR16* GetPixelFormatUnicode(EFI_GRAPHICS_PIXEL_FORMAT fmt) {
  switch (fmt) {
    case PixelRedGreenBlueReserved8BitPerColor:
      return L"PixelRedGreenBlueReserved8BitPerColor";
    case PixelBlueGreenRedReserved8BitPerColor:
      return L"PixelBlueGreenRedReserved8BitPerColor";
    case PixelBitMask:
      return L"PixelBitMask";
    case PixelBltOnly:
      return L"PixelBltOnly";
    case PixelFormatMax:
      return L"PixelFormatMax";
    default:
      return L"InvalidPixelFormat";
  }
}

EFI_STATUS EFIAPI UefiMain(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE* system_table) 
{
  EFI_STATUS status;

  // ** bootloader banner 
  Print(L"Hello, Mikan World!\n");

  // ** read memory map from UEFI API
  CHAR8 memmap_buf[4*PAGE];
  struct MemoryMap memmap = {sizeof(memmap_buf), memmap_buf, 0, 0, 0, 0};
  GetMemoryMap(&memmap);

  EFI_FILE_PROTOCOL* root_dir;
  OpenRootDir(image_handle, &root_dir);

  EFI_FILE_PROTOCOL* memmap_file;
  root_dir->Open(
      root_dir, &memmap_file, L"\\memmap",
      EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);

  SaveMemoryMap(&memmap, memmap_file);
  memmap_file->Close(memmap_file);

  // ** get frame-buffer
  EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
  OpenGOP(image_handle, &gop);
  Print(L"[+] Resolution: %ux%u, Pixel Fromat: %s, %u pixcels/line\n",
    gop->Mode->Info->HorizontalResolution,
    gop->Mode->Info->VerticalResolution,
    GetPixelFormatUnicode(gop->Mode->Info->PixelFormat),
    gop->Mode->Info->PixelsPerScanLine
  );
  Print(L"[+] Frame Buffer: 0x%0lx - 0x%0lx, Size: 0x%lx bytes\n",
      gop->Mode->FrameBufferBase,
      gop->Mode->FrameBufferBase + gop->Mode->FrameBufferSize,
      gop->Mode->FrameBufferSize);
  
  // ** load kernel
  EFI_FILE_PROTOCOL *kernel_file;
  root_dir->Open(root_dir, &kernel_file, L"\\kernel.elf", EFI_FILE_MODE_READ, 0);

  // read kernel file size
  UINTN file_info_size = sizeof(EFI_FILE_INFO) + sizeof(CHAR16) * 12; // sizeof(char)*12 is for filename buffer
  UINT8 file_info_buffer[file_info_size];
  kernel_file->GetInfo(kernel_file, &gEfiFileInfoGuid, &file_info_size, file_info_buffer);
  EFI_FILE_INFO *file_info = (EFI_FILE_INFO*)file_info_buffer;
  UINTN kernel_file_size = file_info->FileSize;

  // read kernel image temporaly
  VOID *kernel_buffer;  // tmp buffer for kernel ELF
  status = gBS->AllocatePool(EfiLoaderData, kernel_file_size, &kernel_buffer);
  if(EFI_ERROR(status)){
    Print(L"[!] failed to allocate pool: %r\n", status);
    Halt();
  }
  status = kernel_file->Read(kernel_file, &kernel_file_size, (VOID*)kernel_buffer);
  if(EFI_ERROR(status)){
    Print(L"[!] failed to load temp kernel image: %r\n", status);
    Halt();
  }
  Print(L"[+] Kernel(tmp): 0x%0lx (%lu bytes)\n", kernel_buffer, kernel_file_size);

  // parse kernel ELF
  Elf64_Ehdr *kernel_ehdr = (Elf64_Ehdr*)kernel_buffer;
  UINT64 kernel_first_addr, kernel_last_addr;
  CalcLoadAddressRange(kernel_ehdr, &kernel_first_addr, &kernel_last_addr);
  UINTN num_pages = (kernel_last_addr - kernel_first_addr + PAGE - 1) / PAGE;

  // allocate actual page to load kernel and copy load segments
  status = gBS->AllocatePages(AllocateAddress, EfiLoaderData, num_pages, &kernel_first_addr);
  if(EFI_ERROR(status)){
    Print(L"[!] failed to allocate pages: %r\n", status);
    Halt();
  }
  CopyLoadSegments(kernel_ehdr);
  Print(L"[+] Kernel: 0x%0lx - 0x%0lx\n", kernel_first_addr, kernel_last_addr);

  // free tmp page
  status = gBS->FreePool(kernel_buffer);
  if(EFI_ERROR(status)){
    Print(L"failed to free pool: %r\n", status);
    Halt();
  }

  // ** exit boot service
  //Print(L"[.] Exiting Boot Services...\n");
  status = gBS->ExitBootServices(image_handle, memmap.map_key);
  //Print(L"[.] Checking exit status...\n");
  if(EFI_ERROR(status)){
    //Print(L"[!] First check is error(%r). Re-getting memmap...\n", status);
    status = GetMemoryMap(&memmap);
    if(EFI_ERROR(status)){
      Print(L"[!] failed to get memory map: %r\n", status);
      Halt();
    }
    //Print(L"[.] Retrying to exit Boot Services...\n");
    status = gBS->ExitBootServices(image_handle, memmap.map_key);
    if(EFI_ERROR(status)){
      Print(L"[!] Couldn't exit boot service: %r\n", status);
      Halt();
    }
  }

  // ** get Frame Buffer information
  //Print(L"[!] Fetching frame buffer information...\n");
  struct FrameBufferConfig config = {
    .frame_buffer = (UINT8*)gop->Mode->FrameBufferBase,
    .pixels_per_scan_line = gop->Mode->Info->PixelsPerScanLine,
    .horizontal_resolution = gop->Mode->Info->HorizontalResolution,
    .vertical_resolution = gop->Mode->Info->VerticalResolution,
    0
  };
  switch(gop->Mode->Info->PixelFormat){
    case PixelRedGreenBlueReserved8BitPerColor:
      config.pixel_format = kPixelBGRResv8BitPerColor;
      break;
    case PixelBlueGreenRedReserved8BitPerColor:
      config.pixel_format = kPixelBGRResv8BitPerColor;
      break;
    default:
      Print(L"[!] Unimplemented pixel format: %d\n", gop->Mode->Info->PixelFormat);
      Halt();
  }

  // ** boot kernel
  //Print(L"[.] Kernel is booting...\n"); // [guess] printing something would change memmap and invoke error.
  UINT64 entry_addr = *(UINT64*)(kernel_first_addr + 0x18); // 0x18 is offset inside ELF header, where addr of entry-point is written
  typedef void EntryPointType(const struct FrameBufferConfig*, const struct MemoryMap*);  
  EntryPointType *entry_point = (EntryPointType*)entry_addr; // cast addr of entrypoint into func pointer
  entry_point(&config, &memmap);


  Print(L"[.] All done\n");

  Halt();
  return EFI_SUCCESS;
}
