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
#include "mruby/data.h"
#include "compat.h"
#include "buffer.h"
#include "buffer_class.h"

struct RClass *cMessagePack_Buffer;

static mrb_sym s_read;
static mrb_sym s_readpartial;
static mrb_sym s_write;
static mrb_sym s_append;
static mrb_sym s_close;

#if 0
static void
mrb_msgpack_buffer_t_free(mrb_state *mrb, void *ptr) {
  mrb_free(mrb, ptr);
}
#endif

#if 0
static struct mrb_data_type mrb_msgpack_buffer_t = {"msgpack_buffer_t", mrb_msgpack_buffer_t_free};

#define BUFFER(mrb, from, name) \
  msgpack_buffer_t *name = NULL; \
  Data_Get_Struct(mrb, from, &mrb_msgpack_buffer_t, name); \
  if (name == NULL) { \
    mrb_raisef(mrb, E_ARGUMENT_ERROR, "NULL found for %S when shouldn't be.", name); \
  }

#define CHECK_STRING_TYPE(mrb, value) \
  value = mrb_check_string_type(mrb, value); \
  if (mrb_nil_p(value)) { \
    mrb_raise(mrb, E_ARGUMENT_ERROR, "instance of String needed"); \
  }
#endif

#if 0
static void Buffer_free(mrb_state *mrb, void* data)
{
  if (data == NULL) {
    return;
  }
  msgpack_buffer_t* b = (msgpack_buffer_t*) data;
  msgpack_buffer_destroy(mrb, b);
  mrb_free(mrb, b);
}
#endif

#if 0
static mrb_value Buffer_alloc(mrb_state *mrb, mrb_value klass)
{
    msgpack_buffer_t* b = mrb_malloc(mrb, sizeof(msgpack_buffer_t));
    //msgpack_buffer_t* b = mrb_malloc(mrb, msgpack_buffer_t, 1);
    msgpack_buffer_init(b);

    //return Data_Wrap_Struct(klass, msgpack_buffer_mark, Buffer_free, b);
    return klass; // dummy
}
#endif

#if 0
static mrb_sym
get_partial_read_method(mrb_state *mrb, mrb_value io)
{
  if (mrb_respond_to(mrb, io, s_readpartial)) {
    return s_readpartial;
  } else if (mrb_respond_to(mrb, io, s_read)) {
    return s_read;
  } else {
    return s_read;
  }
}
#endif

#if 0
static mrb_sym get_write_all_method(mrb_state* mrb, mrb_value io)
{
  if (mrb_respond_to(mrb, io, s_write)) {
    return s_write;
  } else if(mrb_respond_to(mrb, io, s_append)) {
    return s_append;
  } else {
    return s_write;
  }
}
#endif

void
MessagePack_Buffer_initialize(mrb_state* mrb, msgpack_buffer_t* b, mrb_value io, mrb_value options)
{
#if 0
  b->io = io;
  b->io_partial_read_method = get_partial_read_method(mrb, io);
  b->io_write_all_method = get_write_all_method(mrb, io);
#endif

#if 0
  if (!mrb_nil_p(options)) {
    mrb_value v;
        v = rb_hash_aref(options, ID2SYM(mrb_intern("read_reference_threshold")));
        if (!mrb_nil_p(v)) {
            msgpack_buffer_set_read_reference_threshold(b, NUM2ULONG(v));
        }

        v = rb_hash_aref(options, ID2SYM(rb_intern2("write_reference_threshold")));
        if (!mrb_nil_p(v)) {
            msgpack_buffer_set_write_reference_threshold(b, NUM2ULONG(v));
        }

        v = rb_hash_aref(options, ID2SYM(rb_intern("io_buffer_size")));
        if(!mrb_nil_p(v)) {
            msgpack_buffer_set_io_buffer_size(b, NUM2ULONG(v));
        }
    }
#endif
}

mrb_value MessagePack_Buffer_wrap(mrb_state *mrb, msgpack_buffer_t* b, mrb_value owner)
{
  b->owner = owner;
  //return Data_Wrap_Struct(mrb, cMessagePack_Buffer, msgpack_buffer_mark, NULL, b);
  return mrb_nil_value();
}

#if 0
static mrb_value Buffer_initialize(mrb_state *mrb, int argc, mrb_value* argv, mrb_value self)
{
  mrb_value io = mrb_nil_value();
  mrb_value options = mrb_nil_value();

  if (argc == 0 || (argc == 1 && mrb_nil_p(argv[0]))) {
        /* Qnil */
  } else if(argc == 1) {
    mrb_value v = argv[0];
    if (mrb_type(v) == MRB_TT_HASH) {
      options = v;
    } else {
      io = v;
    }

  } else if (argc == 2) {
    io = argv[0];
    options = argv[1];
    if (mrb_type(options) != MRB_TT_HASH) {
      mrb_raisef(mrb, E_ARGUMENT_ERROR, "expected Hash but found %S.", mrb_obj_classname(mrb, io));
    }

  } else {
    mrb_raisef(mrb, E_ARGUMENT_ERROR, "wrong number of arguments (%d for 0..1)", argc);
  }

    //BUFFER(mrb, self, b);

    //MessagePack_Buffer_initialize(mrb, b, io, options);

    return self;
}
#endif

#if 0
static mrb_value Buffer_clear(mrb_value self)
{
  BUFFER(self, b);
  msgpack_buffer_clear(b);
  return mrb_nil_value();
}

static mrb_value Buffer_size(mrb_value self)
{
  BUFFER(self, b);
  size_t size = msgpack_buffer_all_readable_size(b);
  return SIZET2NUM(size);
}

static mrb_value Buffer_empty_p(mrb_value self)
{
    BUFFER(self, b);
    if(msgpack_buffer_top_readable_size(b) == 0) {
        return Qtrue;
    } else {
        return Qfalse;
    }
}

static mrb_value Buffer_write(mrb_value self, mrb_value string_or_buffer)
{
    BUFFER(self, b);

    VALUE string = string_or_buffer;  // TODO optimize if string_or_buffer is a Buffer
    StringValue(string);

    size_t length = msgpack_buffer_append_string(b, string);

    return SIZET2NUM(length);
}

static mrb_value Buffer_append(mrb_value self, mrb_value string_or_buffer)
{
    BUFFER(self, b);

    mrb_value string = string_or_buffer;  // TODO optimize if string_or_buffer is a Buffer
    StringValue(string);

    msgpack_buffer_append_string(b, string);

    return self;
}


#define MAKE_EMPTY_STRING(orig) \
  if (!mrb_nil_p(orig)) { \
        orig = rb_str_buf_new(0); \
    } else { \
        rb_str_resize(orig, 0); \
    }

static mrb_value read_until_eof_rescue(mrb_value args)
{
    msgpack_buffer_t* b = (void*) ((VALUE*) args)[0];
    VALUE out = ((VALUE*) args)[1];
    unsigned long max = ((VALUE*) args)[2];
    size_t* sz = (void*) ((VALUE*) args)[3];

    while(true) {
        size_t rl;
        if(max == 0) {
            if(out == Qnil) {
                rl = msgpack_buffer_skip(b, b->io_buffer_size);
            } else {
                rl = msgpack_buffer_read_to_string(b, out, b->io_buffer_size);
            }
            if(rl == 0) {
                break;
            }
            *sz += rl;

        } else {
            if(out == Qnil) {
                rl = msgpack_buffer_skip(b, max);
            } else {
                rl = msgpack_buffer_read_to_string(b, out, max);
            }
            if(rl == 0) {
                break;
            }
            *sz += rl;
            if(max <= rl) {
                break;
            } else {
                max -= rl;
            }
        }
    }

    return mrb_nil_value();
}
#endif

#if 0
static mrb_value read_until_eof_error(mrb_value args)
{
  /* ignore EOFError */
  UNUSED(args);
  return mrb_nil_value();
}
#endif

#if 0
static inline size_t read_until_eof(msgpack_buffer_t* b, mrb_value out, unsigned long max)
{
    if (msgpack_buffer_has_io(b)) {
        size_t sz = 0;
        VALUE args[4] = { (VALUE)(void*) b, out, (VALUE) max, (VALUE)(void*) &sz };
        rb_rescue2(read_until_eof_rescue, (VALUE)(void*) args,
                read_until_eof_error, (VALUE)(void*) args,
                rb_eEOFError, NULL);
        return sz;

    } else {
        if (max == 0) {
            max = ULONG_MAX;
        }
        if (!mrb_nil_p(out)) {
            return msgpack_buffer_skip_nonblock(b, max);
        } else {
            return msgpack_buffer_read_to_string_nonblock(b, out, max);
        }
    }
}
#endif

#if 0
static inline mrb_value read_all(mrb_state *mrb, msgpack_buffer_t* b, mrb_value out)
{
#ifndef DISABLE_BUFFER_READ_TO_S_OPTIMIZE
    if (!mrb_nil_p(out) && !msgpack_buffer_has_io(b)) {
        /* same as to_s && clear; optimize */
        mrb_value str = msgpack_buffer_all_as_string(mrb, b);
        msgpack_buffer_clear(mrb, b);
        return str;
    }
#endif
  //MAKE_EMPTY_STRING(out);
  //read_until_eof(b, out, 0);
  return out;
}
#endif

#if 0
static mrb_value Buffer_skip(mrb_state *mrb, mrb_value self, mrb_value sn)
{
  //BUFFER(mrb, self, b);

  //unsigned long n = FIX2ULONG(sn);
  unsigned long n = 0;

  /* do nothing */
  if (n == 0) {
    //return ULONG2NUM(0);
    return mrb_nil_value();
  }

  //size_t sz = read_until_eof(b, mrb_nil_value(), n);
  //return ULONG2NUM(sz);
  return mrb_nil_value();
}
#endif

#if 0
static mrb_value Buffer_skip_all(mrb_state *mrb, mrb_value self, mrb_value sn)
{
//    BUFFER(mrb, self, b);

    //unsigned long n = FIX2ULONG(sn);
    unsigned long n = 0;

    /* do nothing */
    if(n == 0) {
        return self;
    }

#if 0
    if(!msgpack_buffer_ensure_readable(b, n)) {
        //rb_raise(rb_eEOFError, "end of buffer reached");
    }
#endif

    //msgpack_buffer_skip_nonblock(b, n);

  return self;
}
#endif

#if 0
static mrb_value Buffer_read_all(mrb_state *mrb, int argc, mrb_value* argv, mrb_value self)
{
    mrb_value out = mrb_nil_value();
    unsigned long n = 0;
    mrb_bool all = FALSE;

    switch(argc) {
    case 2:
        out = argv[1];
        /* pass through */
    case 1:
        //n = FIX2ULONG(argv[0]);
        n = 0;
        break;
    case 0:
        all = TRUE;
        break;
    default:
        mrb_raisef(mrb, E_ARGUMENT_ERROR, "wrong number of arguments (%d for 0..2)", argc);
    }

    //BUFFER(self, b);

    if (!mrb_nil_p(out)) {
        //CHECK_STRING_TYPE(out);
    }

  if (all) {
    //return read_all(b, out);
    return mrb_nil_value();
  }

  if (n == 0) {
    /* do nothing */
    ///MAKE_EMPTY_STRING(out);
    return out;
  }

#if 0
  if (!msgpack_buffer_ensure_readable(b, n)) {
    //rb_raise(rb_eEOFError, "end of buffer reached");
  }
#endif
    //MAKE_EMPTY_STRING(out);
    //msgpack_buffer_read_to_string_nonblock(b, out, n);

    return out;
}
#endif

#if 0
static mrb_value Buffer_read(mrb_state *mrb, int argc, mrb_value* argv, mrb_value self)
{
    mrb_value out = mrb_nil_value();
    unsigned long n = -1;
    mrb_bool all = FALSE;

    switch(argc) {
    case 2:
        out = argv[1];
        /* pass through */
    case 1:
        //n = FIX2ULONG(argv[0]);
        n = 0;
        break;
    case 0:
        all = TRUE;
        break;
    default:
        mrb_raisef(mrb, E_ARGUMENT_ERROR, "wrong number of arguments (%d for 0..2)", argc);
    }

    //BUFFER(self, b);

    if (!mrb_nil_p(out)) {
        //CHECK_STRING_TYPE(out);
    }

    if (all) {
        //return read_all(b, out);
        return mrb_nil_value();
    }

    if (n == 0) {
        /* do nothing */
        //MAKE_EMPTY_STRING(out);
        return out;
    }
#if 0
#ifndef DISABLE_BUFFER_READ_TO_S_OPTIMIZE
    if(!msgpack_buffer_has_io(b) && !mrb_nil_p(out) &&
            msgpack_buffer_all_readable_size(b) <= n) {
        // same as to_s && clear; optimize
        mrb_value str = msgpack_buffer_all_as_string(b);
        msgpack_buffer_clear(b);

        if(RSTRING_LEN(str) == 0) {
            return mrb_nil_value();
        } else {
            return str;
        }
    }
#endif
#endif

  //MAKE_EMPTY_STRING(out);
  //read_until_eof(b, out, n);

  if (RSTRING_LEN(out) == 0) {
    return mrb_nil_value();
  } else {
    return out;
  }
}
#endif

#if 0
static mrb_value Buffer_to_str(mrb_value self)
{
  BUFFER(self, b);
  return msgpack_buffer_all_as_string(b);
}

static mrb_value Buffer_to_a(mrb_value self)
{
  BUFFER(self, b);
  return msgpack_buffer_all_as_string_array(b);
}

static mrb_value Buffer_flush(mrb_value self)
{
  BUFFER(self, b);
  msgpack_buffer_flush(b);
  return self;
}

static mrb_value Buffer_io(mrb_value self)
{
  BUFFER(self, b);
  return b->io;
}

static mrb_value Buffer_close(mrb_value self)
{
  BUFFER(mrb, self, b);
  if (!mrb_nil_p(b->io)) {
    return rb_funcall(b->io, s_close, 0);
  }
  return mrb_nil_value();
}

static mrb_value Buffer_write_to(mrb_value self, mrb_value io)
{
    BUFFER(self, b);
    size_t sz = msgpack_buffer_flush_to_io(b, io, s_write, true);
    return ULONG2NUM(sz);
}
#endif

void MessagePack_Buffer_module_init(mrb_state *mrb, struct RClass *mMessagePack)
{
  s_read = mrb_intern_lit(mrb, "read");
  s_readpartial = mrb_intern_lit(mrb, "readpartial");
  s_write = mrb_intern_lit(mrb, "write");
  s_append = mrb_intern_lit(mrb, "<<");
  s_close = mrb_intern_lit(mrb, "close");

  msgpack_buffer_static_init();

  cMessagePack_Buffer = mrb_define_class_under(mrb, mMessagePack, "Buffer", mrb->object_class);

#if 0
  mrb_define_alloc_func(cMessagePack_Buffer, Buffer_alloc);

  mrb_define_method(cMessagePack_Buffer, "initialize", Buffer_initialize, MRB_ARGS_ANY());
  mrb_define_method(cMessagePack_Buffer, "clear", Buffer_clear, MRB_ARGS_NONE());
  mrb_define_method(cMessagePack_Buffer, "size", Buffer_size, MRB_ARGS_NONE());
  mrb_define_method(cMessagePack_Buffer, "empty?", Buffer_empty_p, MRB_ARGS_NONE());
  mrb_define_method(cMessagePack_Buffer, "write", Buffer_write, MRB_ARGS_REQ(1));
  mrb_define_method(cMessagePack_Buffer, "<<", Buffer_append, MRB_ARGS_REQ(1));
  mrb_define_method(cMessagePack_Buffer, "skip", Buffer_skip, MRB_ARGS_REQ(1));
  mrb_define_method(cMessagePack_Buffer, "skip_all", Buffer_skip_all, MRB_ARGS_REQ(1));
  mrb_define_method(cMessagePack_Buffer, "read", Buffer_read, MRB_ARGS_ANY());
  mrb_define_method(cMessagePack_Buffer, "read_all", Buffer_read_all, MRB_ARGS_ANY());
  mrb_define_method(cMessagePack_Buffer, "io", Buffer_io, MRB_ARGS_NONE());
  mrb_define_method(cMessagePack_Buffer, "flush", Buffer_flush, MRB_ARGS_NONE());
  mrb_define_method(cMessagePack_Buffer, "close", Buffer_close, MRB_ARGS_NONE());
  mrb_define_method(cMessagePack_Buffer, "write_to", Buffer_write_to, MRB_ARGS_REQ(1));
  mrb_define_method(cMessagePack_Buffer, "to_str", Buffer_to_str, MRB_ARGS_NONE());
  mrb_define_alias(cMessagePack_Buffer, "to_s", "to_str");
  mrb_define_method(cMessagePack_Buffer, "to_a", Buffer_to_a, MRB_ARGS_NONE());
#endif
}

