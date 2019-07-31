/*SECTIONS {
  . = 0x80000000;
  .text : {
    *(entry)
    *(.text)
  }
  etext = .;
  _etext = .;
  .rodata : {
    *(.rodata*)
  }
  .data : {
    *(.data)
  }
  edata = .;
  _data = .;
  .bss : {
	_bss_start = .;
    *(.bss*)
    *(.sbss*)
    *(.scommon)
  }
  _stack_top = ALIGN(4096);
  . = _stack_top + 0x8000;
  _stack_pointer = .;
  end = .;
  _end = .;
  _heap_start = ALIGN(4096);
  _heap_end = 0x88000000;
}*/

MEMORY {
  ddr : ORIGIN = 0x80000000, LENGTH = 128M
}

SECTIONS {
  .text : {
    *(loader)
    *(.text)
    etext = .;
    _etext = .;
  } > ddr
  .ex_entry : {
	KEEP(*(SORT(.ex_entry*))) ;
  } > ddr
  .data : {
    *(.data)
    edata = .;
    _edata = .;
  } > ddr
  .bss : {
    *(.bss*)
    *(.sbss)
    *(.scommon)
    end = .;
    _end = .;
  } > ddr
  _heap_start = ALIGN(4096);
  _stack_pointer = ORIGIN(ddr) + LENGTH(ddr);
  _heap_end = _stack_pointer - 4 * 1024 * 1024;
}

