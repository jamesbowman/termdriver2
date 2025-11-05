#include <stdbool.h>

void text_init();
void text_ch(uint8_t ch);
void text_str(const uint8_t *s);
void hide_cursor();
void show_cursor();
void remap(bool dtr, bool rts, bool *m_dtr, bool *m_rts);
