#include <lcom/lcf.h>

#include <machine/int86.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>

#include "video.h"

#define VBE_SET_MODE 0x4F02
#define LINEAR_FB BIT(14)

static vbe_mode_info_t mode_info;
static uint8_t *video_mem;          // Memória real da placa gráfica (VRAM)
static uint8_t *secondary_buffer;   // Buffer secundário para evitar flickering
static unsigned h_res;
static unsigned v_res;
static unsigned bits_per_pixel;
static unsigned bytes_per_pixel;

static int(set_graphics_mode)(uint16_t mode) {
  struct reg86 r;
  memset(&r, 0, sizeof(r));

  r.intno = 0x10;
  r.ax = VBE_SET_MODE;
  r.bx = mode | LINEAR_FB;

  if (sys_int86(&r) != OK)
    return 1;

  if (r.ax != 0x004F)
    return 1;

  return 0;
}

void *(my_vg_init)(uint16_t mode) {
  if (vbe_get_mode_info(mode, &mode_info) != 0)
    return NULL;

  h_res = mode_info.XResolution;
  v_res = mode_info.YResolution;
  bits_per_pixel = mode_info.BitsPerPixel;
  bytes_per_pixel = (bits_per_pixel + 7) / 8;

  struct minix_mem_range mr;
  unsigned int vram_base = mode_info.PhysBasePtr;
  unsigned int vram_size = h_res * v_res * bytes_per_pixel;

  mr.mr_base = vram_base;
  mr.mr_limit = mr.mr_base + vram_size;

  if (sys_privctl(SELF, SYS_PRIV_ADD_MEM, &mr) != OK)
    return NULL;

  video_mem = vm_map_phys(SELF, (void *) mr.mr_base, vram_size);
  if (video_mem == MAP_FAILED)
    return NULL;

  // Alocação dinâmica de memória para o buffer secundário
  secondary_buffer = (uint8_t *) malloc(vram_size);
  if (secondary_buffer == NULL)
    return NULL;

  if (set_graphics_mode(mode) != 0)
    return NULL;

  return video_mem;
}

int(my_vg_draw_pixel)(uint16_t x, uint16_t y, uint32_t color) {
  if (x >= h_res || y >= v_res)
    return 1;

  unsigned int index = (y * h_res + x) * bytes_per_pixel;
  
  // Agora escrevemos no buffer secundário em vez de enviar direto para o ecrã
  memcpy(&secondary_buffer[index], &color, bytes_per_pixel);

  return 0;
}

int(my_vg_draw_hline)(uint16_t x, uint16_t y, uint16_t len, uint32_t color) {
  for (uint16_t col = 0; col < len; col++) {
    if (my_vg_draw_pixel(x + col, y, color) != 0)
      return 1;
  }

  return 0;
}

int(my_vg_draw_rectangle)(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color) {
  for (uint16_t row = 0; row < height; row++) {
    if (my_vg_draw_hline(x, y + row, width, color) != 0)
      return 1;
  }

  return 0;
}

int(my_vg_draw_xpm)(uint8_t *xpm, uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
  if (xpm == NULL)
    return 1;

  for (uint16_t row = 0; row < height; row++) {
    for (uint16_t col = 0; col < width; col++) {
      uint32_t color = xpm[row * width + col];

      if (my_vg_draw_pixel(x + col, y + row, color) != 0)
        return 1;
    }
  }

  return 0;
}

// Limpa o ecrã virtual pintando-o todo a preto (0) antes do próximo frame
void (vg_clear)() {
  unsigned int vram_size = h_res * v_res * bytes_per_pixel;
  memset(secondary_buffer, 0, vram_size);
}

// Despeja todo o conteúdo do buffer secundário na VRAM real de uma só vez
void (vg_flush)() {
  unsigned int vram_size = h_res * v_res * bytes_per_pixel;
  memcpy(video_mem, secondary_buffer, vram_size);
}
