#include <am.h>
#include <x86.h>

#define RTC_PORT 0x48   // Note that this is not standard
static unsigned long boot_time;

void _ioe_init() {
  boot_time = inl(RTC_PORT);
}

unsigned long _uptime() {
  return inl(RTC_PORT) - boot_time;
}

uint32_t* const fb = (uint32_t *)0x40000;

_Screen _screen = {
  .width  = 400,
  .height = 300,
};

extern void* memcpy(void *, const void *, int);

void _draw_rect(const uint32_t *pixels, int x, int y, int w, int h) {
  int i, j;
  for (i = 0; i < h; i++) {
    for (j = 0; j < w; j++) {
      fb[(y + i) * _screen.width + (x + j)] = pixels[i * w + j];
    }
  }
}

void _draw_sync() {
}

int _read_key() {
  int key = inb(0x60);
  if (key == 0 || key == 0xe0) return _KEY_NONE;
  return key + 0x8000;
}
