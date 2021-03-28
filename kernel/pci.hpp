#pragma once

#include<cstdint>
#include<array>
#include"error.hpp"

namespace pci{
  const uint16_t kConfigAddress = 0x0CF8;   // CONFIG_ADDRESS register's IO port addr
  const uint16_t kConfigData    = 0x0CFC;   // CONFIG_DATA register's IO port addr

  void WriteAddress(uint32_t address);      // write @address into CONFIG_ADDRESS addr
  void WriteData(uint32_t value);           // write @value into CONFIG_DATA addr
  uint32_t ReadData();                      // read from CONFIG_DATA addr

  uint16_t ReadVendorId(uint8_t bus, uint8_t device, uint8_t function);
  uint16_t ReadDeviceId(uint8_t bus, uint8_t device, uint8_t function);
  uint8_t ReadHeaderType(uint8_t bus, uint8_t device, uint8_t function);
  // @ret: 32:24=base class, 23:16=sub class, 15:8=interface, 7:0=revision
  uint32_t ReadClassCode(uint8_t bus, uint8_t device, uint8_t function);
  // @ret: 23:16: sub ordinate bus, 15:8=secondary bus, 7:0=revision
  uint32_t ReadBusNumbers(uint8_t bus, uint8_t device, uint8_t function);

  bool IsSingleFunctionDevice(uint8_t header_type);

  struct Device{
    uint8_t bus, device, function, header_type;
  };

  inline std::array<Device, 32> devices;    // all the found PIC devices
  inline int num_device;                    // num of effective PIC devices

  Error ScanAllBus();
}