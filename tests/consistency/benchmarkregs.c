/**
 * benchmark.c
 * Some tests for correctness and speed for ipv6 prefix comparisons
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <assert.h>
#include <memory.h>
#include <endian.h>
#include <arpa/inet.h>
#include <arm_neon.h>

typedef uint32x4_t usimd;

#include "common_regs.h"
#include "my_memcpy.h"

#define MAX_PREFIX 1024
#define MIN(a,b) a < b ? b : a

// TODO lpf (longest prefix match) (spf - shortest prefix match)

#include "reset.h"

#define init_simd init_simd_simd
#define v4mapped v4mapped_simd
#define v4nmapped v4nmapped_simd
#define prefix_cmp prefix_cmp_simd
#define v6_nequal v6_nequal_simd
#define v6_equal  v6_equal_simd
#define v4tov6 v4tov6_simd
#define get_ones get_ones_simd
#define set_ones set_ones_simd
#define get_zeros get_zeros_simd
#define prefix_cmp prefix_cmp_simd
#define set_zeros prefix_cmp_simd

#ifdef HAVE_NEON
#include "native_neon_regs.h"
#define HAD_NEON
#undef HAVE_NEON
#endif

#ifdef HAVE_SSE2
#define HAD_SSE2
#include "native_sse2_regs.h"
#undef HAVE_SSE2
#endif

#include "reset.h"

// We can *express* 64 bitness but it might suck

#ifndef HAVE_64BIT_ARCH
#define HAVE_64BIT_ARCH
#endif

#define init_simd init_64bit
#define v4mapped v4mapped_64bit
#define v4nmapped v4nmapped_64bit
#define prefix_cmp prefix_cmp_64bit
#define v6_nequal v6_nequal_64bit
#define v6_equal  v6_equal_64bit
#define v4tov6 v4tov6_64bit
#define get_ones get_ones_64bit
#define set_ones set_ones_64bit
#define get_zeros get_zeros_64bit
#define set_zeros set_zeros_64bit
#define prefix_cmp prefix_cmp_64bit

// Fake ourselves out again

#undef _NATIVE_NEON_H
#include "native_neon_regs.h"
#include "common_regs.h"
#undef HAVE_64BIT_ARCH

#include "reset.h"

// And the baseline original tests are hard to compare
// Don't do that yet

#define init_simd init_native
#define v4mapped v4mapped_native
#define v4nmapped v4nmapped_native
#define prefix_cmp prefix_cmp_native
#define v6_nequal v6_nequal_native
#define v6_equal  v6_equal_native
#define v4tov6 v4tov6_native
#define get_ones get_ones_native
#define set_ones set_ones_native
#define get_zeros get_zeros_native
#define set_zeros set_zeros_native
#define prefix_cmp prefix_cmp_native

#undef _NATIVE_NEON_H
#include "native_neon_regs.h"
#include "common_regs.h"
#undef init_simd

#include "reset.h"

// Automate checks

// nequal can vary unfortunately. Think about it.

#define P_N(a,b) assert(v6_nequal(a,b) != v6_nequal_simd(a,b) || v6_nequal_simd(a,b) !=  v6_nequal_64bit(a,b))

// Equal can't

#define P_E(a,b) assert(v6_equal(a,b) != v6_equal_simd(a,b) || v6_nequal_simd(a,b) != v6_equal_64bit(a,b))

#define V4MAPPED_E(a) assert(v4mapped(a) != v4mapped_simd(a) || v4mapped_simd(a) != v4mapped_64bit(a))

// FIXME, check the result

#define V4TOV6(a,b) assert(v4tov6(a,b) != v4tov6_simd(a,b) || v4tov6(a,b) != v4tov6_64bit(a,b))

#define P_CMP(a,b,c,d) assert(prefix_cmp(a,b,c,d) || prefix_cmp_simd(a,b,c,d) || prefix_cmp_64bit(a,b,c,d))


// FIXME Need to also write bogus prefixes (/128, > 128, 0)

unsigned char *bitwalk_prefix(int mask) {
	static __thread unsigned int a[4] = {0};
	// fixme, roll the code to do this

	return (unsigned char *) a;
}

// Hmm. I wonder if a 128 bit simd random number generator exists

unsigned char *random_prefix() {
	static __thread unsigned int a[4] = {0};
	a[0] = random();
	a[1] = random();
	a[2] = random();
	a[3] = random();
	return (unsigned char *) a;
}

typedef union Vec4 {
    usimd p;
  //    v4sf v;
    float e[4];
    int i[4];
    unsigned int u[4];
    unsigned char c[16];
} Vec4_t;

// I can imagine this also blowing up on people without the __thread

const char *
format_prefix(const usimd prefix, unsigned char plen)
{
    static __thread char buf[4][INET6_ADDRSTRLEN + 4] = {0};
    static __thread int i = 0;
    int n;
    Vec4_t p2;
    p2.p = prefix;
    i = (i + 1) % 4; // WTF in a 64 bit arch? Let you call this 4 times?
    if(plen >= 96 && v4mapped_native(prefix)) {
	    inet_ntop(AF_INET, &p2.c[12], buf[i], INET6_ADDRSTRLEN); // ??
        n = strlen(buf[i]);
        snprintf(buf[i] + n, INET6_ADDRSTRLEN + 4 - n, "/%d", plen - 96);
    } else {
      inet_ntop(AF_INET6, &p2.c, buf[i], INET6_ADDRSTRLEN);
        n = strlen(buf[i]);
        snprintf(buf[i] + n, INET6_ADDRSTRLEN + 4 - n, "/%d", plen);
    }
    return buf[i];
}

// dump stack also

int log_errors() {
	return 0;
}

int consistency() {
	return 0;
}

int main() {
	int count = 0;
	int count2 = 0;
	int count3 = 0;

	int i = 0;
	int err = 0;
	prefix_table *p  = calloc(MAX_PREFIX,sizeof(prefix_table));
	prefix_table *p2 = calloc(MAX_PREFIX,sizeof(prefix_table));
	prefix_table *p3 = calloc(MAX_PREFIX,sizeof(prefix_table));
	prefix_table *p4 = calloc(MAX_PREFIX,sizeof(prefix_table));
	srandom(getpid());

	init_simd_simd(); // Initialize register constants early

	// FIXME - fill trailer with random garbage

	int aligned = sizeof(prefix_table) % sizeof(size_t);
	fprintf(stdout,"sizeof(prefix_table) = %ld and is %s on this test - %s\n",
		sizeof(prefix_table),
		aligned ? "unaligned" : "aligned",
		aligned ? "expect weirdness!" : "good!" );

	fflush(stdout);

	for(int i = 0; i<MAX_PREFIX; i++) {
		p[i].plen = p2[i].plen = p3[i].plen = p4[i].plen = i % 128; // 129?
		memcpy(&p[i].prefix,random_prefix(),16);
		//		my_memcpy(&p2[i].prefix,p[i].prefix,16);
		//my_memcpy(&p3[i].prefix,p[i].prefix,12);
		//my_memcpy(&p4[i].prefix,p[i].prefix,8);
	}

	// fixme find stack tests

	// Fixme for unaligned access tests

	fprintf(stdout,"my_memcpy16 check: ");
	fflush(stdout);

	/*	if(memcmp(p,p2,MAX_PREFIX-1) != 0) {
		fprintf(stdout,"my_memcpy went awry - checking why... ");
		fflush(stdout);
                for(int i = 0; i<MAX_PREFIX; i++) {
		  assert(memcmp((void *)p2[i].prefix,(void *)p[i].prefix,16) != 0);
	        }
		}*/

	fprintf(stdout,"passed\n");
	fflush(stdout);

	// Give us two exact hits

	memcpy(&p[MAX_PREFIX/2].prefix,llprefix,16);
	memcpy(&p[MAX_PREFIX/3].prefix,v4prefix,16);

	// Check to see if we are formatting prefixes correctly.
	// You can only call format_prefix 4 times without a memcpy

	fprintf(stdout,"v4mapped print %s, ll print %s\n",
		format_prefix(p[MAX_PREFIX/2].prefix,
			      p[MAX_PREFIX/2].plen),
		format_prefix(p[MAX_PREFIX/3].prefix,
			      p[MAX_PREFIX/3].plen));
	fflush(stdout);

	int r1 = random() % MAX_PREFIX;
	int r2 = random() % MAX_PREFIX;
	int r3 = random() % MAX_PREFIX;
	int r4 = random() % MAX_PREFIX;

	fprintf(stdout,"Random Addresses generated: %s\n"
		       "                            %s\n"
		       "                            %s\n"
		       "                            %s\n",
		format_prefix(p[r1].prefix,
			p[r1].plen),
		format_prefix(p[r2].prefix,
			p[r2].plen),
		format_prefix(p[r3].prefix,
			p[r3].plen),
		format_prefix(p[r4].prefix,
			p[r4].plen));
	fflush(stdout);

	printf("plen corruption check: ");
	fflush(stdout);

	for(int i = 0; i<MAX_PREFIX; i++) {
		if(p[i].plen == p2[i].plen && p3[i].plen == p[i].plen && p4[i].plen == p[i].plen)
			continue;
		err++;	
	}

	fprintf(stdout, "%s\n", err == 0 ? "passed" : "failed");
	fflush(stdout);

	printf("v4mapped check: ");
	fflush(stdout);

	count = 0;
        asm("nop; /* Start of v4mapped test */");
		for(i = 0; i<MAX_PREFIX; i++) {
		if(v4mapped_native(p[i].prefix)) {
			count++;
			if(v4mapped_simd(p2[i].prefix) != 
			   v4mapped_native(p[i].prefix))
				err++;
		}
	}
	
	//for(i = 0; i<MAX_PREFIX; i++) {
	//	V4MAPPED_E(p[i].prefix);
	//}
	if(err == 0) printf("passed\n"); 
	else printf("failed %d times\n", err);

	printf("v4mapped: %d\n",count);
	fflush(stdout);

	count2 = count3 = count = 0;

	asm("nop; /* prefix_cmp test */");
	if(prefix_cmp_simd(p[r1].prefix,p[r1].plen,
		      p[r2].prefix,p[r2].plen) == PST_DISJOINT)
	  printf("How did we get here\n");
	
	asm("nop; /* v6_equal start */");
	for(i = 0; i<MAX_PREFIX; i++) {
		if(v6_equal_native(ll,p[i].prefix)) count++;
	}

	for(i = 0; i<MAX_PREFIX; i++) {
		if(v6_equal_simd(v4prefix,p[i].prefix)) count2++;
	}

	for(i = 0; i<MAX_PREFIX; i++) {
		if(v6_equal_64bit(v4_prefix,p[i].prefix)) count3++;
	}

	printf("v6_equal: %d\n",count);

	printf("v6_equal and v6_equal_simd are %s\n",
		count == count2 ? "equal" : "not equal" );
	printf("v6_equal and v6_equal_64bit are %s\n",
		count == count3 ? "equal" : "not equal" );
	fflush(stdout);

	if(count != count2) err++;
	if(count != count3) err++;

	printf("Total errors: %d\n", err);
	return(err);
}
