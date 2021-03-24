SHELL = /bin/bash
BASEDIR = $(CURDIR)/../devenv/x86_64-elf

TARGET = kernel.elf
OBJS = main.o

CXXFLAGS = -O2 -Wall -g --target=x86_64-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++17 
INCLUDES = -I$(BASEDIR)/include/c++/v1 -I$(BASEDIR)/include -I$(BASEDIR)/include/freetype2 -nostdlibinc -D__ELF__ -D_LDBL_EQ_DBL -D_GNU_SOURCE -D_POSIX_TIMERS
LDFLAGS = -L$(BASEDIR)/lib --entry KernelMain -z norelro --image-base 0x100000 --static

.PHONY: all clean
all: $(TARGET)
clean:
	rm -rf *.o

kernel.elf: $(OBJS) Makefile
	ld.lld $(LDFLAGS) -o $@ $<

%.o: %.cpp Makefile
	clang++ $(INCLUDES) $(CXXFLAGS) -c $<