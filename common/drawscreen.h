#include <stdint.h>

#define COLS  60  // (240 / 4)
#define ROWS  25  // (200 / 8)

#define LAMP_TX   0
#define LAMP_RX   1
#define LAMP_RTS  2
#define LAMP_DTR  3

#define LAMP_ATTACK 160

typedef struct {
  uint32_t traffic;
  uint32_t cols, rows;
  uint32_t y;
  uint32_t s[COLS * 32];
  int32_t lamps[4];
  int32_t freeze;
  int32_t cursor;
  uint32_t l_dtr, l_rts;
  uint32_t pause, paused;
  // 1,234,567 8N1
  char mode[14];
} screen_t;

void line1(uint8_t *dst, int y);
void inclamp(size_t i, int32_t n);
