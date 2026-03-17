MEMORY
{
  /* IRAM/DRAM on ESP32-C3 is 400 KB */
  SRAM (RWX) : ORIGIN = 0x40380000, LENGTH = 400K
}

ENTRY(_start)

SECTIONS
{
  .text :
  {
    _text_start = .;
    KEEP(*(.text.entry))
    *(.text .text.*)
    _text_end = .;
  } > SRAM

  .rodata :
  {
    *(.rodata .rodata.*)
  } > SRAM

  .data :
  {
    _data_start = .;
    *(.data .data.*)
    *(.sdata .sdata.*)
    _data_end = .;
  } > SRAM

  .bss :
  {
    _bss_start = .;
    *(.bss .bss.*)
    *(.sbss .sbss.*)
    *(COMMON)
    _bss_end = .;
  } > SRAM

  /* End of RAM for stack */
  _stack_top = ORIGIN(SRAM) + LENGTH(SRAM);
}
