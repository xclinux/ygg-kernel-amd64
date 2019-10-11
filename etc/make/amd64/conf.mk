include etc/make/amd64/compiler.mk
include etc/make/amd64/platform.mk
include etc/make/amd64/config.mk

.PHONY+=$(O)/sys/amd64/hw/ap_code_blob.o

all:

# add .inc includes for asm
HEADERS+=$(shell find include -name "*.inc")

$(O)/sys/amd64/kernel.elf: $(kernel_OBJS) $(kernel_LINKER) config
	@printf " LD\t%s\n" $(@:$(O)/%=%)
	@$(CC64) $(kernel_LDFLAGS)  -o $@ $(kernel_OBJS)

$(O)/sys/%.o: sys/%.S $(HEADERS) config
	@printf " AS\t%s\n" $(@:$(O)/%=%)
	@$(CC64) $(kernel_CFLAGS) -D__ASM__ -c -o $@ $<

$(O)/sys/%.o: sys/%.c $(HEADERS) config
	@printf " CC\t%s\n" $(@:$(O)/%=%)
	@$(CC64) $(kernel_CFLAGS) -c -o $@ $<

$(O)/sys/amd64/hw/ap_code_blob.o: $(O)/sys/amd64/hw/ap_code.bin config
	@printf " AS\t%s\n" $(@:$(O)/%=%)
	@$(CC64) $(kernel_CFLAGS) -c -DAP_CODE_BIN='"$<"' -o $@ sys/amd64/hw/ap_code_blob.S

$(O)/sys/amd64/hw/ap_code.bin: sys/amd64/hw/ap_code.nasm config
	@printf " NASM\t%s\n" $(@:$(O)/%=%)
	@nasm -f bin -o $@ $<

### Kernel loader build
TARGETS+=$(O)/sys/amd64/image.iso
DIRS+=$(O)/sys/amd64/loader
loader_OBJS+=$(O)/sys/amd64/loader/boot.o \
			 $(O)/sys/amd64/loader/loader.o \
			 $(O)/sys/amd64/loader/util.o
loader_LINKER=sys/amd64/loader/link.ld
loader_CFLAGS=-ffreestanding \
			  -nostdlib \
			  -I. \
			  -Iinclude \
			  -Wall \
			  -Wextra \
			  -Wpedantic \
			  -Wno-unused-argument \
			  -Werror \
			  -Wno-language-extension-token
loader_LDFLAGS=-nostdlib -T$(loader_LINKER)

$(O)/sys/amd64/loader.elf: $(loader_OBJS) $(loader_LINKER) config
	@printf " LD\t%s\n" $(@:$(O)/%=%)
	@$(CC86) $(loader_LDFLAGS) -o $@ $(loader_OBJS)

$(O)/sys/amd64/loader/%.o: sys/amd64/loader/%.S $(HEADERS) config
	@printf " AS\t%s\n" $(@:$(O)/%=%)
	@$(CC86) $(loader_CFLAGS) -D__ASM__ -c -o $@ $<

$(O)/sys/amd64/loader/%.o: sys/amd64/loader/%.c $(HEADERS) config
	@printf " CC\t%s\n" $(@:$(O)/%=%)
	@$(CC86) $(loader_CFLAGS) -c -o $@ $<

### Initrd building
amd64_mkstage:
	@rm -rf $(O)/sys/amd64/stage
	@mkdir -p $(O)/sys/amd64/stage
	@cp -r usr/etc $(O)/sys/amd64/stage/etc

$(O)/sys/amd64/initrd.img: amd64_mkstage
	@cd $(O)/sys/amd64/stage && tar czf $@ *
	@du -sh $@

### Debugging and emulation
QEMU_BIN?=qemu-system-x86_64
QEMU_OPTS?=-serial mon:stdio \
		   -m $(QEMU_MEM) \
		   --accel tcg,thread=multi \
		   -smp $(QEMU_SMP)

$(O)/sys/amd64/image.iso: $(O)/sys/amd64/kernel.elf \
						  $(O)/sys/amd64/loader.elf \
						  $(O)/sys/amd64/initrd.img \
						  sys/amd64/grub.cfg
	@printf " ISO\t%s\n" $(@:$(O)/%=%)
	@cp sys/amd64/grub.cfg $(O)/sys/amd64/image/boot/grub/grub.cfg
	@cp $(O)/sys/amd64/kernel.elf $(O)/sys/amd64/image/boot/kernel
	@cp $(O)/sys/amd64/loader.elf $(O)/sys/amd64/image/boot/loader
	@cp $(O)/sys/amd64/initrd.img $(O)/sys/amd64/image/boot/initrd.img
	@grub-mkrescue -o $(O)/sys/amd64/image.iso $(O)/sys/amd64/image 2>/dev/null

qemu: all
	@$(QEMU_BIN) \
		-cdrom $(O)/sys/amd64/image.iso \
		$(QEMU_OPTS)