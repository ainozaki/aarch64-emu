#ifndef UTILS_H_
#define UTILS_H_

#define SHIFT(x, bottom, top) ((x >> bottom) & ((1 << (top - bottom + 1)) - 1))
#define BIT(x, bit) ((x >> bit) & 1)

#endif // UTILS_H_
