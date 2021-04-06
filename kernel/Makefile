SHELL = /bin/bash

TARGET = kernel.elf
OBJS = main.o graphics.o font.o hankaku.o console.o pci.o asmfunc.o logger.o mouse.o interrupt.o segment.o  paging.o memory_manager.o window.o layer.o \
			libcxx_support.o newlib_support.o \
      usb/memory.o usb/device.o usb/xhci/ring.o usb/xhci/trb.o usb/xhci/xhci.o \
      usb/xhci/port.o usb/xhci/device.o usb/xhci/devmgr.o usb/xhci/registers.o \
      usb/classdriver/base.o usb/classdriver/hid.o usb/classdriver/keyboard.o \
      usb/classdriver/mouse.o
DEPENDS = $(join $(dir $(OBJS)),$(addprefix .,$(notdir $(OBJS:.o=.d))))

CXXDEFINES := __ELF__ _LDBL_EQ_DBL _GNU_SOURCE _POSIX_TIMERS

CPPFLAGS += -I.
CFLAGS   += -O2 -Wall -g --target=x86_64-elf -ffreestanding -mno-red-zone
CXXFLAGS += -O2 -Wall -g --target=x86_64-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++17  -nostdlibinc $(addprefix -D,$(CXXDEFINES))
LDFLAGS += --entry KernelMain -z norelro --image-base 0x100000 --static

.PHONY: all clean depends
all: $(TARGET)
clean:
	rm -rf *.o
	rm -rf .*.d
	rm -rf *.bin
	rm -rf *.elf
depends:
	$(MAKE) $(DEPENDS)

kernel.elf: $(OBJS) Makefile
	ld.lld $(LDFLAGS) -o $@ $(OBJS) -lc -lc++ -lc++abi

%.o: %.cpp Makefile
	clang++ $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

%.o: %.c Makefile
	clang  $(CPPFLAGS) $(CFLAGS) -c $< -o $@

%.o: %.asm Makefile
	nasm -f elf64 -o $@ $<

.%.d: %.cpp
	clang++ $(CPPFLAGS) $(CXXFLAGS) -MM $< > $@
	$(eval OBJ = $(<:.cpp=.o))
	sed --in-place 's|$(notdir $(OBJ))|$(OBJ)|' $@

.%.d: %.c
	clang $(CPPFLAGS) $(CFLAGS) -MM $< > $@
	$(eval OBJ = $(<:.c=.o))
	sed --in-place 's|$(notdir $(OBJ))|$(OBJ)|' $@

hankaku.bin: hankaku.txt
	../tools/makefont.py -o $@ $<

hankaku.o: hankaku.bin
	objcopy -I binary -O elf64-x86-64 -B i386:x86-64 $< $@

.%.d: %.bin
	touch $@

-include $(DEPENDS)
