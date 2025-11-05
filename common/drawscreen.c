#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "drawscreen.h"

extern screen_t screen;

#include "../gen/mainfont.i"
#include "../gen/lamps.i"

extern void line_text_4x6(uint8_t *dst, int y);
extern void line_text_6x9_sub(uint8_t *dst, int y);

#define PIXBYTES(n) ((n) * 3 / 2)

#define LAMP_BPL (LAMP_W * 3 / 2)

static void line_text(uint8_t *dst, int y)
{
  // line_text_4x6(dst, y);
  line_text_6x9_sub(dst, y);
}


// Number of uint32_t per line
#define WPL (240 * 3 / 8)

void modulate(uint8_t *dst, uint32_t n)
{
  uint32_t *p = (uint32_t*)dst;
  const uint32_t
    LO = 0x0f0f0f0f,
    HI = 0xf0f0f0f0;
  for (size_t i = 0; i < WPL; i++) {
    uint32_t xL = *p & LO;
    uint32_t xH = (*p & HI) >> 4;
    uint32_t pL = ((xL * n) & HI) >> 4;
    uint32_t pH = ((xH * n) & HI);
    *p++ = pL | pH;
  }
}

static void line_status(uint8_t *dst, int y)
{
  memset(dst, MAINFONT_BG * 0x11, 3 * 240 / 2);

  uint32_t w = mainfont[0], h = mainfont[1];
  uint32_t bpl = (w * 3) / 2;
  uint32_t bpc = bpl * h;
  uint32_t y0 = 22 - h / 2;
  uint32_t fy = (y - y0);
  const char *s = screen.mode;
  if (fy < h) {
    uint32_t x = 4;
    for (size_t i = 0; i < strlen(s); i++) {
      if (s[i] == ' ') {
        x += 8;
      } else if (s[i] == ',') {
        x += 3;
      } else {
        size_t ix = mainfont_decode[(uint8_t)s[i]];
        const uint8_t *src = mainfont + 2 + (bpc * ix) + (bpl * fy);
        x &= ~1;
        uint8_t *pd = dst + (x * 3 / 2);
        for (uint32_t i = 0; i < bpl; i++) {
          uint16_t a = (*src & 0xf) | ((*src & 0xf0) << 4);
          a = (MAINFONT_BG * 0x1010) - (a * MAINFONT_BG);
          src++;
          *pd++ = ((a & 0xf000) >> 8) | ((a & 0x00f0) >> 4);
        }
        x += w - 1;
      }
    }
  }

  y0 = ((40 - 32) / 2);
  fy = y - y0;
  if (fy < 32) {
    uint32_t lo = (fy >> 4) << 1;
    for (int i = 0; i < 2; i++) {
      const uint8_t *src = lamps[lo + i][screen.lamps[lo + i] >> 4] + LAMP_BPL * (fy & 0xf);
      memcpy(dst + PIXBYTES(188) + LAMP_BPL * i, src, LAMP_BPL);
    }
  }
}

void line1(uint8_t *dst, int y)
{
  if (y < 40)
    line_status(dst, y);
  else if (y == 40)
    line_text(dst, 0), modulate(dst, 10);
  else if (y == 41)
    line_text(dst, 1), modulate(dst, 12);
  else if (y == 42)
    line_text(dst, 2), modulate(dst, 13);
  else if (y < 238)
    line_text(dst, y - 40);
  else
    memset(dst, 0x00, 3 * 240 / 2);
}

void inclamp(size_t i, int32_t n)
{
  int32_t v = screen.lamps[i] + n;
  if (v < 0)
    v = 0;
  else if (v > 255)
    v = 255;
  screen.lamps[i] = v;
}
