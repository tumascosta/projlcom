#include <lcom/lcf.h>
#include <stdint.h>

uint32_t cnt = 0;

int(util_sys_inb)(int port, uint8_t *value) {
  uint32_t val;

  if (sys_inb(port, &val) != 0)
    return 1;

  *value = (uint8_t) val;

#ifdef LAB3
  cnt++;
#endif

  return 0;
}
