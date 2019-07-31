#include <am.h>
#include <nemu.h>
#include <klib.h>

extern char _heap_start;
extern char _heap_end;
int main();

_Area _heap = {
  .start = &_heap_start,
  .end = &_heap_end,
};

void _putc(char ch) {
  while ((inl(SERIAL_PORT + 8) >> 3) & 1);
  outl(SERIAL_PORT + 4, ch);
}

void _halt(int code) {
#define GPIO_ADDR 0x10000000
  printf("halt\n");
  outl(GPIO_ADDR, code);
  // should not reach here
  while (1);
}

void _trm_init() {
  //printf("trm_init\n");
  //int ret = main();
  //printf("trm_init> main: %d\n", ret);
  _halt(main());
}
