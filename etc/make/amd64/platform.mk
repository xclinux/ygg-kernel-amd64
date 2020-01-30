### This file contains all things for a generic amd64-build

### Kernel build
DEFINES+=-DARCH_AMD64

ACPICA_SRCS=$(shell find sys/amd64/acpica -type f -name "*.c")
ACPICA_OBJS=$(ACPICA_SRCS:%.c=$(O)/%.o)
ACPICA_SRCD=$(shell find sys/amd64/acpica -type d)
ACPICA_OBJD=$(ACPICA_SRCD:%=$(O)/%)

OBJS+=$(O)/sys/amd64/hw/rs232.o \
	  $(O)/sys/amd64/kernel.o \
	  $(O)/sys/amd64/hw/gdt.o \
	  $(O)/sys/amd64/hw/gdt_s.o \
	  $(O)/sys/amd64/hw/acpi.o \
	  $(O)/sys/amd64/mm/mm.o \
	  $(O)/sys/amd64/hw/apic.o \
	  $(O)/sys/amd64/hw/idt.o \
	  $(O)/sys/amd64/hw/exc_s.o \
	  $(O)/sys/amd64/hw/exc.o \
	  $(O)/sys/amd64/hw/irq0.o \
	  $(O)/sys/amd64/hw/timer.o \
	  $(O)/sys/amd64/hw/ioapic.o \
	  $(O)/sys/amd64/hw/irqs_s.o \
	  $(O)/sys/amd64/sys/spin_s.o \
	  $(O)/sys/amd64/cpu.o \
	  $(O)/sys/amd64/mm/heap.o \
	  $(O)/sys/amd64/mm/map.o \
	  $(O)/sys/amd64/mm/phys.o \
	  $(O)/sys/amd64/mm/vmalloc.o \
	  $(O)/sys/amd64/mm/pool.o \
	  $(O)/sys/amd64/hw/ps2.o \
	  $(O)/sys/amd64/hw/irq.o \
	  $(ACPICA_OBJS) \
	  $(O)/sys/amd64/acpi_osl_mem.o \
	  $(O)/sys/amd64/acpi_osl_printf.o \
	  $(O)/sys/amd64/acpi_osl_thread.o \
	  $(O)/sys/amd64/acpi_osl_init.o \
	  $(O)/sys/amd64/acpi_osl_table.o \
	  $(O)/sys/amd64/acpi_osl_irq.o \
	  $(O)/sys/amd64/acpi_osl_hw.o \
	  $(O)/sys/amd64/hw/rtc.o \
	  $(O)/sys/amd64/fpu.o \
	  $(O)/sys/amd64/cpuid.o \
	  $(O)/sys/amd64/sched_s.o \
	  $(O)/sys/amd64/syscall_s.o \
	  $(O)/sys/amd64/syscall.o

kernel_LINKER=sys/amd64/link.ld
kernel_LDFLAGS=-nostdlib \
			   -fPIE \
			   -fno-plt \
			   -fno-pic \
			   -static \
			   -Wl,--build-id=none \
			   -z max-page-size=0x1000 \
			   -ggdb \
			   -T$(kernel_LINKER)
kernel_CFLAGS=-ffreestanding \
			  -I. \
			  -I include/sys/amd64/acpica \
			  $(DEFINES) \
			  $(CFLAGS) \
			  -nostdlib \
			  -fPIE \
			  -fno-plt \
			  -fno-pic \
			  -static \
			  -fno-asynchronous-unwind-tables \
			  -Wno-format \
			  -Wno-comment \
			  -Wno-unused-but-set-variable \
			  -mcmodel=large \
			  -mno-red-zone \
			  -mno-mmx \
			  -mno-sse \
			  -mno-sse2 \
			  -z max-page-size=0x1000 \
			  -D__KERNEL__

kernel_OBJS=$(O)/sys/amd64/crti.o \
			$(O)/sys/amd64/entry.o \
			$(shell $(CC64) $(CFLAGS) -print-file-name=crtbegin.o) \
			$(OBJS) \
			$(shell $(CC64) $(CFLAGS) -print-file-name=crtend.o) \
			$(O)/sys/amd64/crtn.o

DIRS+=$(O)/sys/amd64/image/boot/grub \
	  $(O)/sys/amd64/hw \
	  $(O)/sys/amd64/sys \
	  $(O)/sys/amd64/mm \
	  $(ACPICA_OBJD)
