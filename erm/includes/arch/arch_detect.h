/*
 * arch_detect.h
 *
 * 2017-03-08
 */

#ifndef ARCH_DETECT_H
#define ARCH_DETECT_H

#ifdef __GNUC__
#define GCC_VERSION                                                          \
  (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#endif

#define P(a)                                                                 \
  typedef struct a a##_t;                                                    \
  printf("%4ld = sizeof (%s)\n", sizeof(a##_t), #a)


#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
const char endian[] = "little";
#else
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
const char endian[] = "big";
#else
#if __BYTE_ORDER__ == __ORDER_PDP_ENDIAN__
const char endian[] = "pdp - you are kidding me, right?";
#else
const char endian[] = "unknown";
#endif
#endif
#endif

#if !(defined(__arm__) || defined(__aarch64__) || defined(__x86_64__) ||     \
      defined(__i386__) || defined(__mips__) || defined(__epiphany__))
const char arch[] = "unknown";
#else
#if defined(__arm__) || defined(__aarch64__)
#if defined(__ARM_ARCH_ISA_A64__) || defined(__aarch64__)
const char arch[] = "aarch64";
#else
#ifdef __ARM_ARCH_7S__
const char arch[] = "armv7s";
#else
#ifdef __ARM_ARCH_7A__
const char arch[] = "armv7";
#endif
#endif
#endif /* end of arm detection. Fixme - need to find neon regs */
#endif

#if defined(__x86_64__)
const char arch[] = "x86_64";
#endif

#if defined(__i386__)
const char arch[] = "x86";
#endif

#if defined(__mips__)
const char arch[] = "MIPS";
#endif

#if defined(__epiphany__)
const char arch[] = "epiphany";
#endif

#endif

#endif
