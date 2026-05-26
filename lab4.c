#include <lcom/lcf.h>
#include <stdint.h>
#include <stdio.h>

#include "mouse.h"
#include "timer.h"

int (mouse_enable_data_reporting)();

int main(int argc, char *argv[]) {
  lcf_set_language("EN-US");
  lcf_trace_calls("/home/lcom/labs/lab4/trace.txt");
  lcf_log_output("/home/lcom/labs/lab4/output.txt");

  if (lcf_start(argc, argv))
    return 1;

  lcf_cleanup();
  return 0;
}

int (mouse_test_packet)(uint32_t cnt) {
  int ipc_status, r;
  message msg;
  uint8_t mouse_irq;

  if (mouse_subscribe_int(&mouse_irq) != 0)
    return 1;

  if (mouse_enable_data_reporting() != 0) {
    mouse_unsubscribe_int();
    return 1;
  }

  uint32_t printed = 0;
  struct packet pp;

  while (printed < cnt) {
    if ((r = driver_receive(ANY, &msg, &ipc_status)) != 0) {
      printf("driver_receive failed with: %d\n", r);
      continue;
    }

    if (is_ipc_notify(ipc_status)) {
      switch (_ENDPOINT_P(msg.m_source)) {
        case HARDWARE:
          if (msg.m_notify.interrupts & mouse_irq) {
            mouse_ih();

            if (mouse_packet_ready()) {
              mouse_build_packet(&pp);
              mouse_print_packet(&pp);
              printed++;
            }
          }
          break;

        default:
          break;
      }
    }
  }

  if (mouse_disable_stream_reporting() != 0) {
    mouse_unsubscribe_int();
    return 1;
  }

  if (mouse_unsubscribe_int() != 0)
    return 1;

  return 0;
}

int (mouse_test_async)(uint8_t idle_time) {
  int ipc_status, r;
  message msg;

  uint8_t mouse_irq;
  uint8_t timer_bit_no;

  if (mouse_subscribe_int(&mouse_irq) != 0)
    return 1;

  if (timer_subscribe_int(&timer_bit_no) != 0) {
    mouse_unsubscribe_int();
    return 1;
  }

  if (mouse_enable_stream_reporting() != 0) {
    timer_unsubscribe_int();
    mouse_unsubscribe_int();
    return 1;
  }

  uint32_t hz = (uint32_t) sys_hz();
  uint32_t idle_ticks = idle_time * hz;
  timer_counter = 0;

  struct packet pp;
  bool done = false;

  while (!done) {
    if ((r = driver_receive(ANY, &msg, &ipc_status)) != 0) {
      printf("driver_receive failed with: %d\n", r);
      continue;
    }

    if (is_ipc_notify(ipc_status)) {
      switch (_ENDPOINT_P(msg.m_source)) {
        case HARDWARE:
          if (msg.m_notify.interrupts & mouse_irq) {
            mouse_ih();

            if (mouse_packet_ready()) {
              mouse_build_packet(&pp);
              mouse_print_packet(&pp);
              timer_counter = 0;
            }
          }

          if (msg.m_notify.interrupts & BIT(timer_bit_no)) {
            timer_int_handler();

            if (timer_counter >= idle_ticks)
              done = true;
          }
          break;

        default:
          break;
      }
    }
  }

  if (mouse_disable_stream_reporting() != 0) {
    timer_unsubscribe_int();
    mouse_unsubscribe_int();
    return 1;
  }

  if (timer_unsubscribe_int() != 0) {
    mouse_unsubscribe_int();
    return 1;
  }

  if (mouse_unsubscribe_int() != 0)
    return 1;

  return 0;
}
