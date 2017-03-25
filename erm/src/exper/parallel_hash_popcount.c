/*
 * parallel_hash_popcount.c © 2017 Michael David Täht
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* The root of ERM's classification system is to *rapidly* get both a popcount
   of the value(s) and a usable hash. One of the things that irks me about hash
   developers is that they all benchmark their hashes with these enormouse loops
   against X amount of data - long strings often - each time. Big, beautiful benchmarks.

   Running a hot cache for their ins. As if our cpus did nothing more than hash
   stuff all day.

   I don't run the hash often enough to care about that! I need a quick hash on
   some fairly small fixed length values, then to move on. Start time is
   important. Now, it happens that cake ended up using a similar hash on 3
   values at the same time ... and I'm wondering that can yield to running 4
   hashes in parallel in the simd unit, which would be pretty awesome...

... but I don't know anyone that has single hash, multiple data code. (I should
look).

   The first problem, is actually popcount. Which is an SSE4 instruction that
actually runs in the main cpu. Awesome. 0 clock cycles, given all the other
overheads we got. Except, where we don't have popcount - which is on everything
else. __builtin_popcount is like 20 instructions per *word*. Which also might
yield to a vectorized approach. Now, if we can hash and popcount 4 values at
exactly the same time... big win. Worth writing lots of code for.

Well, first benchmarking what straightline code does makes the most sense. :)

*/


#include <math.h>
#include <stdint.h>
#include <tgmath.h>

#include "debug.h"
#include "erm_types.h"
#include "preprocessor.h"
#include "simd.h"

typedef struct {
	u8 one;
	u8 two;
} twocount;

inline twocount popcount2cheaper(const u64 *buf) {
    u64 cnt[1] = {0};
    const int i = 0;
    __asm__ __volatile__(
            "popcnt %2, %2  \n\t"
            "add %2, %0     \n\t"
            "popcnt %4, %4  \n\t"
            "add %4, %1     \n\t"
            "popcnt %3, %3  \n\t"
            "add %3, %0     \n\t"
            "popcnt %5, %5  \n\t"
            "add %5, %1     \n\t"
            : "+r" (cnt[0]), "+r" (cnt[1])
            : "r"  (buf[i]), "r"  (buf[i+1]), "r"  (buf[i+2]), "r"  (buf[i+3])
                );
  twocount t;
  t.one =   cnt[0];
  t.two =   cnt[1];
  return t;
}


inline twocount popcount2(const u64 *buf) {
    u64 cnt[4] = {0};
    const int i = 0;
    __asm__ __volatile__(
            "popcnt %4, %4  \n\t"
            "add %4, %0     \n\t"
            "popcnt %5, %5  \n\t"
            "add %5, %1     \n\t"
            "popcnt %6, %6  \n\t"
            "add %6, %2     \n\t"
            "popcnt %7, %7  \n\t"
            "add %7, %3     \n\t"
            : "+r" (cnt[0]), "+r" (cnt[1]), "+r" (cnt[2]), "+r" (cnt[3])
            : "r"  (buf[i]), "r"  (buf[i+1]), "r"  (buf[i+2]), "r"  (buf[i+3])
                );
  twocount t;
  t.one =   cnt[0] + cnt[1] ;
  t.two =   cnt[2] + cnt[3] ;
  return t;
}


#ifdef DEBUG_MODULE
#define LOGGER_INFO(where, fmt, ...)

#ifndef DONOTHING
#define DONOTHING                                                            \
  do {                                                                       \
  } while(0)
#endif

#ifndef CALLOCA
#define CALLOCA(type, dest, size, num)                                       \
  type dest __attribute__((aligned(16)));                                    \
  dest = calloc(size, num)
#endif

#include "align.h"
#include "cycles_bench.h"
#include "erm_logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

u64 test1[4] = {0};
u64 test2[4] = {0xff, 0xff, 0xff, 0x7f};
u64 test3[4] = {0xff7f,0x7fffffff, 0x7f, 0xff };
u64 test4[4] = {0xff7f,0x7fffffff, 0xff, 0x7f };

int main() {
	twocount t = popcount2(&test1);
	printf("popzero: %d %d\n",t.one, t.two);
	t = popcount2(&test2);
	printf("popzero: %d %d\n",t.one, t.two);
	t = popcount2(&test3);
	printf("popzero: %d %d\n",t.one, t.two);
	t = popcount2cheaper(&test4);
	printf("pop2cheaper: %d %d\n",t.one, t.two);
	return 0;
}

#endif
