/*
High-speed in-memory bit stream I/O that supports reading and writing between
0 and 64 bits at a time.  The implementation, which relies heavily on bit
shifts, has been carefully written to ensure that all shifts are between
zero and one less the width of the type being shifted to avoid undefined
behavior.  This occasionally causes somewhat convoluted code.

The following assumptions and restrictions apply:

1. The user must allocate a memory buffer large enough to hold the bit stream,
   whether for reading, writing, or both.  This buffer is associated with the
   bit stream via stream_open(buffer, bytes), which allocates and returns a
   pointer to an opaque bit stream struct.  Call stream_close(stream) to
   deallocate this struct.

2. The stream is either in a read or write state (or, initially, in both
   states).  When done writing, call stream_flush(stream) before entering
   read mode to ensure any buffered bits are output.  To enter read mode,
   call stream_rewind(stream) or stream_rseek(stream, offset) to position
   the stream at the beginning or at a particular bit offset.  Conversely,
   stream_rewind(stream) or stream_wseek(stream, offset) positions the
   stream for writing.  In read mode, the following functions may be called:

     size_t stream_size(stream);
     bitstream_offset stream_rtell(stream);
     void stream_rewind(stream);
     void stream_rseek(stream, offset);
     void stream_skip(stream, n);
     bitstream_count stream_align(stream);
     uint stream_read_bit(stream);
     uint64 stream_read_bits(stream, n);

   Each of the above read calls has a corresponding write call:

     size_t stream_size(stream);
     bitstream_offset stream_wtell(stream);
     void stream_rewind(stream);
     void stream_wseek(stream, offset);
     void stream_pad(stream, n);
     bitstream_count stream_flush(stream);
     uint stream_write_bit(stream, bit);
     uint64 stream_write_bits(stream, value, n);

3. The stream buffer is an unsigned integer of a user-specified type given
   by the BIT_STREAM_WORD_TYPE macro.  Bits are read and written in units of
   this integer word type.  Supported types are 8, 16, 32, or 64 bits wide.
   The bit width of the buffer is denoted by 'wsize' and can be accessed
   either via the global constant stream_word_bits or stream_alignment().
   A small wsize allows for fine granularity reads and writes, and may be
   preferable when working with many small blocks of data that require
   non-sequential access.  The default maximum size of 64 bits ensures maximum
   speed.  Note that even when wsize < 64, it is still possible to read and
   write up to 64 bits at a time using stream_read_bits() and
   stream_write_bits().

4. If BIT_STREAM_STRIDED is defined, words read from or written to the stream
   may be accessed noncontiguously by setting a power-of-two block size (which
   by default is one word) and a block stride (defaults to zero blocks).  The
   word pointer is always incremented by one word each time a word is accessed.
   Once advanced past a block boundary, the word pointer is also advanced by
   the stride to the next block.  This feature may be used to store blocks of
   data interleaved, e.g., for progressive coding or for noncontiguous parallel
   access to the bit stream  Note that the block size is measured in words,
   while the stride is measured in multiples of the block size.  Strided access
   can have a significant performance penalty.

5. Multiple bits are read and written in order of least to most significant
   bit.  Thus, the statement

       value = stream_write_bits(stream, value, n);

   is essentially equivalent to (but faster than)

       for (i = 0; i < n; i++, value >>= 1)
         stream_write_bit(stream, value & 1);

   when 0 <= n <= 64.  The same holds for read calls, and thus

       value = stream_read_bits(stream, n);

   is essentially equivalent to

       for (i = 0, value = 0; i < n; i++)
         value += (uint64)stream_read_bit(stream) << i;

   Note that it is possible to write fewer bits than the argument 'value'
   holds (possibly even no bits), in which case any unwritten bits are
   shifted right to the least significant position and returned.  That is,
   value = stream_write_bits(stream, value, n); is equivalent to

       for (i = 0; i < n; i++)
         value = stream_write_bits(stream, value, 1);

6. Although the stream_wseek(stream, offset) call allows positioning the
   stream for writing at any bit offset without any data loss (i.e. all
   previously written bits preceding the offset remain valid), for efficiency
   the stream_flush(stream) operation will zero all bits up to the next
   multiple of wsize bits, thus overwriting bits that were previously stored
   at that location.  Consequently, random write access is effectively
   supported only at wsize granularity.  For sequential access, the largest
   possible wsize is preferred due to higher speed.

7. It is up to the user to adhere to these rules.  For performance reasons,
   no error checking is done, and in particular buffer overruns are not
   caught.
*/

// This file is from ZFP (https://github.com/LLNL/zfp/blob/develop/include/zfp/bitstream.inl)
// It is based on commit b2366c0 on Jul 6, 2022.
// All changes from the ZFP repo are denoted by Sam.

#include <climits>  // Sam
#include <cstddef>  // Sam
#include <cstdint>  // Sam
#include <cstdlib>  // Sam

#ifndef inline_
  #define inline_
#endif

// #include "zfp/bitstream.h"  // Sam removed it

namespace zfp {  // Sam

typedef unsigned int uint;          // Sam
typedef uint64_t uint64;            // Sam
typedef struct bitstream bitstream; // Sam: forward declaration of opaque type */
typedef uint64 bitstream_offset;    // Sam: bit offset into stream where bits are read/written
typedef bitstream_offset bitstream_size; // Sam: type for counting number of bits in a stream
typedef size_t bitstream_count; // Sam: type for counting a small number of bits in a stream

/* satisfy compiler when args unused */
#define unused_(x) ((void)(x))

/* bit stream word/buffer type; granularity of stream I/O operations */
#ifdef BIT_STREAM_WORD_TYPE
  /* may be 8-, 16-, 32-, or 64-bit unsigned integer type */
  typedef BIT_STREAM_WORD_TYPE bitstream_word;
#else
  /* use maximum word size by default for highest speed */
  typedef uint64 bitstream_word;
#endif

/* number of bits in a buffered word */
#define wsize ((bitstream_count)(sizeof(bitstream_word) * CHAR_BIT))

/* bit stream structure (opaque to caller) */
struct bitstream {
  bitstream_count bits;  /* number of buffered bits (0 <= bits < wsize) */
  bitstream_word buffer; /* incoming/outgoing bits (buffer < 2^bits) */
  bitstream_word* ptr;   /* pointer to next word to be read/written */
  bitstream_word* begin; /* beginning of stream */
  bitstream_word* end;   /* end of stream (not enforced) */
#ifdef BIT_STREAM_STRIDED
  size_t mask;           /* one less the block size in number of words */
  ptrdiff_t delta;       /* number of words between consecutive blocks */
#endif
};

/* private functions ------------------------------------------------------- */

/* read a single word from memory */
bitstream_word // Sam remove `static`
stream_read_word(bitstream* s)
{
  bitstream_word w = *s->ptr++;
#ifdef BIT_STREAM_STRIDED
  if (!((s->ptr - s->begin) & s->mask))
    s->ptr += s->delta;
#endif
  return w;
}

/* write a single word to memory */
void // Sam removed `static`
stream_write_word(bitstream* s, bitstream_word value)
{
  *s->ptr++ = value;
#ifdef BIT_STREAM_STRIDED
  if (!((s->ptr - s->begin) & s->mask))
    s->ptr += s->delta;
#endif
}

/* public functions -------------------------------------------------------- */

/* word size in bits (equals bitstream_word_bits) */
inline_ bitstream_count
stream_alignment()
{
  return wsize;
}

/* pointer to beginning of stream */
inline_ void*
stream_data(const bitstream* s)
{
  return s->begin;
}

/* current byte size of stream (if flushed) */
inline_ size_t
stream_size(const bitstream* s)
{
  return (size_t)(s->ptr - s->begin) * sizeof(bitstream_word);
}

/* byte capacity of stream */
inline_ size_t
stream_capacity(const bitstream* s)
{
  return (size_t)(s->end - s->begin) * sizeof(bitstream_word);
}

/* number of words per block */
inline_ size_t
stream_stride_block(const bitstream* s)
{
#ifdef BIT_STREAM_STRIDED
  return s->mask + 1;
#else
  unused_(s);
  return 1;
#endif
}

/* number of blocks between consecutive stream blocks */
inline_ ptrdiff_t
stream_stride_delta(const bitstream* s)
{
#ifdef BIT_STREAM_STRIDED
  return s->delta / (s->mask + 1);
#else
  unused_(s);
  return 0;
#endif
}

/* read single bit (0 or 1) */
inline_ uint
stream_read_bit(bitstream* s)
{
  uint bit;
  if (!s->bits) {
    s->buffer = stream_read_word(s);
    s->bits = wsize;
  }
  s->bits--;
  bit = (uint)s->buffer & 1u;
  s->buffer >>= 1;
  return bit;
}

/* write single bit (must be 0 or 1) */
inline_ uint
stream_write_bit(bitstream* s, uint bit)
{
  s->buffer += (bitstream_word)bit << s->bits;
  if (++s->bits == wsize) {
    stream_write_word(s, s->buffer);
    s->buffer = 0;
    s->bits = 0;
  }
  return bit;
}

/* read 0 <= n <= 64 bits */
inline_ uint64
stream_read_bits(bitstream* s, bitstream_count n)
{
  uint64 value = s->buffer;
  if (s->bits < n) {
    /* keep fetching wsize bits until enough bits are buffered */
    do {
      /* assert: 0 <= s->bits < n <= 64 */
      s->buffer = stream_read_word(s);
      value += (uint64)s->buffer << s->bits;
      s->bits += wsize;
    } while (sizeof(s->buffer) < sizeof(value) && s->bits < n);
    /* assert: 1 <= n <= s->bits < n + wsize */
    s->bits -= n;
    if (!s->bits) {
      /* value holds exactly n bits; no need for masking */
      s->buffer = 0;
    }
    else {
      /* assert: 1 <= s->bits < wsize */
      s->buffer >>= wsize - s->bits;
      /* assert: 1 <= n <= 64 */
      value &= ((uint64)2 << (n - 1)) - 1;
    }
  }
  else {
    /* assert: 0 <= n <= s->bits < wsize <= 64 */
    s->bits -= n;
    s->buffer >>= n;
    value &= ((uint64)1 << n) - 1;
  }
  return value;
}

/* write 0 <= n <= 64 low bits of value and return remaining bits */
inline_ uint64
stream_write_bits(bitstream* s, uint64 value, bitstream_count n)
{
  /* append bit string to buffer */
  s->buffer += (bitstream_word)(value << s->bits);
  s->bits += n;
  /* is buffer full? */
  if (s->bits >= wsize) {
    /* 1 <= n <= 64; decrement n to ensure valid right shifts below */
    value >>= 1;
    n--;
    /* assert: 0 <= n < 64; wsize <= s->bits <= wsize + n */
    do {
      /* output wsize bits while buffer is full */
      s->bits -= wsize;
      /* assert: 0 <= s->bits <= n */
      stream_write_word(s, s->buffer);
      /* assert: 0 <= n - s->bits < 64 */
      s->buffer = (bitstream_word)(value >> (n - s->bits));
    } while (sizeof(s->buffer) < sizeof(value) && s->bits >= wsize);
  }
  /* assert: 0 <= s->bits < wsize */
  s->buffer &= ((bitstream_word)1 << s->bits) - 1;
  /* assert: 0 <= n < 64 */
  return value >> n;
}

/* return bit offset to next bit to be read */
inline_ bitstream_offset
stream_rtell(const bitstream* s)
{
  return (bitstream_offset)(s->ptr - s->begin) * wsize - s->bits;
}

/* return bit offset to next bit to be written */
inline_ bitstream_offset
stream_wtell(const bitstream* s)
{
  return (bitstream_offset)(s->ptr - s->begin) * wsize + s->bits;
}

/* position stream for reading or writing at beginning */
inline_ void
stream_rewind(bitstream* s)
{
  s->ptr = s->begin;
  s->buffer = 0;
  s->bits = 0;
}

/* position stream for reading at given bit offset */
inline_ void
stream_rseek(bitstream* s, bitstream_offset offset)
{
  bitstream_count n = (bitstream_count)(offset % wsize);
  s->ptr = s->begin + (size_t)(offset / wsize);
  if (n) {
    s->buffer = stream_read_word(s) >> n;
    s->bits = wsize - n;
  }
  else {
    s->buffer = 0;
    s->bits = 0;
  }
}

/* position stream for writing at given bit offset */
inline_ void
stream_wseek(bitstream* s, bitstream_offset offset)
{
  bitstream_count n = (bitstream_count)(offset % wsize);
  s->ptr = s->begin + (size_t)(offset / wsize);
  if (n) {
    bitstream_word buffer = *s->ptr;
    buffer &= ((bitstream_word)1 << n) - 1;
    s->buffer = buffer;
    s->bits = n;
  }
  else {
    s->buffer = 0;
    s->bits = 0;
  }
}

/* skip over the next n bits (n >= 0) */
inline_ void
stream_skip(bitstream* s, bitstream_size n)
{
  stream_rseek(s, stream_rtell(s) + n);
}

/* append n zero-bits to stream (n >= 0) */
inline_ void
stream_pad(bitstream* s, bitstream_size n)
{
  bitstream_offset bits = s->bits;
  for (bits += n; bits >= wsize; bits -= wsize) {
    stream_write_word(s, s->buffer);
    s->buffer = 0;
  }
  s->bits = (bitstream_count)bits;
}

/* align stream on next word boundary */
inline_ bitstream_count
stream_align(bitstream* s)
{
  bitstream_count bits = s->bits;
  if (bits)
    stream_skip(s, bits);
  return bits;
}

/* write any remaining buffered bits and align stream on next word boundary */
inline_ bitstream_count
stream_flush(bitstream* s)
{
  bitstream_count bits = (wsize - s->bits) % wsize;
  if (bits)
    stream_pad(s, bits);
  return bits;
}

/* copy n bits from one bit stream to another */
inline_ void
stream_copy(bitstream* dst, bitstream* src, bitstream_size n)
{
  while (n > wsize) {
    bitstream_word w = (bitstream_word)stream_read_bits(src, wsize);
    stream_write_bits(dst, w, wsize);
    n -= wsize;
  }
  if (n) {
    bitstream_word w = (bitstream_word)stream_read_bits(src, (bitstream_count)n);
    stream_write_bits(dst, w, (bitstream_count)n);
  }
}

#ifdef BIT_STREAM_STRIDED
/* set block size in number of words and spacing in number of blocks */
inline_ int
stream_set_stride(bitstream* s, size_t block, ptrdiff_t delta)
{
  /* ensure block size is a power of two */
  if (block & (block - 1))
    return 0;
  s->mask = block - 1;
  s->delta = delta * block;
  return 1;
}
#endif

/* allocate and initialize bit stream to user-allocated buffer */
inline_ bitstream*
stream_open(void* buffer, size_t bytes)
{
  bitstream* s = (bitstream*)std::malloc(sizeof(bitstream));  // Sam added std
  if (s) {
    s->begin = (bitstream_word*)buffer;
    s->end = s->begin + bytes / sizeof(bitstream_word);
#ifdef BIT_STREAM_STRIDED
    stream_set_stride(s, 0, 0);
#endif
    stream_rewind(s);
  }
  return s;
}

/* close and deallocate bit stream */
inline_ void
stream_close(bitstream* s)
{
  std::free(s);  // Sam added std
}

/* make a copy of bit stream to shared memory buffer */
inline_ bitstream*
stream_clone(const bitstream* s)
{
  bitstream* c = (bitstream*)std::malloc(sizeof(bitstream));  // Sam added std
  if (c)
    *c = *s;
  return c;
}

// Sam addition: test if a range contains at least one "1" bit
inline_ uint
stream_test_range(bitstream* s, bitstream_offset start_pos, bitstream_offset range_len)
{
  stream_rseek(s, start_pos);

  /* step 1: test the buffered word */
  const bitstream_count buf_bit_num = s->bits;
  const bitstream_count num_to_test = buf_bit_num < range_len ? buf_bit_num : range_len;
  uint64 value = stream_read_bits(s, num_to_test);
  if (value != 0u)
    return 1u;

  /* step 2: test the whole integers */
  const bitstream_count no_buf_bit_num = range_len > buf_bit_num ? range_len - buf_bit_num : 0;
  const bitstream_count whole_int_num = no_buf_bit_num / wsize;
  bitstream_count idx = 0;
  for (idx = 0; idx < whole_int_num; idx++) {
    if (stream_read_word(s) != 0u)
      return 1u;
  }

  /* step 3: test the remaining bits */
  const bitstream_count remaining_bit_num = no_buf_bit_num % wsize;
  value = stream_read_bits(s, remaining_bit_num);
  return (value != 0u ? 1u : 0u);
}

// Sam addition: write a bit (must be 0 or 1) at a random position and flush (in a safe manner).
inline_ uint
stream_random_write(bitstream* s, uint bit, bitstream_offset pos)
{
  const bitstream_offset wstart = pos / wsize;
  const bitstream_offset wremaining = pos % wsize;

  s->ptr = s->begin + wstart;
  bitstream_word buffer = *s->ptr;
  const bitstream_word mask = (bitstream_word)1u << wremaining;
  if (bit)
    buffer |= mask;
  else
    buffer &= ~mask;
  stream_write_word(s, buffer);

  return bit;
}
// Finish Sam addition

#undef unused_

};  // Sam: namespace zfp
