/**
 * neon attempt
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <memory.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <libgen.h>
#include <arm_neon.h>

#include "v6lib.h"

// "q" means 128 bit values

#define PAND(a,b) vandq_u64(a,b)
#define PANDN(a,b) vbicq_u64(a,b)
#define PXOR(a,b) veorq_u64(a,b)
#define PXORN(a,b) vornq_u64(a,b)
#define POR(a,b) vorrq_u64(a,b)

// FIXME: check these are really 128 bit?

#define PSHL(a,b) vqrshlq_u64(a,b)
#define PSHR(a,b) vqrshrq_u64(a,b)

#define PMASKL(a) PSHL(u128_ones,a) // zero fill right load
#define PMASKR(a) PSHR(u128_ones,a) // zero fill left load

// https://community.arm.com/processors/b/blog/posts/coding-for-neon---part-4-shifting-left-and-right

#define PMASK(a) PMASKL(a) // FIXME: wrong and endian dependent

// Conditionals

#define PISZERO(a) a
#define PISNOTZERO(a) !a

// these are nice because you can shift by a constant int
// but tricky because they shift one 64 bit value left.
// It's saner to just load a constant 128 bit value and use
// the regular shift instructions above
// #define PSHRL(a,b) vqrshlq_u64(a,b)
// #define PSHRC(a,b) vqrshrq_u64(a,b)

// While this is a sane way to do it on x86
//static const u128 u128_zero = {0};
//static const u128 u128_ones = {-1};

// this is probably saner on neon til I figure out
// how to zero allocate a register (xor itself?)
//#define u128_zero vdup_n_u32(0)
//#define u128_ones vdup_n_u32(-1)

static u128 u128_zero = {0};
static u128 u128_ones = {-1};

// vget_lane_u64

// 

typedef uint64x2_t u128;

// FIXME - how does the ABI handle this order? Should we put the last first?

int find_prefix(v6_prefix_table *p, int size, u128 prefix, u8 mask) {
	int i;
	i128 s = vld1q_u64(mask);
	u128 m = PMASK(s);
      	u128 cmp = PAND(prefix,m);
	for(i = 0; i<size &&
	      PISZERO(PXOR(PAND(p[i].prefix,m),cmp)); i++) ;
	return i;
}
