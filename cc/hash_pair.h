// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Defines methods for hashing a pair of integers.

#ifndef CC_HASH_PAIR_H_
#define CC_HASH_PAIR_H_

#include "base/hash_tables.h"

#if defined(COMPILER_MSVC)

#define DEFINE_PAIR_HASH_FUNCTION_START(type1, type2) \
    template<> \
    inline std::size_t hash_value<std::pair<type1, type2> >( \
        const std::pair<type1, type2>& value)

#define DEFINE_PAIR_HASH_FUNCTION_END()

#elif defined(COMPILER_GCC)

#define DEFINE_PAIR_HASH_FUNCTION_START(type1, type2) \
    template<> \
    struct hash<std::pair<type1, type2> > { \
      std::size_t operator()(std::pair<type1, type2> value) const

#define DEFINE_PAIR_HASH_FUNCTION_END() \
    };

#else
#error define DEFINE_PAIR_HASH_FUNCTION_START for your compiler
#endif // COMPILER

namespace BASE_HASH_NAMESPACE {

// Implement hashing for pairs of at-most 32 bit integer values.
// When size_t is 32 bits, we turn the 64-bit hash code into 32 bits by using
// multiply-add hashing. This algorithm, as described in
// Theorem 4.3.3 of the thesis "Über die Komplexität der Multiplikation in
// eingeschränkten Branchingprogrammmodellen" by Woelfel, is:
//
//   h32(x32, y32) = (h64(x32, y32) * randOdd64 + rand16 * 2^16) % 2^64 / 2^32

#define DEFINE_32BIT_PAIR_HASH(type1, type2) \
    DEFINE_PAIR_HASH_FUNCTION_START(type1, type2) { \
      uint64 first = value.first; \
      uint32 second = value.second; \
      uint64 hash64 = (first << 32) | second; \
      \
      if (sizeof(std::size_t) >= sizeof(uint64)) \
        return static_cast<std::size_t>(hash64); \
      \
      uint64 oddRandom = 481046412LL << 32 | 1025306954LL; \
      uint32 shiftRandom = 10121U << 16; \
      \
      hash64 = hash64 * oddRandom + shiftRandom; \
      std::size_t highBits = static_cast<std::size_t>( \
          hash64 >> (sizeof(uint64) - sizeof(std::size_t))); \
      return highBits; \
    } \
  DEFINE_PAIR_HASH_FUNCTION_END();

DEFINE_32BIT_PAIR_HASH(short, short);
DEFINE_32BIT_PAIR_HASH(short, unsigned short);
DEFINE_32BIT_PAIR_HASH(short, int);
DEFINE_32BIT_PAIR_HASH(short, unsigned);
DEFINE_32BIT_PAIR_HASH(unsigned short, short);
DEFINE_32BIT_PAIR_HASH(unsigned short, unsigned short);
DEFINE_32BIT_PAIR_HASH(unsigned short, int);
DEFINE_32BIT_PAIR_HASH(unsigned short, unsigned);
DEFINE_32BIT_PAIR_HASH(int, short);
DEFINE_32BIT_PAIR_HASH(int, unsigned short);
DEFINE_32BIT_PAIR_HASH(int, int);
DEFINE_32BIT_PAIR_HASH(int, unsigned);
DEFINE_32BIT_PAIR_HASH(unsigned, short);
DEFINE_32BIT_PAIR_HASH(unsigned, unsigned short);
DEFINE_32BIT_PAIR_HASH(unsigned, int);
DEFINE_32BIT_PAIR_HASH(unsigned, unsigned);

#undef DEFINE_32BIT_PAIR_HASH

// Implement hashing for pairs of up-to 64-bit integer values.
// We use the compound integer hash method to produce a 64-bit hash code, by
// breaking the two 64-bit inputs into 4 32-bit values:
// http://opendatastructures.org/versions/edition-0.1d/ods-java/node33.html#SECTION00832000000000000000
// Then we reduce our result to 32 bits if required, similar to above.

#define DEFINE_64BIT_PAIR_HASH(type1, type2) \
    DEFINE_PAIR_HASH_FUNCTION_START(type1, type2) { \
      uint32 shortRandom1 = 842304669U; \
      uint32 shortRandom2 = 619063811U; \
      uint32 shortRandom3 = 937041849U; \
      uint32 shortRandom4 = 3309708029U; \
      \
      uint64 value1 = value.first; \
      uint64 value2 = value.second; \
      uint32 value1a = static_cast<uint32>(value1 & 0xffffffff); \
      uint32 value1b = static_cast<uint32>((value1 >> 32) & 0xffffffff); \
      uint32 value2a = static_cast<uint32>(value2 & 0xffffffff); \
      uint32 value2b = static_cast<uint32>((value2 >> 32) & 0xffffffff); \
      \
      uint64 product1 = static_cast<uint64>(value1a) * shortRandom1; \
      uint64 product2 = static_cast<uint64>(value1b) * shortRandom2; \
      uint64 product3 = static_cast<uint64>(value2a) * shortRandom3; \
      uint64 product4 = static_cast<uint64>(value2b) * shortRandom4; \
      \
      uint64 hash64 = product1 + product2 + product3 + product4; \
      \
      if (sizeof(std::size_t) >= sizeof(uint64)) \
        return static_cast<std::size_t>(hash64); \
      \
      uint64 oddRandom = 1578233944LL << 32 | 194370989LL; \
      uint32 shiftRandom = 20591U << 16; \
      \
      hash64 = hash64 * oddRandom + shiftRandom; \
      std::size_t highBits = static_cast<std::size_t>( \
          hash64 >> (sizeof(uint64) - sizeof(std::size_t))); \
      return highBits; \
    } \
  DEFINE_PAIR_HASH_FUNCTION_END();

DEFINE_64BIT_PAIR_HASH(short, int64);
DEFINE_64BIT_PAIR_HASH(short, uint64);
DEFINE_64BIT_PAIR_HASH(unsigned short, int64);
DEFINE_64BIT_PAIR_HASH(unsigned short, uint64);
DEFINE_64BIT_PAIR_HASH(int, int64);
DEFINE_64BIT_PAIR_HASH(int, uint64);
DEFINE_64BIT_PAIR_HASH(unsigned, int64);
DEFINE_64BIT_PAIR_HASH(unsigned, uint64);
DEFINE_64BIT_PAIR_HASH(int64, short);
DEFINE_64BIT_PAIR_HASH(int64, unsigned short);
DEFINE_64BIT_PAIR_HASH(int64, int);
DEFINE_64BIT_PAIR_HASH(int64, unsigned);
DEFINE_64BIT_PAIR_HASH(int64, int64);
DEFINE_64BIT_PAIR_HASH(int64, uint64);
DEFINE_64BIT_PAIR_HASH(uint64, short);
DEFINE_64BIT_PAIR_HASH(uint64, unsigned short);
DEFINE_64BIT_PAIR_HASH(uint64, int);
DEFINE_64BIT_PAIR_HASH(uint64, unsigned);
DEFINE_64BIT_PAIR_HASH(uint64, int64);
DEFINE_64BIT_PAIR_HASH(uint64, uint64);

#undef DEFINE_64BIT_PAIR_HASH
}

#undef DEFINE_PAIR_HASH_FUNCTION_START
#undef DEFINE_PAIR_HASH_FUNCTION_END

#endif  // CC_HASH_PAIR_H_
