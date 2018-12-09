/*
 * setup.ld
 *
 * Linker script for the i386 setup code
 */
OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(_start)

/*下面的内存layout来自于内核文档x86/boot.txt
         ~                        ~
        |  Protected-mode kernel |
100000  +------------------------+
        |  I/O memory hole       |
0A0000  +------------------------+
        |  Reserved for BIOS     |      Leave as much as possible unused
        ~                        ~
        |  Command line          |      (Can also be below the X+10000 mark)
X+10000 +------------------------+
        |  Stack/heap            |      For use by the kernel real-mode code.
X+08000 +------------------------+
        |  Kernel setup          |      The kernel real-mode code.
        |  Kernel boot sector    |      The kernel legacy boot sector.
X       +------------------------+
        |  Boot loader           |      <- Boot sector entry point 0000:7C00
001000  +------------------------+
        |  Reserved for MBR/BIOS |
000800  +------------------------+
        |  Typically used by MBR |
000600  +------------------------+
        |  BIOS use only         |
000000  +------------------------+
*/

SECTIONS
{
	/*arch/x86/boot/header.s文件，编译的地址是以0为基址*/
	. = 0;
	.bstext		: { *(.bstext) }
	.bsdata		: { *(.bsdata) }

	/*arch/x86/boot/header.s文件中,header段偏移地址为495*/
	. = 495;
	.header		: { *(.header) }
	.entrytext	: { *(.entrytext) }
	.inittext	: { *(.inittext) }
	.initdata	: { *(.initdata) }
	__end_init = .;

	.text		: { *(.text) }
	.text32		: { *(.text32) }

	. = ALIGN(16);
	.rodata		: { *(.rodata*) }

	.videocards	: {
		video_cards = .;
		*(.videocards)
		video_cards_end = .;
	}

	. = ALIGN(16);
	.data		: { *(.data*) }

	.signature	: {
		setup_sig = .;
		LONG(0x5a5aaa55)
	}


	/*bss段的链接过程*/
	. = ALIGN(16);
	.bss		:
	{
		__bss_start = .;
		*(.bss)
		__bss_end = .;
	}
	/*setup.bin文件的结尾*/
	. = ALIGN(16);
	_end = .;

	/DISCARD/ : { *(.note*) }

	/*
	 * The ASSERT() sink to . is intentional, for binutils 2.14 compatibility:
	 */
	. = ASSERT(_end <= 0x8000, "Setup too big!");
	. = ASSERT(hdr == 0x1f1, "The setup header has the wrong offset!");
	/* Necessary for the very-old-loader check to work... */
	. = ASSERT(__end_init <= 5*512, "init sections too big!");

}
