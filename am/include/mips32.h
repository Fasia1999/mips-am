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

#define MFC0(dst, src, sel) \
asm volatile("mfc0 %0, $"_STR(src)", %1; nop\n\t":"=r"(dst):"i"(sel))

#define MTC0(dst, src, sel) \
asm volatile("mtc0 %0, $"_STR(dst)", %1; nop\n\t"::"g"(src),"i"(sel))

// Address in page table or page directory entry
#define PTE_ADDR(pte)   ((uint32_t)(pte) & ~0xfff)

#define STRINGIFY(s) #s
#define TOSTRING(s) STRINGIFY(s)


void _putc(char ch);

void _halt(int code);

//cp0 registers
#define CP0_INDEX        0
#define CP0_RANDOM       1
#define CP0_ENTRY_LO0    2
#define CP0_ENTRY_LO1    3
#define CP0_CONTEXT      4  // maintained by kernel
#define CP0_PAGEMASK     5
#define CP0_WIRED        6
#define CP0_RESERVED     7  // for extra debug and segment
#define CP0_BADVADDR     8
#define CP0_COUNT        9
#define CP0_ENTRY_HI     10
#define CP0_COMPARE      11
#define CP0_STATUS       12
#define CP0_CAUSE        13
#define CP0_EPC          14
#define CP0_PRID         15 // sel = 0
#define CP0_EBASE        15 // sel = 1
#define CP0_CONFIG       16

typedef struct {
	uint32_t IE   : 1;
	uint32_t EXL  : 1;
	uint32_t ERL  : 1;
	uint32_t R0   : 1;

	uint32_t UM   : 1;
	uint32_t UX   : 1;
	uint32_t SX   : 1;
	uint32_t KX   : 1;

	uint32_t IM   : 8;

	uint32_t Impl : 2;
	uint32_t _0   : 1;
	uint32_t NMI  : 1;
	uint32_t SR   : 1;
	uint32_t TS   : 1;

	uint32_t BEV  : 1;
	uint32_t PX   : 1;

	uint32_t MX   : 1;
	uint32_t RE   : 1;
	uint32_t FR   : 1;
	uint32_t RP   : 1;
	uint32_t CU   : 4;
} cp0_status_t;
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

static inline cp0_status_t get_sr() {//get status register
  volatile cp0_status_t sr;
  MFC0(sr, CP0_STATUS, 0);
  return sr;
}

static inline void di() {//disable interrupt
  volatile cp0_status_t sr;
  sr = get_sr();
  sr.IE = 0;//set ie = 0
  MTC0(CP0_STATUS, sr, 0);//write status register
}

static inline void ei() {//enable interrupt
  volatile cp0_status_t sr;
  sr = get_sr();
  sr.IE = 1;//set ie = 1
  MTC0(CP0_STATUS, sr, 0);//write status register
}

#endif

#endif
