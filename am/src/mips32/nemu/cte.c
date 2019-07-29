#include <am.h>
#include <mips32.h>
#include <klib.h>

static _Context* (*user_handler)(_Event, _Context*) = NULL;

void __am_get_cur_as(_Context *c);
void __am_switch(_Context *c);
void __am_tlb_refill(void);

//cp0_count increments independently, and cp0_compare needs to be writen
//when cp0_compare == cp0_count, interrupt occurs(for thread switch maybe?)
void update_timer(uint32_t step) {//cp0_compare = cp0_count + step
  uint32_t compare = 0;
  //compare = cp0_count
  MFC0(compare, CP0_COUNT, 0);//#define CP0_COUNT 9(cp0 register)
  compare += step;
  //cp0_compare = compare
  MTC0(CP0_COMPARE, compare, 0);//#define CP0_COMPARE 11(cp0 register)
}
//same as update_timer
static void init_timer(int step) {
  int compare = 0;
  MFC0(compare, CP0_COUNT, 0);
  compare += step;
  MTC0(CP0_COMPARE, compare, 0);
}
static inline void flush_cache(void *begin, void *end) {
  for(void *p = begin; p < end; p += 4) {
    //why??
	  asm volatile("cache 0x10,0(%0)" ::"r"(p));//10000b: invalidate cache
	  asm volatile("cache 0x15,0(%0)" ::"r"(p));//10101b: write back & invalidate cache
  }
}

_Context* __am_irq_handle(_Context *c) {
  __am_get_cur_as(c);

  _Context *next = c;
  if (user_handler) {
    _Event ev = {0};
    uint32_t ex_code = (c->cause >> 2) & 0x1f;
    uint32_t syscall_instr;
    switch (ex_code) {
      case 0: ev.event = _EVENT_IRQ_TIMER; break;
      case 2:
      case 3: __am_tlb_refill(); return next;
      case 8: 
        syscall_instr = *(uint32_t *)(c->epc);
        ev.event = ((syscall_instr >> 6) == 1) ? _EVENT_YIELD : _EVENT_SYSCALL;
        c->epc += 4;
        break;
      default: ev.event = _EVENT_ERROR; break;
    }

    next = user_handler(ev, c);
    if (next == NULL) {
      next = c;
    }
  }

  __am_switch(next);

  return next;
}

extern void __am_asm_trap(void);

#define EX_ENTRY 0x80000180

int _cte_init(_Context*(*handler)(_Event, _Context*)) {
  // initialize exception entry
  printf("cte_init\n");
  const uint32_t j_opcode = 0x08000000;
  uint32_t instr = j_opcode | (((uint32_t)__am_asm_trap >> 2) & 0x3ffffff);
  *(uint32_t *)EX_ENTRY = instr;
  *(uint32_t *)0x80000000 = instr;  // TLB refill exception

  // register event handler
  user_handler = handler;

  return 0;
}

_Context *_kcontext(_Area stack, void (*entry)(void *), void *arg) {
  _Context *c = (_Context*)stack.end - 1;
  //if(arg) printf("kcontext: %x, %s\n", entry, (char*) arg);
  //else printf("kcontext: %x\n", entry);
  c->epc = (uintptr_t)entry;
  return c;
}

static inline void enable_interrupt() {
  cp0_status_t c0_status;
  MFC0(c0_status, CP0_STATUS, 0);
  //asm volatile("mfc0 %0, $%1" :: "r"(c0_status), "i"(CP0_STATUS));
  c0_status.IM  = 0xFF;//interrupt mask, enable/disable interrupts(0: disable, 1: enable)
                       //0~1: Software interrupt
                       //2~6: Hardware interrupt
                       //7: Hardware/timer/performance counter interrupt
  c0_status.ERL = 0;//0: normal level 1: error level(kernel mode & interrupt disabled & eret --> errorepc(not epc) & ...)
  c0_status.IE  = 1;//0: interrupt disable 1: interrupt enable
  MTC0(CP0_STATUS, c0_status, 0);
  //asm volatile("mtc0 %0, $%1" :: "r"(c0_status), "i"(CP0_STATUS));
}

void _yield() {
  init_timer(INTERVAL);
  enable_interrupt();
  asm volatile("nop; mthi $0; mtlo $0; li $a0, -1; syscall; nop");
}

int _intr_read() {//get IF(interrupt enable flag), return 1 if interrupt is enabled(IF == 1)
  if (!user_handler) panic("no interrupt handler");
  return get_sr().IE != 0;
}

void _intr_write(int enable) {//cli: disable interrupt; sti: enable interrupt;
  if (!user_handler) panic("no interrupt handler");
  if (enable) {
    ei();
  } else {
    di();
  }
}

int _istatus(int enable) {
  return 0;
}
