// IMPORTANT: you must include the following line in all your C files
#include <lcom/lcf.h>

#include <lcom/lab5.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include "kbc.h"
#include "video.h"

int main(int argc, char *argv[]) {
  lcf_set_language("EN-US");

  lcf_trace_calls("/home/lcom/labs/lab5/trace.txt");
  lcf_log_output("/home/lcom/labs/lab5/output.txt");

  if (lcf_start(argc, argv))
    return 1;

  lcf_cleanup();

  return 0;
}

static int wait_for_esc_breakcode(void) {
  int ipc_status, r;
  message msg;
  uint8_t irq_set;
  bool done = false;

  if (my_kbc_subscribe_int(&irq_set) != 0)
    return 1;

  while (!done) {
    if ((r = driver_receive(ANY, &msg, &ipc_status)) != 0) {
      printf("driver_receive failed with: %d\n", r);
      continue;
    }

    if (is_ipc_notify(ipc_status)) {
      switch (_ENDPOINT_P(msg.m_source)) {
        case HARDWARE:
          if (msg.m_notify.interrupts & irq_set) {
            my_kbc_ih();

            if (kbd_scancode_ready && kbd_scancode == 0x81)
              done = true;
          }
          break;
        default:
          break;
      }
    }
  }

  if (my_kbc_unsubscribe_int() != 0)
    return 1;

  return 0;
}

int(video_test_init)(uint16_t mode, uint8_t delay) {
  if (my_vg_init(mode) == NULL)
    return 1;

  sleep(delay);

  if (vg_exit() != 0)
    return 1;

  return 0;
}

int(video_test_rectangle)(uint16_t mode, uint16_t x, uint16_t y,
                          uint16_t width, uint16_t height, uint32_t color) {
  if (my_vg_init(mode) == NULL)
    return 1;

  if (my_vg_draw_rectangle(x, y, width, height, color) != 0) {
    vg_exit();
    return 1;
  }

  if (wait_for_esc_breakcode() != 0) {
    vg_exit();
    return 1;
  }

  if (vg_exit() != 0)
    return 1;

  return 0;
}

int(video_test_xpm)(xpm_map_t xpm, uint16_t x, uint16_t y) {
  xpm_image_t img;
  uint8_t *map;

  if (my_vg_init(0x105) == NULL)
    return 1;

  map = xpm_load(xpm, XPM_INDEXED, &img);
  if (map == NULL) {
    vg_exit();
    return 1;
  }

  if (my_vg_draw_xpm(map, x, y, img.width, img.height) != 0) {
    vg_exit();
    return 1;
  }

  if (wait_for_esc_breakcode() != 0) {
    vg_exit();
    return 1;
  }

  if (vg_exit() != 0)
    return 1;

  return 0;
}
