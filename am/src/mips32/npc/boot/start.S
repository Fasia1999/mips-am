#.section entry, "ax"
#.globl _start
#.type _start, @function

#_start:
#  move $fp, $zero
#  la $sp, _stack_pointer
#  jal _trm_init
.section loader, "ax"
.globl _start
.type _start, @function
.globl _ex_entry
.type _ex_entry, @function

_start:
  la $sp, _stack_pointer
  la $k0, _trm_init
  jr $k0

.org 0x20
_ex_entry:
  la $k0, _trap_entry
  jr $k0
