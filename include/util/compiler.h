#pragma once

#if defined(__GNUC__) || defined(__clang__)
#  define BRGB_LIKELY(x)   __builtin_expect((x), 1)
#  define BRGB_UNLIKELY(x) __builtin_expect((x), 0)
#else
#  define BRGB_LIKELY(x)   (x)
#  define BRGB_UNLIKELY(x) (x)
#endif
