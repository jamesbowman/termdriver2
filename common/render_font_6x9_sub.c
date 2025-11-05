#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "drawscreen.h"
#include "../gen/font_6x9_sub.i"

extern screen_t screen;

//          R G B
// 0  g0      0
// 4  r0    0
// 8  r1    1
// 12 b0        0
// 16 b1        1
// 20 g1      1
// 24 g2      2
// 28 r2    2
// 32 r3    3
// 36 b2        2
// 40 b3        3
// 44 g3      3
//

// Each glyph is 9 lines
// Each line is 64 bits (48 used) of red, green, blue

void line_text_6x9_sub(uint8_t *dst, int y)
{
  uint8_t *dp = dst;
  uint16_t yi = (y / 9);
  uint32_t ymod = y - (yi * 9);
  uint32_t *psrc = screen.s + (screen.cols * ((yi + screen.y) & 0x1f));
  const int stride = 2 * 3;

#define N(c, i)  ((c >> (4 * i)) & 0xf)
#define CH(c, i)  ((c >> (4 + 8 * i)) & 0xf)
#define PACK8(a, b) (a | (b << 4))

  for (int x = 0; x < 40; x++) {
    uint32_t el = psrc[x];
    uint16_t fg = (el >> 8) & 0xfff;
    uint16_t bg = (el >> 20) & 0xfff;
    uint8_t ch = el & 0xff;
    uint32_t r0, r1, g0, g1, b0, b1;
    // ch = 'a';

    int o = (9 * (ch - 0x20) + ymod);
    int ri = font_6x9_sub_indices[o];
    if (ri != 0) {
      const uint32_t *prgb = &font_6x9_sub_rows[stride * ri];

      r0 = prgb[0] * N(fg, 2) + (0x0f0f0f0f - prgb[0]) * N(bg, 2);
      r1 = prgb[1] * N(fg, 2) + (    0x0f0f - prgb[1]) * N(bg, 2);
      g0 = prgb[2] * N(fg, 1) + (0x0f0f0f0f - prgb[2]) * N(bg, 1);
      g1 = prgb[3] * N(fg, 1) + (    0x0f0f - prgb[3]) * N(bg, 1);
      b0 = prgb[4] * N(fg, 0) + (0x0f0f0f0f - prgb[4]) * N(bg, 0);
      b1 = prgb[5] * N(fg, 0) + (    0x0f0f - prgb[5]) * N(bg, 0);
    } else {
      r0 = r1 = 0x0f0f0f0f * N(bg, 2);
      g0 = g1 = 0x0f0f0f0f * N(bg, 1);
      b0 = b1 = 0x0f0f0f0f * N(bg, 0);
    }

    *dp++ = PACK8(CH(g0, 0), CH(r0, 0));
    *dp++ = PACK8(CH(r0, 1), CH(b0, 0));
    *dp++ = PACK8(CH(b0, 1), CH(g0, 1));
    *dp++ = PACK8(CH(g0, 2), CH(r0, 2));
    *dp++ = PACK8(CH(r0, 3), CH(b0, 2));
    *dp++ = PACK8(CH(b0, 3), CH(g0, 3));
    *dp++ = PACK8(CH(g1, 0), CH(r1, 0));
    *dp++ = PACK8(CH(r1, 1), CH(b1, 0));
    *dp++ = PACK8(CH(b1, 1), CH(g1, 1));
  }
}
