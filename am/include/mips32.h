#ifndef __MIPS32_H__
#define __MIPS32_H__

#ifndef __ASSEMBLER__

#include <stdint.h>

#define MMIO_OFFSET(addr) ((uintptr_t)0xa0000000 + addr)

static inline uint8_t  inb(uintptr_t addr) { return *(volatile uint8_t  *)MMIO_OFFSET(addr); }
static inline uint16_t inw(uintptr_t addr) { return *(volatile uint16_t *)MMIO_OFFSET(addr); }
static inline uint32_t inl(uintptr_t addr) { return *(volatile uint32_t *)MMIO_OFFSET(addr); }

static inline void outb(uintptr_t addr, uint8_t  data) { *(volatile uint8_t  *)MMIO_OFFSET(addr) = data; }
static inline void outw(uintptr_t addr, uint16_t data) { *(volatile uint16_t *)MMIO_OFFSET(addr) = data; }
static inline void outl(uintptr_t addr, uint32_t data) { *(volatile uint32_t *)MMIO_OFFSET(addr) = data; }

#define PTE_V 0x2
#define PTE_D 0x4

// Page directory and page table constants
#define NR_PDE    1024    // # directory entries per page directory
#define NR_PTE    1024    // # PTEs per page table
#define PGSHFT    12      // log2(PGSIZE)
#define PTXSHFT   12      // Offset of PTX in a linear address
#define PDXSHFT   22      // Offset of PDX in a linear address

// +--------10------+-------10-------+---------12----------+
// | Page Directory |   Page Table   | Offset within Page  |
// |      Index     |      Index     |                     |
// +----------------+----------------+---------------------+
//  \--- PDX(va) --/ \--- PTX(va) --/\------ OFF(va) ------/
typedef uint32_t PTE;
typedef uint32_t PDE;
#define PDX(va)     (((uint32_t)(va) >> PDXSHFT) & 0x3ff)
#define PTX(va)     (((uint32_t)(va) >> PTXSHFT) & 0x3ff)
#define OFF(va)     ((uint32_t)(va) & 0xfff)

// Address in page table or page directory entry
#define PTE_ADDR(pte)   ((uint32_t)(pte) & ~0xfff)

#define SR_IE   0x00000001//interrupt enable

#define STRINGIFY(s) #s
#define TOSTRING(s) STRINGIFY(s)


void _putc(char ch);/* {
  outb(SERIAL_PORT, ch);
}*/

void _halt(int code);/* {
  __asm__ volatile("move $v0, %0; .word 0xf0000000" : :"r"(code));

  // should not reach here
  while (1);
}*/

static inline void puts(const char *s) {
  for (; *s; s++)
    _putc(*s);
}
#define panic(s) \
  do { \
    puts("AM Panic: "); puts(s); \
    puts(" @ " __FILE__ ":" TOSTRING(__LINE__) "  \n"); \
    _halt(1); \
  } while(0)

static inline uint32_t get_sr() {//get status register
  volatile uint32_t sr;
  asm volatile ("mfc0 %0, $12, 0" : "=r"(sr));
  return sr;
}

static inline void di() {//disable interrupt
  volatile uint32_t sr;
  sr = get_sr() & 0xfffffffe;//set ie = 0
  asm volatile ("mtc0 %0, $12, 0" : : "r"(sr));//write status register
}

static inline void ei() {//enable interrupt
  volatile uint32_t sr;
  sr = get_sr() | SR_IE;//set ie = 1
  asm volatile ("mtc0 %0, $12, 0" : : "r"(sr));//write status register
}

#endif

#endif
