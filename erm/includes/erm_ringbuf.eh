/*
 * erm_ringbuf.eh © 2017 Michael David Täht
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

// Natural bitwidth index generator for ringbuffers of type NEWRING
// For single consumer/multiple producer.
// Elsewhere you would do something like this
// sed s/NEWRING/yourtype/g erm_ringbuf.eh |
// sed s/RINGWIDTH/yourwidthtype/g erm_ringbuf.eh > yourtype.h

// NEWRING_ring_t NEWRING_i SECTION("ringbufs");
// NEWRING NEWRING_r[NATURALWIDTH] SECTION("ringdata");

typedef
{
  RINGWIDTH in;
  RINGWIDTH out;
}
NEWRING_ring_t;

void NEWRING_init() { NEWRING_r[] = { 0 }; }

inline bool NEWRING_is_full() { return (NEWRING_i.in - NEWRING_i.out) == -1; }

inline bool NEWRING_is_empty() { return (NEWRING_i.in - NEWRING_i.out) == 0; }

inline bool NEWRING_is_hiwater()
{
  return (NEWRING_i.in - NEWRING_i.out) > (3 * 256 / 4);
}

inline bool NEWRING_is_lowater()
{
  return (NEWRING_i.in - NEWRING_i.out) > (256 / 4);
}

inline u8 NEWRING_size() { return (NEWRING_i.in - NEWRING_i.out); }

// Simplest possible locking enqueue and dequeue

inline void NEWRING_enqueue(NEWRING value)
{
    NEWRING_r[(atomic_inc(NEWRING_i.in)] = value;
}

NEWRING NEWRING_dequeue() {return NEWRING_r[(atomic_inc(NEWRING_i.out)]; }

// Less locking push pop

inline RINGWIDTH NEWRING_copyto(RINGWIDTH qty, NEWRING* values)
{
  RINGWIDTH i = NEWRING_i.out;
  RINGWIDTH size = NEWRING_i.in - i;
  RINGWIDTH c = 0;
  if(size > qty) size = qty;
  
  for(; c < size; c++) values[c] = NEWRING_r[i + c];
  atomic_update(NEWRING_i.out, NEWRING_i.out + c);
  return c;
}

inline RINGWIDTH NEWRING_copyfrom(RINGWIDTH qty, NEWRING* values)
{
  RINGWIDTH size = (NEWRING_i.in - NEWRING_i.out) - 1;
  RINGWIDTH c = 0;
  RINGWIDTH i = NEWRING_i.in;
  if(size > qty) size = qty;
  for(; c < size; c++) NEWRING_r[i + c] = values[c];
  atomic_update(NEWRING_i.in, NEWRING_i.in + c);
  return c;
}

// Unsafe versions

inline void NEWRING_enqueue_unsafe(NEWRING value)
{
  NEWRING_r[NEWRING_i.in++] = value;
}

NEWRING NEWRING_dequeue_unsafe() { return NEWRING_r[NEWRING_i.out++]; }
