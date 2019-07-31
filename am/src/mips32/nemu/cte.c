#include <am.h>
#include <mips32.h>
#include <klib.h>

static _Context* (*user_handler)(_Event, _Context*) = NULL;
uint8_t am_kstack[16 * 1024];
uint32_t am_kstack_size = sizeof(am_kstack);

//void __am_get_cur_as(_Context *c);
//void __am_switch(_Context *c);
//void __am_tlb_refill(void);

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

void trace_count()
{
  int count = 0;
  MFC0(count, CP0_COUNT, 0);
  //printf("trace_count> cp0_count: %d\n", count);
}

void trace_compare()
{
  int compare = 0;
  MFC0(compare, CP0_COMPARE, 0);
  //printf("trace_compare> cp0_compare: %d\n", compare);
}

void trace_status()
{
  cp0_status_t status;
  MFC0(status, CP0_STATUS, 0);
  //printf("trace_status> cp0_status: 0x%x\n", status);
}

void trace_cause()
{
  int cause;
  MFC0(cause, CP0_CAUSE, 0);
  //printf("trace_cause> cp0_cause: 0x%x\n", cause);
}

static void init_timer(int step) {
  int compare = 0;
  MFC0(compare, CP0_COUNT, 0);
  //printf("init_timer> cp0_count: %d\n", compare);
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

/*_Context* __am_irq_handle(_Context *c) {
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
}*/

void __am_irq_handle(_Context *regs){
  //printf("*****************irq_handle******************\n");
  cp0_cause_t *cause = (void*)&(regs->cause);
  uint32_t exccode = cause->ExcCode;
  printf("--reg start addr: %x\n", regs);
  printf("--at addr: %x\n", &regs->at);
  printf("--v0 addr: %x\n", &regs->v0);
  printf("--v1 addr: %x\n", &regs->v1);
  printf("--a0 addr: %x\n", &regs->a0);
  printf("--cause addr: %x\n", &regs->cause);
  //printf("irq_handle> epc: 0x%x\n", regs->epc);
  //printf("irq_handle> cause: 0x%x\n", regs->cause);
  //printf("irq_handle> status: 0x%x\n", regs->status);
  /*int epc;
  MFC0(epc, CP0_EPC, 0);
  printf("--epc: %x\n", epc);
  int cause_reg;
  asm volatile("mfc0 %0, $%[c0_cause]":"=r"(cause_reg) :[c0_cause]"i"(CP0_CAUSE));
  printf("--cause is 0x%x\n", cause_reg);
  int status;
  MFC0(status, CP0_STATUS, 0);
  printf("--status: %x\n", status);*/
  //uint32_t ipcode = cause->IP;
  //uint32_t exccode = (temp & 0x0000007c) >> 2;
  //uint32_t ipcode = (temp & 0x0000ff00) >> 8;
  //printf("irq_handle> cause.ExcCode: 0x%x\n", exccode);
  //printf("irq_handle> cause.ipcode: 0x%x\n", ipcode);
  
  _Event ev;
  ev.event = _EVENT_NULL;
  //TODO: exception handling
  // Delayslot should be considered when handle exceptions !!!
  update_timer(INTERVAL); // update when exception happens
  switch(exccode){
    case EXC_INTR: {
      //if(ipcode & IP_TIMER_MASK) {
          ev.event = _EVENT_IRQ_TIMER;
		  //cause->IP = 0;
		  //asm volatile("mtc0 %0, $13, 0;nop;nop"::"r"(regs->cause));
	    //} else {
		    //printk("invalid ipcode = %x\n", ipcode);
		    //_halt(-1);
      //}
      break;
    }
    case EXC_SYSCALL:
      regs->epc += 4;
	    if(regs->a0 == -1)
		    ev.event = _EVENT_YIELD;
	    else
		    ev.event = _EVENT_SYSCALL;
        break;
    case EXC_TRAP:
      ev.event = _EVENT_SYSCALL;
      break;
	  case EXC_TLBM:
	  case EXC_TLBL:
	  case EXC_TLBS:
		  ev.event = _EVENT_PAGEFAULT;
		  break;
    case EXC_AdEL:
    case EXC_AdES:
    case EXC_BP:
    case EXC_RI:
    case EXC_OV:
    default:
	    printk("unhandled exccode = %x, epc:%08x, badvaddr:%08x\n", exccode, regs->epc, regs->badvaddr);
	    _halt(-1);
  }

  _Context *ret = regs;
  if(user_handler) {
	  _Context *next = user_handler(ev, regs);
	  if(next != NULL) ret = next;
  }

  // restore common registers
  asm volatile(
	".set noat;"
    "lw $v0, %[v0];" "lw $at, %[at];"
    "lw $a0, %[a0];" "lw $a1, %[a1];" "lw $a2, %[a2];" "lw $a3, %[a3];"
    "lw $t0, %[t0];" "lw $t1, %[t1];" "lw $t2, %[t2];" "lw $t3, %[t3];"
    "lw $t4, %[t4];" "lw $t5, %[t5];" "lw $t6, %[t6];" "lw $t7, %[t7];"
    "lw $s0, %[s0];" "lw $s1, %[s1];" "lw $s2, %[s2];" "lw $s3, %[s3];"
    "lw $s4, %[s4];" "lw $s5, %[s5];" "lw $s6, %[s6];" "lw $s7, %[s7];"
    "lw $t8, %[t8];" "lw $t9, %[t9];"
    "lw $gp, %[gp];" "lw $fp, %[fp];" "lw $ra, %[ra];" "lw $sp, %[sp];"
    : : 
    [v0]"m"(ret->v0), [at]"m"(ret->at),
    [a0]"m"(ret->a0), [a1]"m"(ret->a1), [a2]"m"(ret->a2), [a3]"m"(ret->a3),
    [t0]"m"(ret->t0), [t1]"m"(ret->t1), [t2]"m"(ret->t2), [t3]"m"(ret->t3),
    [t4]"m"(ret->t4), [t5]"m"(ret->t5), [t6]"m"(ret->t6), [t7]"m"(ret->t7),
    [s0]"m"(ret->s0), [s1]"m"(ret->s1), [s2]"m"(ret->s2), [s3]"m"(ret->s3),
    [s4]"m"(ret->s4), [s5]"m"(ret->s5), [s6]"m"(ret->s6), [s7]"m"(ret->s7),
    [t8]"m"(ret->t8), [t9]"m"(ret->t9),
    [gp]"m"(ret->gp), [fp]"m"(ret->fp), [ra]"m"(ret->ra), [sp]"m"(ret->sp)
    :"v0","at",
     "a0","a1","a2","a3",
     "t0","t1","t2","t3",
     "t4","t5","t6","t7",
     "s0","s1","s2","s3","s4","s5","s6","s7",
     "t8","t9",
     "fp","ra","sp"
    );

  asm volatile(
    "lw $k0, %[epc];  nop; mtc0 $k0, $%[epc_no];  nop;"
    "lw $k0, %[base]; nop; mtc0 $k0, $%[base_no]; nop;"
	"lw $k0, %[hi]; mthi $k0; nop;"
	"lw $k0, %[lo]; mtlo $k0; nop;"
    "lw $v1, %[v1];"
	"eret;"
	: :
    [epc]"m"(ret->epc),
    [base]"m"(ret->base),
    [hi]"m"(ret->hi), [lo]"m"(ret->lo),
    [v1]"m"(ret->v1),
	[epc_no]"i"(CP0_EPC),
	[base_no]"i"(CP0_BASE)
	);
}

extern void __am_asm_trap(void);

#define EX_ENTRY 0x80000180

/*int _cte_init(_Context*(*handler)(_Event, _Context*)) {
  // initialize exception entry
  printf("cte_init\n");
  const uint32_t j_opcode = 0x08000000;
  uint32_t instr = j_opcode | (((uint32_t)__am_asm_trap >> 2) & 0x3ffffff);
  *(uint32_t *)EX_ENTRY = instr;
  *(uint32_t *)0x80000000 = instr;  // TLB refill exception

  // register event handler
  user_handler = handler;

  return 0;
}*/

static inline void setup_bev() {//BEV:Controls the location of exception vectors base address
  uint32_t c0_status = 0;
  //printf("setup_bev> ");
  //trace_status();
  asm volatile("mfc0 %0, $%1" :: "r"(c0_status), "i"(CP0_STATUS));
  //printf("c0_status: 0x%x\n", c0_status);
  c0_status |= 0x00400000;//set bev = 1; exception vector base address:
                    //.reset,soft, NMI = 0xbfc00000
                    //.EJTAG Debug(EJTAG_Control_register.ProbEn = 0) = 0Xbfc00480
                    //.EJTAG Debug(EJTAG_Control_register.ProbEn = 1) = 0Xff200200
                    //.Cache Error = 0xbfc00200
                    //.other = 0xbfc00200
  //printf("c0_status: 0x%x\n", c0_status);
  //c0_status.IE = 1;
  //printf("c0_status: 0x%x\n", c0_status);
  asm volatile("mtc0 %0, $%1" :: "r"(c0_status), "i"(CP0_STATUS));
  //printf("setup_bev> ");
  //trace_status();
}

void *get_exception_entry() {
  static uint8_t __attribute__((unused, section(".ex_entry.1"))) data[0];
  //printf("data1 addr: 0x%x\n", data);
  return (void *)data;
}

uint32_t get_exception_entry_size() {
  static uint8_t __attribute__((unused, section(".ex_entry.3"))) data[0];
  //printf("data2 addr: 0x%x\n", data);
  static uint8_t __attribute__((unused, section(".ex_entry.2"))) test[0];
  //printf("test addr: 0x%x\n", test);
  return (void  *)data - get_exception_entry();
}

static inline void set_handler(unsigned offset, void *addr, int size) {
  //printf("set_handler at %x\n", EBASE + offset);
  memcpy(EBASE + offset, addr, size);//EBASE = 0xbfc00000
  flush_cache(EBASE + offset, EBASE + offset + size);
}

int _cte_init(_Context* (*handler)(_Event ev, _Context *regs)){
  user_handler = handler; // set asye handler
  //printf("_cte_init> ");
  trace_status();
  setup_bev();//set exception verctor base address bit

  void *entry = get_exception_entry();
  uint32_t size = get_exception_entry_size();

  //printf("cte_init> ex_entry: 0x%x\n", (int)entry);
  set_handler(0x000, entry, size); // TLB
  set_handler(0x180, entry, size); // EXCEPTION
  set_handler(0x200, entry, size); // INTR
  set_handler(0x380, entry, size); // LOONGSON
  return 0;
}

_Context *_kcontext(_Area stack, void (*entry)(void *), void *arg) {
  //save context
  _Context *c = (_Context*)stack.end - 1;
  //if(arg) printf("kcontext: %x, %s\n", entry, (char*) arg);
  //else printf("kcontext: %x\n", entry);
  c->a0 = (uintptr_t)arg;
  c->sp = (uint32_t) stack.end;
  c->epc = (uint32_t)entry;
  return c;
}

/*_Context *_kcontext(_Area kstack, void (*entry)(void *), void *args){
  if(args) printf("kcontext: %x, %s\n", entry, (char*) args);
  else printf("kcontext: %x\n", entry);
  _Context *regs = (_Context *)kstack.start;
  regs->sp = (uint32_t) kstack.end;
  regs->epc = (uint32_t) entry;

  static const char *envp[] = { "AM=true", NULL };

  uintptr_t *arg = args;
  regs->a0 = 0;
  regs->a1 = (uintptr_t)arg;
  regs->a2 = (uintptr_t)envp;
  for(; *arg; arg ++, regs->a0++);
  return regs;
}*/

void enable_interrupt() {
  init_timer(INTERVAL);
  cp0_status_t c0_status;
  MFC0(c0_status, CP0_STATUS, 0);
  //asm volatile("mfc0 %0, $%1" :: "r"(c0_status), "i"(CP0_STATUS));
  c0_status.IM  = 0xFF;//interrupt mask, enable/disable interrupts(0: disable, 1: enable)
                       //0~1: Software interrupt
                       //2~6: Hardware interrupt
                       //7: Hardware/timer/performance counter interrupt
  c0_status.ERL = 0;//0: normal level 1: error level(kernel mode & interrupt disabled & eret --> errorepc(not epc) & ...)
  trace_count();
  c0_status.IE  = 1;//0: interrupt disable 1: interrupt enable

  MTC0(CP0_STATUS, c0_status, 0);
  trace_count();
  //printf("enable_interrupt> ");
  trace_status();
  //printf("enable_interrupt> ");
  trace_cause();
  //asm volatile("mtc0 %0, $%1" :: "r"(c0_status), "i"(CP0_STATUS));
}

void _yield() {
  //printf("yield1\n");
  init_timer(INTERVAL);
  //printf("yield2\n");
  //enable_interrupt();
  //printf("yield3\n");
  asm volatile("nop; mthi $0; mtlo $0; li $a0, -1; syscall; nop");
}

/*void _yield() {
  asm volatile("syscall 1;");
}*/

int _intr_read() {//get IF(interrupt enable flag), return 1 if interrupt is enabled(IF == 1)
  if (!user_handler) panic("no interrupt handler");
  return get_sr().IE != 0;
}

void _intr_write(int enable) {//cli: disable interrupt; sti: enable interrupt;
  if (!user_handler) panic("no interrupt handler");
  if (enable) {
    ei();
    //printf("enable intr\n");
  } else {
    di();
    //printf("disable intr\n");
  }
}

int _istatus(int enable) {
  return 0;
}
