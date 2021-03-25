SHELL = /bin/bash
BASEDIR = $(CURDIR)/../devenv/x86_64-elf

TARGET = kernel.elf
OBJS = main.o graphics.o font.o hankaku.o
DEPENDS = $(join $(dir $(OBJS)), $(addprefix ., $(notdir $(OBJS:.o=.d))))

CXXFLAGS = -O2 -Wall -g --target=x86_64-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++17 
INCLUDES = -I$(BASEDIR)/include/c++/v1 -I$(BASEDIR)/include -I$(BASEDIR)/include/freetype2 -nostdlibinc -D__ELF__ -D_LDBL_EQ_DBL -D_GNU_SOURCE -D_POSIX_TIMERS
LDFLAGS = -L$(BASEDIR)/lib --entry KernelMain -z norelro --image-base 0x100000 --static

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
	ld.lld $(LDFLAGS) -o $@ $(OBJS)

%.o: %.cpp Makefile
	clang++ $(INCLUDES) $(CPPFLAGS) $(CXXFLAGS) -c $<

.%.d: %.cpp
	clang++ $(CPPFLAGS) $(CXXFLAGS) -MM $< > $@
	$(eval OBJ = $(<:.cpp=.o))
	sed --in-place 's|$(notdir $(OBJ))|$(OBJ)' $@

hankaku.bin: hankaku.txt
	../tools/makefont.py -o $@ $<

hankaku.o: hankaku.bin
	objcopy -I binary -O elf64-x86-64 -B i386:x86-64 $< $@

.%.d: %.bin
	touch $@

-include $(DEPENDS)