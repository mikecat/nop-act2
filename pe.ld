OUTPUT_FORMAT("binary");
OUTPUT_ARCH("i386");

image_base = 0x00400000;
section_align = 0x1000;
file_align = 0x200;
subsystem = 0x0a;

SECTIONS
{
	.IMAGE_DOS_HEADER image_base : AT(0)
	{
		SHORT(0x5a4d) /* e_magic */
		FILL(0x00000000)
		. += 2 * 11;
		SHORT(0x0040) /* e_lfarlc (hack for TSXBIN) */
		SHORT(0)
		. += 2 * 4;
		SHORT(0)
		SHORT(0)
		. += 2 * 10;
		LONG(LOADADDR(.IMAGE_NT_HEADERS)) /* e_lfanew */
	}

	.IMAGE_NT_HEADERS :
	{
		LONG(0x00004550) /* Signature */
		/* IMAGE_FILE_HEADER */
		SHORT(0x014c) /* Machine */
		SHORT(sections_count) /* NumberOfSections */
		LONG(0) /* TimeDataStamp */
		LONG(0) /* PointerToSymbolTable */
		LONG(0) /* NumberOfSymbols */
		SHORT(_optional_header_end - _optional_header_start) /* SizeOfOptionalHeader */
		SHORT(0x0103) /* Characteristics */
		/* IMAGE_OPTIONAL_HEADER32 */
		_optional_header_start = . ;
		SHORT(0x010b) /* Magic */
		BYTE(0) BYTE(0) /*(Major|Minor)LinkerVersion */
		LONG(SIZEOF(.text)) /* SizeOfCode */
		LONG(SIZEOF(.data) + SIZEOF(.rodata)) /* SizeOfInitializedData */
		LONG(SIZEOF(.bss)) /* SizeOfUninitializedData */
		LONG(_start - image_base) /* AddressOfEntryPoint */
		LONG(ADDR(.text) - image_base) /* BaseOfCode */
		LONG(ADDR(.bss) - image_base) /* BaseOfData */
		LONG(image_base) /* ImageBase */
		LONG(section_align) /* SectionAlignment */
		LONG(file_align) /* FileAlignment */
		SHORT(4) SHORT(0) /* (Major|Minor)OperatingSystemVersion */
		SHORT(1) SHORT(0) /* (Major|Minor)ImageVersion */
		SHORT(4) SHORT(0) /* (Major|Minor)SubsystemVersion */
		LONG(0) /* Win32VersionValue */
		LONG(ALIGN(SIZEOF(.text), section_align) +
			ALIGN(SIZEOF(.data), section_align) +
			ALIGN(SIZEOF(.bss), section_align) +
			ALIGN(SIZEOF(.rodata), section_align) +
			ALIGN(SIZEOF(.others), section_align) + section_align) /* SizeOfImage */
		LONG(ALIGN(SIZEOF(.IMAGE_DOS_HEADER) + SIZEOF(.IMAGE_NT_HEADERS) +
			SIZEOF(.IMAGE_SECTION_HEADER), file_align)) /* SizeOfHeaders */
		LONG(0) /* Checksum */
		SHORT(subsystem) /* Subsystem */
		SHORT(0) /* DllCharacteristics */
		LONG(0x100000) /* SizeOfStackReserve */
		LONG(0x1000) /* SIzeOfStackCommit */
		LONG(0x100000) /* SizeOfHeapReserve */
		LONG(0x1000) /* SizeOfHeapCommit */
		LONG(0) /* LoaderFlags */
		LONG(0x10) /* NumberOfRvaAndSizes */
		FILL(0x00000000)
		. += 2 * 4 * 0x10;
		_optional_header_end = .;
	}

	.IMAGE_SECTION_HEADER :
	{
		/* ----- 1st section ----- */
		/* Name */
		LONG(text_exists && text_place == 0 ? text_name1 : data_exists && data_place == 0 ? data_name1 :
			bss_exists && bss_place == 0 ? bss_name1 : rodata_exists && rodata_place == 0 ? rodata_name1 :
			others_exists && others_place == 0 ? others_name1 : 0)
		LONG(text_exists && text_place == 0 ? text_name2 : data_exists && data_place == 0 ? data_name2 :
			bss_exists && bss_place == 0 ? bss_name2 : rodata_exists && rodata_place == 0 ? rodata_name2 :
			others_exists && others_place == 0 ? others_name2 : 0)
		/* VirtualSize */
		LONG(text_exists && text_place == 0 ? text_vsize : data_exists && data_place == 0 ? data_vsize :
			bss_exists && bss_place == 0 ? bss_vsize : rodata_exists && rodata_place == 0 ? rodata_vsize :
			others_exists && others_place == 0 ? others_vsize : 0)
		/* VirtualAddress */
		LONG(text_exists && text_place == 0 ? text_vaddr : data_exists && data_place == 0 ? data_vaddr :
			bss_exists && bss_place == 0 ? bss_vaddr : rodata_exists && rodata_place == 0 ? rodata_vaddr :
			others_exists && others_place == 0 ? others_vaddr : 0)
		/* SizeOfRawData */
		LONG(text_exists && text_place == 0 ? text_rsize : data_exists && data_place == 0 ? data_rsize :
			bss_exists && bss_place == 0 ? bss_rsize : rodata_exists && rodata_place == 0 ? rodata_rsize :
			others_exists && others_place == 0 ? others_rsize : 0)
		/* PointerToRawData */
		LONG(text_exists && text_place == 0 ? text_rptr : data_exists && data_place == 0 ? data_rptr :
			bss_exists && bss_place == 0 ? bss_rptr : rodata_exists && rodata_place == 0 ? rodata_rptr :
			others_exists && others_place == 0 ? others_rptr : 0)
		/* (PointerTo|NumberOf)(Relocations|Linenumbers) */
		LONG(0) LONG(0) SHORT(0) SHORT(0)
		/* Characteristics */
		LONG(text_exists && text_place == 0 ? text_chara : data_exists && data_place == 0 ? data_chara :
			bss_exists && bss_place == 0 ? bss_chara : rodata_exists && rodata_place == 0 ? rodata_chara :
			others_exists && others_place == 0 ? others_chara : 0)

		/* ----- 2nd section ----- */
		/* Name */
		LONG(data_exists && data_place == 1 ? data_name1 : bss_exists && bss_place == 1 ? bss_name1 :
			rodata_exists && rodata_place == 1 ? rodata_name1 : others_exists && others_place == 1 ? others_name1 : 0)
		LONG(data_exists && data_place == 1 ? data_name2 : bss_exists && bss_place == 1 ? bss_name2 :
			rodata_exists && rodata_place == 1 ? rodata_name2 : others_exists && others_place == 1 ? others_name2 : 0)
		/* VirtualSize */
		LONG(data_exists && data_place == 1 ? data_vsize : bss_exists && bss_place == 1 ? bss_vsize :
			rodata_exists && rodata_place == 1 ? rodata_vsize : others_exists && others_place == 1 ? others_vsize : 0)
		/* VirtualAddress */
		LONG(data_exists && data_place == 1 ? data_vaddr : bss_exists && bss_place == 1 ? bss_vaddr :
			rodata_exists && rodata_place == 1 ? rodata_vaddr : others_exists && others_place == 1 ? others_vaddr : 0)
		/* SizeOfRawData */
		LONG(data_exists && data_place == 1 ? data_rsize : bss_exists && bss_place == 1 ? bss_rsize :
			rodata_exists && rodata_place == 1 ? rodata_rsize : others_exists && others_place == 1 ? others_rsize : 0)
		/* PointerToRawData */
		LONG(data_exists && data_place == 1 ? data_rptr : bss_exists && bss_place == 1 ? bss_rptr :
			rodata_exists && rodata_place == 1 ? rodata_rptr : others_exists && others_place == 1 ? others_rptr : 0)
		/* (PointerTo|NumberOf)(Relocations|Linenumbers) */
		LONG(0) LONG(0) SHORT(0) SHORT(0)
		/* Characteristics */
		LONG(data_exists && data_place == 1 ? data_chara : bss_exists && bss_place == 1 ? bss_chara : 
			rodata_exists && rodata_place == 1 ? rodata_chara : others_exists && others_place == 1 ? others_chara : 0)

		/* ----- 3rd section ----- */
		/* Name */
		LONG(bss_exists && bss_place == 2 ? bss_name1 : rodata_exists && rodata_place == 2 ? rodata_name1 :
			others_exists && others_place == 2 ? others_name1 : 0)
		LONG(bss_exists && bss_place == 2 ? bss_name2 : rodata_exists && rodata_place == 2 ? rodata_name2 :
			others_exists && others_place == 2 ? others_name2 : 0)
		/* VirtualSize */
		LONG(bss_exists && bss_place == 2 ? bss_vsize : rodata_exists && rodata_place == 2 ? rodata_vsize :
			others_exists && others_place == 2 ? others_vsize : 0)
		/* VirtualAddress */
		LONG(bss_exists && bss_place == 2 ? bss_vaddr : rodata_exists && rodata_place == 2 ? rodata_vaddr :
			others_exists && others_place == 2 ? others_vaddr : 0)
		/* SizeOfRawData */
		LONG(bss_exists && bss_place == 2 ? bss_rsize : rodata_exists && rodata_place == 2 ? rodata_rsize :
			others_exists && others_place == 2 ? others_rsize : 0)
		/* PointerToRawData */
		LONG(bss_exists && bss_place == 2 ? bss_rptr : rodata_exists && rodata_place == 2 ? rodata_rptr :
			others_exists && others_place == 2 ? others_rptr : 0)
		/* (PointerTo|NumberOf)(Relocations|Linenumbers) */
		LONG(0) LONG(0) SHORT(0) SHORT(0)
		/* Characteristics */
		LONG(bss_exists && bss_place == 2 ? bss_chara : rodata_exists && rodata_place == 2 ? rodata_chara :
			others_exists && others_place == 2 ? others_chara : 0)

		/* ----- 4th section ----- */
		/* Name */
		LONG(rodata_exists && rodata_place == 3 ? rodata_name1 : others_exists && others_place == 3 ? others_name1 : 0)
		LONG(rodata_exists && rodata_place == 3 ? rodata_name2 : others_exists && others_place == 3 ? others_name2 : 0)
		/* VirtualSize */
		LONG(rodata_exists && rodata_place == 3 ? rodata_vsize : others_exists && others_place == 3 ? others_vsize : 0)
		/* VirtualAddress */
		LONG(rodata_exists && rodata_place == 3 ? rodata_vaddr : others_exists && others_place == 3 ? others_vaddr : 0)
		/* SizeOfRawData */
		LONG(rodata_exists && rodata_place == 3 ? rodata_rsize : others_exists && others_place == 3 ? others_rsize : 0)
		/* PointerToRawData */
		LONG(rodata_exists && rodata_place == 3 ? rodata_rptr : others_exists && others_place == 3 ? others_rptr : 0)
		/* (PointerTo|NumberOf)(Relocations|Linenumbers) */
		LONG(0) LONG(0) SHORT(0) SHORT(0)
		/* Characteristics */
		LONG(rodata_exists && rodata_place == 3 ? rodata_chara : others_exists && others_place == 3 ? others_chara : 0)

		/* ----- 5th section ----- */
		/* Name */
		LONG(others_exists && others_place == 4 ? others_name1 : 0)
		LONG(others_exists && others_place == 4 ? others_name2 : 0)
		/* VirtualSize */
		LONG(others_exists && others_place == 4 ? others_vsize : 0)
		/* VirtualAddress */
		LONG(others_exists && others_place == 4 ? others_vaddr : 0)
		/* SizeOfRawData */
		LONG(others_exists && others_place == 4 ? others_rsize : 0)
		/* PointerToRawData */
		LONG(others_exists && others_place == 4 ? others_rptr : 0)
		/* (PointerTo|NumberOf)(Relocations|Linenumbers) */
		LONG(0) LONG(0) SHORT(0) SHORT(0)
		/* Characteristics */
		LONG(others_exists && others_place == 4 ? others_chara : 0)
	}

	.dummy :
	AT(LOADADDR(.IMAGE_SECTION_HEADER) + SIZEOF(.IMAGE_SECTION_HEADER)) {}

	.text ALIGN(section_align) :
	AT(ALIGN(LOADADDR(.dummy) + SIZEOF(.dummy), file_align))
	{
		*(.text)
	}

	.data ALIGN(section_align) :
	AT(ALIGN(LOADADDR(.text) + SIZEOF(.text), file_align))
	{
		*(.data)
	}

	.bss ALIGN(section_align) (NOLOAD) :
	AT(ALIGN(LOADADDR(.data) + SIZEOF(.data), file_align))
	{
		*(.bss)
		*(COMMON)
	}
	_nobss = (SIZEOF(.bss) == 0);

	.rodata ALIGN(section_align) :
	AT(ALIGN(LOADADDR(.data) + SIZEOF(.data), file_align))
	{
		*(.rodata)
	}

	.others ALIGN(section_align) :
	AT(ALIGN(LOADADDR(.rodata) + SIZEOF(.rodata), file_align))
	{
		_others_start = . ;
		*(*)
		_others_size = ABSOLUTE(. - _others_start);
		FILL(0x00000000)
		. = ALIGN(file_align);
	}

	/* calculate section information after loading */
	text_exists = (SIZEOF(.text) > 0);
	text_place = 0;
	text_name1 = 0x7865742e; text_name2 = 0x00000074;
	text_vsize = SIZEOF(.text);
	text_vaddr = ADDR(.text) - image_base;
	text_rsize = ALIGN(SIZEOF(.text), file_align);
	text_rptr = LOADADDR(.text);
	text_chara = 0x60000020;

	data_exists = (SIZEOF(.data) > 0);
	data_place = text_place + text_exists;
	data_name1 = 0x7461642e; data_name2 = 0x00000061;
	data_vsize = SIZEOF(.data);
	data_vaddr = ADDR(.data) - image_base;
	data_rsize = ALIGN(SIZEOF(.data), file_align);
	data_rptr = LOADADDR(.data);
	data_chara = 0xc0000040;

	bss_exists = (SIZEOF(.bss) > 0);
	bss_place = data_place + data_exists;
	bss_name1 = 0x7373622e; bss_name2 = 0x00000000;
	bss_vsize = SIZEOF(.bss);
	bss_vaddr = ADDR(.bss) - image_base;
	bss_rsize = 0;
	bss_rptr = 0;
	bss_chara = 0xc0000080;

	rodata_exists = (SIZEOF(.rodata) > 0);
	rodata_place = bss_place + bss_exists;
	rodata_name1 = 0x646f722e; rodata_name2 = 0x00617461;
	rodata_vsize = SIZEOF(.rodata);
	rodata_vaddr = ADDR(.rodata) - image_base;
	rodata_rsize = ALIGN(SIZEOF(.rodata), file_align);
	rodata_rptr = LOADADDR(.rodata);
	rodata_chara = 0x40000040;

	others_exists = (_others_size > 0);
	others_place = rodata_place + rodata_exists;
	others_name1 = 0x68746f2e; others_name2 = 0x00737265;
	others_vsize = _others_size;
	others_vaddr = ADDR(.others) - image_base;
	others_rsize = ALIGN(_others_size, file_align);
	others_rptr = LOADADDR(.others);
	others_chara = 0xc0000040;

	sections_count = others_place + others_exists;
}
