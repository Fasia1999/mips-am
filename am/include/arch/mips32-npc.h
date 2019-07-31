#ifndef __ARCH_H__
#define __ARCH_H__

struct gpr {
  uint32_t at,
  	v0,v1,
	  a0,a1,a2,a3,
	  t0,t1,t2,t3,t4,t5,t6,t7,t8,t9,
	  s0,s1,s2,s3,s4,s5,s6,s7,
	  k0,k1,
	  gp,sp,fp,ra;
};

struct _Context {
  union {
    struct _AddressSpace *prot;
    struct gpr gpr;
  };
  uint32_t epc, cause, status, badvaddr, base;
  uint32_t hi, lo;
};

#define GPR1 a0
#define GPR2 a1
#define GPR3 a2
#define GPR4 a3
#define GPRx v0

#endif
