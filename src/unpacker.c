/*
 * The original is https://github.com/msgpack/msgpack-ruby
 * MessagePack for Ruby
 *
 * Copyright (C) 2008-2012 FURUHASHI Sadayuki
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */
#include "mruby.h"
#include "mruby/array.h"
#include "mruby/hash.h"
#include "unpacker.h"
#include "rmem.h"

#if !defined(DISABLE_RMEM) && !defined(DISABLE_UNPACKER_STACK_RMEM) && \
  MSGPACK_UNPACKER_STACK_CAPACITY * MSGPACK_UNPACKER_STACK_SIZE <= MSGPACK_RMEM_PAGE_SIZE
#define UNPACKER_STACK_RMEM
#endif

#ifdef UNPACKER_STACK_RMEM
static msgpack_rmem_t s_stack_rmem;
#endif

#ifdef COMPAT_HAVE_ENCODING  /* see compat.h*/
static int s_enc_utf8;
#endif

void msgpack_unpacker_static_init(mrb_state *mrb)
{
#ifdef UNPACKER_STACK_RMEM
  msgpack_rmem_init(mrb, &s_stack_rmem);
#endif

#ifdef COMPAT_HAVE_ENCODING
  s_enc_utf8 = rb_utf8_encindex();
#endif
}

void msgpack_unpacker_static_destroy(mrb_state *mrb)
{
#ifdef UNPACKER_STACK_RMEM
  msgpack_rmem_destroy(mrb, &s_stack_rmem);
#endif
}

#define HEAD_BYTE_REQUIRED 0xc6

void msgpack_unpacker_init(mrb_state *mrb, msgpack_unpacker_t* uk)
{
  memset(uk, 0, sizeof(msgpack_unpacker_t));

  msgpack_buffer_init(UNPACKER_BUFFER_(uk));

  uk->head_byte = HEAD_BYTE_REQUIRED;

  uk->last_object = mrb_nil_value();
  uk->reading_raw = mrb_nil_value();

#ifdef UNPACKER_STACK_RMEM
  uk->stack = msgpack_rmem_alloc(mrb, &s_stack_rmem);
  /*memset(uk->stack, 0, MSGPACK_UNPACKER_STACK_CAPACITY);*/
#else
  /*uk->stack = calloc(MSGPACK_UNPACKER_STACK_CAPACITY, sizeof(msgpack_unpacker_stack_t));*/
  uk->stack = mrb_malloc(mrb, MSGPACK_UNPACKER_STACK_CAPACITY * sizeof(msgpack_unpacker_stack_t));
#endif
  uk->stack_capacity = MSGPACK_UNPACKER_STACK_CAPACITY;
}

void msgpack_unpacker_destroy(mrb_state *mrb, msgpack_unpacker_t* uk)
{
#ifdef UNPACKER_STACK_RMEM
  msgpack_rmem_free(mrb, &s_stack_rmem, uk->stack);
#else
  mrb_free(mrb, uk->stack);
#endif

  msgpack_buffer_destroy(mrb, UNPACKER_BUFFER_(uk));
}

void msgpack_unpacker_mark(mrb_state *mrb, msgpack_unpacker_t* uk)
{
  mrb_gc_mark_value(mrb, uk->last_object);
  mrb_gc_mark_value(mrb, uk->reading_raw);

  msgpack_unpacker_stack_t* s = uk->stack;
  msgpack_unpacker_stack_t* send = uk->stack + uk->stack_depth;
  for(; s < send; s++) {
    mrb_gc_mark_value(mrb, s->object);
    mrb_gc_mark_value(mrb, s->key);
  }

  /* See MessagePack_Buffer_wrap */
  /* msgpack_buffer_mark(UNPACKER_BUFFER_(uk)); */
  mrb_gc_mark_value(mrb, uk->buffer_ref);
}

void
msgpack_unpacker_reset(mrb_state *mrb, msgpack_unpacker_t* uk)
{
  msgpack_buffer_clear(mrb, UNPACKER_BUFFER_(uk));

  uk->head_byte = HEAD_BYTE_REQUIRED;

  /*memset(uk->stack, 0, sizeof(msgpack_unpacker_t) * uk->stack_depth);*/
  uk->stack_depth = 0;

  uk->last_object = mrb_nil_value();
  uk->reading_raw = mrb_nil_value();
  uk->reading_raw_remaining = 0;
}


/* head byte functions */
static
int read_head_byte(mrb_state *mrb, msgpack_unpacker_t* uk)
{
  int r = msgpack_buffer_read_1(mrb, UNPACKER_BUFFER_(uk));

  if (r == -1) {
    return PRIMITIVE_EOF;
  }
  return uk->head_byte = r;
}

static inline int
get_head_byte(mrb_state *mrb, msgpack_unpacker_t* uk)
{
  int b = uk->head_byte;

  if (b == HEAD_BYTE_REQUIRED) {
    b = read_head_byte(mrb, uk);
  }
  return b;
}

static inline void
reset_head_byte(msgpack_unpacker_t* uk)
{
  uk->head_byte = HEAD_BYTE_REQUIRED;
}

static inline int
object_complete(msgpack_unpacker_t* uk, mrb_value object)
{
  uk->last_object = object;
  reset_head_byte(uk);
  return PRIMITIVE_OBJECT_COMPLETE;
}

static inline int
object_complete_string(msgpack_unpacker_t* uk, mrb_value str)
{
  // TODO ruby 2.0 has String#b method
#ifdef COMPAT_HAVE_ENCODING
  //str_modifiable(str);
  ENCODING_SET(str, s_enc_utf8);
#endif
  return object_complete(uk, str);
}

/* stack funcs */
static inline msgpack_unpacker_stack_t*
_msgpack_unpacker_stack_top(msgpack_unpacker_t* uk)
{
  return &uk->stack[uk->stack_depth-1];
}

static inline int
_msgpack_unpacker_stack_push(msgpack_unpacker_t* uk, enum stack_type_t type, size_t count, mrb_value object)
{
  reset_head_byte(uk);

  if (uk->stack_capacity - uk->stack_depth <= 0) {
    return PRIMITIVE_STACK_TOO_DEEP;
  }

  msgpack_unpacker_stack_t* next = &uk->stack[uk->stack_depth];
  next->count = count;
  next->type = type;
  next->object = object;
  next->key = mrb_nil_value();

  uk->stack_depth++;
  return PRIMITIVE_CONTAINER_START;
}

//static inline mrb_value
static inline size_t
msgpack_unpacker_stack_pop(msgpack_unpacker_t* uk)
{
  return --uk->stack_depth;
}

static inline mrb_bool
msgpack_unpacker_stack_is_empty(msgpack_unpacker_t* uk)
{
  return uk->stack_depth == 0;
}

#ifdef USE_CASE_RANGE

#define SWITCH_RANGE_BEGIN(BYTE)     { switch(BYTE) {
#define SWITCH_RANGE(BYTE, FROM, TO) } case FROM ... TO: {
#define SWITCH_RANGE_DEFAULT         } default: {
#define SWITCH_RANGE_END             } }

#else

#define SWITCH_RANGE_BEGIN(BYTE)     { if(0) {
#define SWITCH_RANGE(BYTE, FROM, TO) } else if(FROM <= (BYTE) && (BYTE) <= TO) {
#define SWITCH_RANGE_DEFAULT         } else {
#define SWITCH_RANGE_END             } }

#endif


#define READ_CAST_BLOCK_OR_RETURN_EOF(mrb, cb, uk, n) \
  union msgpack_buffer_cast_block_t* cb = msgpack_buffer_read_cast_block(mrb, UNPACKER_BUFFER_(uk), n); \
  if (cb == NULL) { \
    return PRIMITIVE_EOF; \
  }

static inline mrb_bool
is_reading_map_key(msgpack_unpacker_t* uk)
{
  if (uk->stack_depth > 0) {
    msgpack_unpacker_stack_t* top = _msgpack_unpacker_stack_top(uk);
    if (top->type == STACK_TYPE_MAP_KEY) {
      return TRUE;
    }
  }
  return FALSE;
}

static int
read_raw_body_cont(mrb_state *mrb, msgpack_unpacker_t* uk)
{
  size_t length = uk->reading_raw_remaining;

  if (mrb_nil_p(uk->reading_raw)) {
    uk->reading_raw = mrb_str_buf_new(mrb, length);
  }

  do {
    size_t n = msgpack_buffer_read_to_string(mrb, UNPACKER_BUFFER_(uk), uk->reading_raw, length);
    if (n == 0) {
      return PRIMITIVE_EOF;
    }
    /* update reading_raw_remaining everytime because
     * msgpack_buffer_read_to_string raises IOError */
    uk->reading_raw_remaining = length = length - n;
  } while(length > 0);

  object_complete_string(uk, uk->reading_raw);
  uk->reading_raw = mrb_nil_value();
  return PRIMITIVE_OBJECT_COMPLETE;
}

static inline int
read_raw_body_begin(mrb_state *mrb, msgpack_unpacker_t* uk)
{
  /* assuming mrb_nil_p(uk->reading_raw) */

  /* try optimized read */
  size_t length = uk->reading_raw_remaining;
  if (length <= msgpack_buffer_top_readable_size(UNPACKER_BUFFER_(uk))) {
    /* don't use zerocopy for hash keys but get a frozen string directly
     * because rb_hash_aset freezes keys and it causes copying */
    mrb_bool frozen = is_reading_map_key(uk);
    mrb_value string = msgpack_buffer_read_top_as_string(mrb, UNPACKER_BUFFER_(uk), length, frozen);

    object_complete_string(uk, string);
    uk->reading_raw_remaining = 0;
    return PRIMITIVE_OBJECT_COMPLETE;
  }

  return read_raw_body_cont(mrb, uk);
}

static int
read_primitive(mrb_state *mrb, msgpack_unpacker_t* uk)
{
  int b;

  if (uk->reading_raw_remaining > 0) {
    return read_raw_body_cont(mrb, uk);
  }

  b = get_head_byte(mrb, uk);

  if (b < 0) {
    return b;
  }

  SWITCH_RANGE_BEGIN(b)
  SWITCH_RANGE(b, 0x00, 0x7f)  /* Positive Fixnum */
    //return object_complete(uk, INT2NUM(b));
    return object_complete(uk, mrb_fixnum_value(b));

  SWITCH_RANGE(b, 0xe0, 0xff)  /* Negative Fixnum */
    //return object_complete(uk, INT2NUM((int8_t)b));
    return object_complete(uk, mrb_fixnum_value((int8_t)b));
  SWITCH_RANGE(b, 0xa0, 0xbf)  /* FixRaw */
    int count = b & 0x1f;
    if (count == 0) {
      return object_complete_string(uk, mrb_str_buf_new(mrb, 0));
    }
    /* Read_raw_body_begin sets uk->reading_raw */
    uk->reading_raw_remaining = count;
    return read_raw_body_begin(mrb, uk);

  SWITCH_RANGE(b, 0x90, 0x9f)  /* FixArray */
    int count = b & 0x0f;
    if (count == 0) {
      return object_complete(uk, mrb_ary_new(mrb));
    }
    return _msgpack_unpacker_stack_push(uk, STACK_TYPE_ARRAY, count, mrb_ary_new_capa(mrb, count));

  SWITCH_RANGE(b, 0x80, 0x8f)  /* FixMap */
    int count = b & 0x0f;
    if (count == 0) {
      return object_complete(uk, mrb_hash_new(mrb));
    }
    return _msgpack_unpacker_stack_push(uk, STACK_TYPE_MAP_KEY, count*2, mrb_hash_new(mrb));

  SWITCH_RANGE(b, 0xc0, 0xdf)  /* Variable */
    switch(b) {
    case 0xc0:  /* nil */
      return object_complete(uk, mrb_nil_value());

    //case 0xc1:  /* string */

    case 0xc2:  /* false */
      return object_complete(uk, mrb_false_value());

    case 0xc3:  /* true */
      return object_complete(uk, mrb_true_value());

    //case 0xc4:
    //case 0xc5:
    //case 0xc6:
    //case 0xc7:
    //case 0xc8:
    //case 0xc9:

    case 0xca:  /* float */
      {
        READ_CAST_BLOCK_OR_RETURN_EOF(mrb, cb, uk, 4);
        cb->u32 = _msgpack_be_float(cb->u32);
        //return object_complete(uk, rb_float_new(cb->f));
        return object_complete(uk, mrb_float_value(mrb, cb->f));
      }

    case 0xcb:  /* double */
      {
        READ_CAST_BLOCK_OR_RETURN_EOF(mrb, cb, uk, 8);
        cb->u64 = _msgpack_be_double(cb->u64);
        //return object_complete(uk, rb_float_new(cb->d));
        return object_complete(uk, mrb_float_value(mrb, cb->d));
      }

    case 0xcc:  /* unsigned int  8 */
      {
        READ_CAST_BLOCK_OR_RETURN_EOF(mrb, cb, uk, 1);
        uint8_t u8 = cb->u8;
        //return object_complete(uk, INT2NUM((int)u8));
        return object_complete(uk, mrb_fixnum_value((int)u8));
      }

    case 0xcd:  /* unsigned int 16 */
      {
        READ_CAST_BLOCK_OR_RETURN_EOF(mrb, cb, uk, 2);
        uint16_t u16 = _msgpack_be16(cb->u16);
        //return object_complete(uk, INT2NUM((int)u16));
        return object_complete(uk, mrb_fixnum_value((int)u16));
      }

    case 0xce:  /* unsigned int 32 */
      {
        READ_CAST_BLOCK_OR_RETURN_EOF(mrb, cb, uk, 4);
        uint32_t u32 = _msgpack_be32(cb->u32);
        //return object_complete(uk, ULONG2NUM((unsigned long)u32));
        return object_complete(uk, mrb_fixnum_value((unsigned long)u32));
      }

    case 0xcf:  /* unsigned int 64 */
      {
        READ_CAST_BLOCK_OR_RETURN_EOF(mrb, cb, uk, 8);
        uint64_t u64 = _msgpack_be64(cb->u64);
        //return object_complete(uk, rb_ull2inum(u64));
        return object_complete(uk, mrb_fixnum_value(u64));
      }

    case 0xd0:  /* signed int  8 */
      {
        READ_CAST_BLOCK_OR_RETURN_EOF(mrb, cb, uk, 1);
        int8_t i8 = cb->i8;
        //return object_complete(uk, INT2NUM((int)i8));
        return object_complete(uk, mrb_fixnum_value((int)i8));
      }

    case 0xd1:  /* signed int 16 */
      {
        READ_CAST_BLOCK_OR_RETURN_EOF(mrb, cb, uk, 2);
        int16_t i16 = _msgpack_be16(cb->i16);
        //return object_complete(uk, INT2NUM((int)i16));
        return object_complete(uk, mrb_fixnum_value((int)i16));
      }

    case 0xd2:  /* signed int 32 */
      {
        READ_CAST_BLOCK_OR_RETURN_EOF(mrb, cb, uk, 4);
        int32_t i32 = _msgpack_be32(cb->i32);
        //return object_complete(uk, LONG2NUM((long)i32));
        return object_complete(uk, mrb_fixnum_value((long)i32));
      }

    case 0xd3:  /* signed int 64 */
      {
        READ_CAST_BLOCK_OR_RETURN_EOF(mrb, cb, uk, 8);
        int64_t i64 = _msgpack_be64(cb->i64);
        //return object_complete(uk, rb_ll2inum(i64));
        return object_complete(uk, mrb_fixnum_value(i64));
      }

    //case 0xd4:
    //case 0xd5:
    //case 0xd6:  /* big integer 16 */
    //case 0xd7:  /* big integer 32 */
    //case 0xd8:  /* big float 16 */
    //case 0xd9:  /* big float 32 */

    case 0xda:  /* raw 16 */
      {
        READ_CAST_BLOCK_OR_RETURN_EOF(mrb, cb, uk, 2);
        uint16_t count = _msgpack_be16(cb->u16);
        if (count == 0) {
          return object_complete_string(uk, mrb_str_buf_new(mrb, 0));
        }
        /* read_raw_body_begin sets uk->reading_raw */
        uk->reading_raw_remaining = count;
        return read_raw_body_begin(mrb, uk);
      }

    case 0xdb:  /* raw 32 */
      {
        READ_CAST_BLOCK_OR_RETURN_EOF(mrb, cb, uk, 4);
        uint32_t count = _msgpack_be32(cb->u32);
        if (count == 0) {
          return object_complete_string(uk, mrb_str_buf_new(mrb, 0));
        }
        /* read_raw_body_begin sets uk->reading_raw */
        uk->reading_raw_remaining = count;
        return read_raw_body_begin(mrb, uk);
      }

      case 0xdc:  /* array 16 */
        {
          READ_CAST_BLOCK_OR_RETURN_EOF(mrb, cb, uk, 2);
          uint16_t count = _msgpack_be16(cb->u16);
          if (count == 0) {
            return object_complete(uk, mrb_ary_new(mrb));
          }
          return _msgpack_unpacker_stack_push(uk, STACK_TYPE_ARRAY, count, mrb_ary_new_capa(mrb, count));
        }

        case 0xdd:  /* array 32 */
          {
            READ_CAST_BLOCK_OR_RETURN_EOF(mrb, cb, uk, 4);
            uint32_t count = _msgpack_be32(cb->u32);
            if(count == 0) {
              return object_complete(uk, mrb_ary_new(mrb));
            }
            return _msgpack_unpacker_stack_push(uk, STACK_TYPE_ARRAY, count, mrb_ary_new_capa(mrb, count));
          }

        case 0xde:  /* map 16 */
          {
            READ_CAST_BLOCK_OR_RETURN_EOF(mrb, cb, uk, 2);
            uint16_t count = _msgpack_be16(cb->u16);
            if (count == 0) {
              return object_complete(uk, mrb_hash_new(mrb));
            }
            return _msgpack_unpacker_stack_push(uk, STACK_TYPE_MAP_KEY, count*2, mrb_hash_new(mrb));
          }

        case 0xdf:  /* map 32 */
          {
            READ_CAST_BLOCK_OR_RETURN_EOF(mrb, cb, uk, 4);
            uint32_t count = _msgpack_be32(cb->u32);
            if (count == 0) {
              return object_complete(uk, mrb_hash_new(mrb));
            }
            return _msgpack_unpacker_stack_push(uk, STACK_TYPE_MAP_KEY, count*2, mrb_hash_new(mrb));
          }

        default:
          return PRIMITIVE_INVALID_BYTE;
        }

    SWITCH_RANGE_DEFAULT
        return PRIMITIVE_INVALID_BYTE;

    SWITCH_RANGE_END
}

int
msgpack_unpacker_read_array_header(mrb_state *mrb, msgpack_unpacker_t* uk, uint32_t* result_size)
{
  int b = get_head_byte(mrb, uk);

  if (b < 0) {
    return b;
  }

  if (0x90 < b && b < 0x9f) {
    *result_size = b & 0x0f;

  } else if (b == 0xdc) {
    /* array 16 */
    READ_CAST_BLOCK_OR_RETURN_EOF(mrb, cb, uk, 2);
    *result_size = _msgpack_be16(cb->u16);

  } else if(b == 0xdd) {
    /* array 32 */
    READ_CAST_BLOCK_OR_RETURN_EOF(mrb, cb, uk, 4);
    *result_size = _msgpack_be32(cb->u32);

  } else {
    return PRIMITIVE_UNEXPECTED_TYPE;
  }

  return 0;
}

int
msgpack_unpacker_read_map_header(mrb_state *mrb, msgpack_unpacker_t* uk, uint32_t* result_size)
{
  int b = get_head_byte(mrb, uk);

  if (b < 0) {
    return b;
  }

  if (0x80 < b && b < 0x8f) {
    *result_size = b & 0x0f;

  } else if(b == 0xde) {
    /* map 16 */
    READ_CAST_BLOCK_OR_RETURN_EOF(mrb, cb, uk, 2);
    *result_size = _msgpack_be16(cb->u16);

  } else if (b == 0xdf) {
    /* map 32 */
    READ_CAST_BLOCK_OR_RETURN_EOF(mrb, cb, uk, 4);
    *result_size = _msgpack_be32(cb->u32);

  } else {
    return PRIMITIVE_UNEXPECTED_TYPE;
  }

  return 0;
}

int
msgpack_unpacker_read(mrb_state *mrb, msgpack_unpacker_t* uk, size_t target_stack_depth)
{
  while (TRUE) {
    int r = read_primitive(mrb, uk);

    if (r < 0) {
      return r;
    }
    if (r == PRIMITIVE_CONTAINER_START) {
      continue;
    }
    /* PRIMITIVE_OBJECT_COMPLETE */

    if (msgpack_unpacker_stack_is_empty(uk)) {
      return PRIMITIVE_OBJECT_COMPLETE;
    }

    container_completed:
    {
      msgpack_unpacker_stack_t* top = _msgpack_unpacker_stack_top(uk);
      switch(top->type) {
        case STACK_TYPE_ARRAY:
          mrb_ary_push(mrb, top->object, uk->last_object);
          break;
        case STACK_TYPE_MAP_KEY:
          top->key = uk->last_object;
          top->type = STACK_TYPE_MAP_VALUE;
          break;
         case STACK_TYPE_MAP_VALUE:
            mrb_hash_set(mrb, top->object, top->key, uk->last_object);
            top->type = STACK_TYPE_MAP_KEY;
            break;
        }
        size_t count = --top->count;

        if (count == 0) {
          object_complete(uk, top->object);
          if (msgpack_unpacker_stack_pop(uk) <= target_stack_depth) {
            return PRIMITIVE_OBJECT_COMPLETE;
          }
          goto container_completed;
        }
    }
  }
}

int
msgpack_unpacker_skip(mrb_state *mrb, msgpack_unpacker_t* uk, size_t target_stack_depth)
{
  while (TRUE) {
    int r = read_primitive(mrb, uk);

    if (r < 0) {
      return r;
    }
    if (r == PRIMITIVE_CONTAINER_START) {
      continue;
    }
    /* PRIMITIVE_OBJECT_COMPLETE */

    if (msgpack_unpacker_stack_is_empty(uk)) {
      return PRIMITIVE_OBJECT_COMPLETE;
    }

    container_completed:
    {
      msgpack_unpacker_stack_t* top = _msgpack_unpacker_stack_top(uk);

      /* this section optimized out */
      /* TODO object_complete still creates objects which should be optimized out */

      size_t count = --top->count;

      if (count == 0) {
        object_complete(uk, mrb_nil_value());
        if (msgpack_unpacker_stack_pop(uk) <= target_stack_depth) {
          return PRIMITIVE_OBJECT_COMPLETE;
        }
        goto container_completed;
      }
    }
  }
}

int
msgpack_unpacker_peek_next_object_type(mrb_state *mrb, msgpack_unpacker_t* uk)
{
  int b = get_head_byte(mrb, uk);

  if (b < 0) {
    return b;
  }

  SWITCH_RANGE_BEGIN(b)
  SWITCH_RANGE(b, 0x00, 0x7f)  // Positive Fixnum
    return TYPE_INTEGER;

  SWITCH_RANGE(b, 0xe0, 0xff)  // Negative Fixnum
    return TYPE_INTEGER;

  SWITCH_RANGE(b, 0xa0, 0xbf)  // FixRaw
    return TYPE_RAW;

  SWITCH_RANGE(b, 0x90, 0x9f)  // FixArray
    return TYPE_ARRAY;

  SWITCH_RANGE(b, 0x80, 0x8f)  // FixMap
    return TYPE_MAP;

  SWITCH_RANGE(b, 0xc0, 0xdf)  // Variable
    switch(b) {
    case 0xc0:  // nil
      return TYPE_NIL;

    case 0xc2:  // false
    case 0xc3:  // true
      return TYPE_BOOLEAN;

    case 0xca:  // float
    case 0xcb:  // double
      return TYPE_FLOAT;

    case 0xcc:  // unsigned int  8
    case 0xcd:  // unsigned int 16
    case 0xce:  // unsigned int 32
    case 0xcf:  // unsigned int 64
      return TYPE_INTEGER;

    case 0xd0:  // signed int  8
    case 0xd1:  // signed int 16
    case 0xd2:  // signed int 32
    case 0xd3:  // signed int 64
      return TYPE_INTEGER;

    case 0xda:  // raw 16
    case 0xdb:  // raw 32
      return TYPE_RAW;

    case 0xdc:  // array 16
    case 0xdd:  // array 32
      return TYPE_ARRAY;

    case 0xde:  // map 16
    case 0xdf:  // map 32
      return TYPE_MAP;

    default:
      return PRIMITIVE_INVALID_BYTE;
    }

  SWITCH_RANGE_DEFAULT
    return PRIMITIVE_INVALID_BYTE;

  SWITCH_RANGE_END
}

mrb_bool
msgpack_unpacker_skip_nil(mrb_state *mrb, msgpack_unpacker_t* uk)
{
  int b = get_head_byte(mrb, uk);

  if (b < 0) {
    return b;
  }
  if (b == 0xc0) {
    return TRUE;
  }
  return FALSE;
}
