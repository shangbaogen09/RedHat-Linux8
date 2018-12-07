# SPDX-License-Identifier: GPL-2.0
#
# linux/arch/x86/boot/compressed/Makefile
#
# create a compressed vmlinux image from the original vmlinux
#
# vmlinuz is:
#	decompression code (*.o)
#	asm globals (piggy.S), including:
#		vmlinux.bin.(gz|bz2|lzma|...)
#
# vmlinux.bin is:
#	vmlinux stripped of debugging and comments
# vmlinux.bin.all is:
#	vmlinux.bin + vmlinux.relocs
# vmlinux.bin.(gz|bz2|lzma|...) is:
#	(see scripts/Makefile.lib size_append)
#	compressed vmlinux.bin.all + u32 size of vmlinux.bin.all

KASAN_SANITIZE			:= n
OBJECT_FILES_NON_STANDARD	:= y

# Prevents link failures: __sanitizer_cov_trace_pc() is not linked in.
KCOV_INSTRUMENT		:= n

targets := vmlinux vmlinux.bin vmlinux.bin.gz vmlinux.bin.bz2 vmlinux.bin.lzma \
	vmlinux.bin.xz vmlinux.bin.lzo vmlinux.bin.lz4

KBUILD_CFLAGS := -m$(BITS) -O2
KBUILD_CFLAGS += -fno-strict-aliasing $(call cc-option, -fPIE, -fPIC)
KBUILD_CFLAGS += -DDISABLE_BRANCH_PROFILING
cflags-$(CONFIG_X86_32) := -march=i386
cflags-$(CONFIG_X86_64) := -mcmodel=small
KBUILD_CFLAGS += $(cflags-y)
KBUILD_CFLAGS += -mno-mmx -mno-sse
KBUILD_CFLAGS += $(call cc-option,-ffreestanding)
KBUILD_CFLAGS += $(call cc-option,-fno-stack-protector)
KBUILD_CFLAGS += $(call cc-disable-warning, address-of-packed-member)
KBUILD_CFLAGS += $(call cc-disable-warning, gnu)

KBUILD_AFLAGS  := $(KBUILD_CFLAGS) -D__ASSEMBLY__
GCOV_PROFILE := n
UBSAN_SANITIZE :=n

LDFLAGS := -m elf_$(UTS_MACHINE)
# Compressed kernel should be built as PIE since it may be loaded at any
# address by the bootloader.
ifeq ($(CONFIG_X86_32),y)
LDFLAGS += $(call ld-option, -pie) $(call ld-option, --no-dynamic-linker)
else
# To build 64-bit compressed kernel as PIE, we disable relocation
# overflow check to avoid relocation overflow error with a new linker
# command-line option, -z noreloc-overflow.
LDFLAGS += $(shell $(LD) --help 2>&1 | grep -q "\-z noreloc-overflow" \
	&& echo "-z noreloc-overflow -pie --no-dynamic-linker")
endif
LDFLAGS_vmlinux := -T

hostprogs-y	:= mkpiggy
HOST_EXTRACFLAGS += -I$(srctree)/tools/include

sed-voffset := -e 's/^\([0-9a-fA-F]*\) [ABCDGRSTVW] \(_text\|__bss_start\|_end\)$$/\#define VO_\2 _AC(0x\1,UL)/p'

quiet_cmd_voffset = VOFFSET $@
      cmd_voffset = $(NM) $< | sed -n $(sed-voffset) > $@

targets += ../voffset.h

$(obj)/../voffset.h: vmlinux FORCE
	$(call if_changed,voffset)

$(obj)/misc.o: $(obj)/../voffset.h

#链接vmlinux依赖的文件
vmlinux-objs-y := $(obj)/vmlinux.lds $(obj)/head_$(BITS).o $(obj)/misc.o \
	$(obj)/string.o $(obj)/cmdline.o $(obj)/error.o \
	$(obj)/piggy.o $(obj)/cpuflags.o

vmlinux-objs-$(CONFIG_EARLY_PRINTK) += $(obj)/early_serial_console.o
vmlinux-objs-$(CONFIG_RANDOMIZE_BASE) += $(obj)/kaslr.o
ifdef CONFIG_X86_64
	vmlinux-objs-$(CONFIG_RANDOMIZE_BASE) += $(obj)/kaslr_64.o
	vmlinux-objs-y += $(obj)/mem_encrypt.o
	vmlinux-objs-y += $(obj)/pgtable_64.o
endif

$(obj)/eboot.o: KBUILD_CFLAGS += -fshort-wchar -mno-red-zone

vmlinux-objs-$(CONFIG_EFI_STUB) += $(obj)/eboot.o $(obj)/efi_stub_$(BITS).o \
	$(objtree)/drivers/firmware/efi/libstub/lib.a
vmlinux-objs-$(CONFIG_EFI_MIXED) += $(obj)/efi_thunk_$(BITS).o

# The compressed kernel is built with -fPIC/-fPIE so that a boot loader
# can place it anywhere in memory and it will still run. However, since
# it is executed as-is without any ELF relocation processing performed
# (and has already had all relocation sections stripped from the binary),
# none of the code can use data relocations (e.g. static assignments of
# pointer values), since they will be meaningless at runtime. This check
# will refuse to link the vmlinux if any of these relocations are found.
quiet_cmd_check_data_rel = DATAREL $@
define cmd_check_data_rel
	for obj in $(filter %.o,$^); do \
		${CROSS_COMPILE}readelf -S $$obj | grep -qF .rel.local && { \
			echo "error: $$obj has data relocations!" >&2; \
			exit 1; \
		} || true; \
	done
endef

# We need to run two commands under "if_changed", so merge them into a
# single invocation.
quiet_cmd_check-and-link-vmlinux = LD      $@
      cmd_check-and-link-vmlinux = $(cmd_check_data_rel); $(cmd_ld)

#调用连接器ld生成当前目录下的arch/x86/boot/compressed/vmlinux,不同与顶层的vmlinux
$(obj)/vmlinux: $(vmlinux-objs-y) FORCE
	$(call if_changed,check-and-link-vmlinux)

#把顶层生成的vmlinux,使用objcopy处理后生成arch/x86/boot/compressed/vmlinux.bin
OBJCOPYFLAGS_vmlinux.bin :=  -R .comment -S
$(obj)/vmlinux.bin: vmlinux FORCE
	$(call if_changed,objcopy)

targets += $(patsubst $(obj)/%,%,$(vmlinux-objs-y)) vmlinux.bin.all vmlinux.relocs

#使用arch/x86/tools/relocs工具从vmlinux中提取重定位信息到文件vmlinux.relocs中
CMD_RELOCS = arch/x86/tools/relocs
quiet_cmd_relocs = RELOCS  $@
      cmd_relocs = $(CMD_RELOCS) $< > $@;$(CMD_RELOCS) --abs-relocs $<
$(obj)/vmlinux.relocs: vmlinux FORCE
	$(call if_changed,relocs)

#再次处理vmlinux.bin,如果配置为可重定位,则追加重定位表到vmlinux.bin的末尾
vmlinux.bin.all-y := $(obj)/vmlinux.bin
vmlinux.bin.all-$(CONFIG_X86_NEED_RELOCS) += $(obj)/vmlinux.relocs

#根据不同的压缩选项,调用对应的压缩函数进行处理
$(obj)/vmlinux.bin.gz: $(vmlinux.bin.all-y) FORCE
	$(call if_changed,gzip)
$(obj)/vmlinux.bin.bz2: $(vmlinux.bin.all-y) FORCE
	$(call if_changed,bzip2)
$(obj)/vmlinux.bin.lzma: $(vmlinux.bin.all-y) FORCE
	$(call if_changed,lzma)
$(obj)/vmlinux.bin.xz: $(vmlinux.bin.all-y) FORCE
	$(call if_changed,xzkern)
$(obj)/vmlinux.bin.lzo: $(vmlinux.bin.all-y) FORCE
	$(call if_changed,lzo)
$(obj)/vmlinux.bin.lz4: $(vmlinux.bin.all-y) FORCE
	$(call if_changed,lz4)

#根据内核配置的压缩选项,设置对应的压缩后缀,默认为CONFIG_KERNEL_GZIP=y
suffix-$(CONFIG_KERNEL_GZIP)	:= gz
suffix-$(CONFIG_KERNEL_BZIP2)	:= bz2
suffix-$(CONFIG_KERNEL_LZMA)	:= lzma
suffix-$(CONFIG_KERNEL_XZ)	:= xz
suffix-$(CONFIG_KERNEL_LZO) 	:= lzo
suffix-$(CONFIG_KERNEL_LZ4) 	:= lz4

#展开后为:cmd_mkpiggy = arch/x86/boot/compressed/mkpiggy arch/x86/boot/compressed/vmlinux.bin.gz > arch/x86/boot/compressed/piggy.S
quiet_cmd_mkpiggy = MKPIGGY $@
      cmd_mkpiggy = $(obj)/mkpiggy $< > $@ || ( rm -f $@ ; false )

targets += piggy.S

#生成的piggy.S文件如下:
#.section ".rodata..compressed","a",@progbits
#.globl z_input_len
#z_input_len = 8096170
#.globl z_output_len
#z_output_len = 30546500
#.globl input_data, input_data_end
#input_data:
#.incbin "arch/x86/boot/compressed/vmlinux.bin.gz"
#input_data_end:

#使用mkpiggy工具对vmlinux.bin.gz进行处理,生成piggy.S文件,然后汇编为piggy.o
$(obj)/piggy.S: $(obj)/vmlinux.bin.$(suffix-y) $(obj)/mkpiggy FORCE
	$(call if_changed,mkpiggy)
