all: hello.o
	clang -target x86_64-pc-win32-coff -mno-red-zone -fno-stack-protector -fshort-wchar -Wall -c hello.c
	llvm-link /subsystem:efi_application /entry:EfiMain /out:hello.efi hello.o

clean:
	rm -f ./hello.o
	rm -f ./hello.efi