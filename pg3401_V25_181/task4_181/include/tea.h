// include/tea.h
#ifndef TEA_H
#define TEA_H

#include <stdint.h>

extern const uint32_t TEA_KEY[4];
void encipher(uint32_t v[2], uint32_t w[2], const uint32_t k[4]);

#endif
