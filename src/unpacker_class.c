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
#include "mruby/class.h"
#include "mruby/data.h"
#include "mruby/variable.h"
#include "unpacker.h"
#include "unpacker_class.h"
#include "buffer_class.h"

struct RClass *cMessagePack_Unpacker;

static mrb_sym sym_unpacker_data;

static struct RClass *eUnpackError;
static struct RClass *eMalformedFormatError;
static struct RClass *eStackError;
static struct RClass *eTypeError;

static void Unpacker_free(mrb_state *mrb, void* uk)
{
  if (uk == NULL) {
    return;
  }
  msgpack_unpacker_destroy(mrb, (msgpack_unpacker_t *)uk);
  mrb_free(mrb, uk);
}

static struct mrb_data_type mrb_data_type_msgpack_unpacker_t = {"msgpack_unpacker_t", Unpacker_free};

#define UNPACKER(mrb, from, name) \
  msgpack_unpacker_t *name = NULL; \
  Data_Get_Struct(mrb, mrb_iv_get(mrb, from, sym_unpacker_data), &mrb_data_type_msgpack_unpacker_t, name); \
  if (name == NULL) { \
    mrb_raise(mrb, E_ARGUMENT_ERROR, "NULL found for " # name " when shouldn't be."); \
  }


static mrb_value
Unpacker_alloc(mrb_state *mrb, struct RClass *klass)
{
  mrb_value klass_value;
  mrb_value data;

  //msgpack_unpacker_t* uk = ALLOC_N(msgpack_unpacker_t, 1);
  msgpack_unpacker_t* uk = (msgpack_unpacker_t *)mrb_malloc(mrb, sizeof(msgpack_unpacker_t));

  msgpack_unpacker_init(mrb, uk);

  data = mrb_obj_value(Data_Wrap_Struct(mrb, klass, &mrb_data_type_msgpack_unpacker_t, (void*)uk));
  klass_value = mrb_obj_value(klass);
  mrb_iv_set(mrb, klass_value, sym_unpacker_data, data);
  uk->buffer_ref = MessagePack_Buffer_wrap(mrb, UNPACKER_BUFFER_(uk), data);

  return klass_value;
}

static mrb_value
Unpacker_initialize(mrb_state *mrb, mrb_value self)
{
  mrb_value io = mrb_nil_value();
  mrb_value options = mrb_nil_value();
  mrb_value *argv;
  int argc; /* Number of arguments */

  mrb_get_args(mrb, "*", &argv, &argc);
  if (argc == 0 || (argc == 1 && mrb_obj_eq(mrb, argv[0], mrb_nil_value()))) {
    /* mrb_nil_value() */
  } else if (argc == 1) {
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

  self = Unpacker_alloc(mrb, mrb_class_ptr(self));

  UNPACKER(mrb, self, uk);
  if (!mrb_nil_p(io) || !mrb_nil_p(options)) {
    MessagePack_Buffer_initialize(mrb, UNPACKER_BUFFER_(uk), io, options);
  }

  // TODO options

  return self;
}

static void
raise_unpacker_error(mrb_state *mrb, int r)
{
  switch(r) {
  case PRIMITIVE_EOF:
    //mrb_raise(mrb, rb_eEOFError, "end of buffer reached");
    mrb_raise(mrb, eUnpackError, "end of buffer reached");
    break;
  case PRIMITIVE_INVALID_BYTE:
    mrb_raise(mrb, eMalformedFormatError, "invalid byte");
    break;
  case PRIMITIVE_STACK_TOO_DEEP:
    mrb_raise(mrb, eStackError, "stack level too deep");
    break;
  case PRIMITIVE_UNEXPECTED_TYPE:
    mrb_raise(mrb, eTypeError, "unexpected type");
    break;
  default:
    mrb_raisef(mrb, eUnpackError, "logically unknown error %d", r);
    break;
  }
}

static mrb_value
Unpacker_buffer(mrb_state *mrb, mrb_value self)
{
  UNPACKER(mrb, self, uk);
  return uk->buffer_ref;
}

static mrb_value
Unpacker_read(mrb_state *mrb, mrb_value self)
{
  int r;

  UNPACKER(mrb, self, uk);

  r = msgpack_unpacker_read(mrb, uk, 0);
  if (r < 0) {
    raise_unpacker_error(mrb, r);
  }

  return msgpack_unpacker_get_last_object(uk);
}

static mrb_value
Unpacker_skip(mrb_state *mrb, mrb_value self)
{
  int r;
  UNPACKER(mrb, self, uk);

  r = msgpack_unpacker_skip(mrb, uk, 0);
  if (r < 0) {
    raise_unpacker_error(mrb, r);
  }

  return mrb_nil_value();
}

static mrb_value
Unpacker_skip_nil(mrb_state *mrb, mrb_value self)
{
  int r;
  UNPACKER(mrb, self, uk);

  r = msgpack_unpacker_skip_nil(mrb, uk);
  if (r < 0) {
    raise_unpacker_error(mrb, r);
  }

  if (r) {
    return mrb_true_value();
  }
  return mrb_false_value();
}

static mrb_value
Unpacker_read_array_header(mrb_state *mrb, mrb_value self)
{
  uint32_t size;
  int r;
  UNPACKER(mrb, self, uk);

  r = msgpack_unpacker_read_array_header(mrb, uk, &size);
  if (r < 0) {
    raise_unpacker_error(mrb, r);
  }

  return mrb_fixnum_value(size);
}

static mrb_value
Unpacker_read_map_header(mrb_state *mrb, mrb_value self)
{
  uint32_t size;
  int r;

  UNPACKER(mrb, self, uk);
  r = msgpack_unpacker_read_map_header(mrb, uk, &size);
  if (r < 0) {
    raise_unpacker_error(mrb, (int)r);
  }

  return mrb_fixnum_value(size);
}

#if 0
static mrb_sym
Unpacker_peek_next_type(mrb_state *mrb, mrb_value self)
{
  int r;

  UNPACKER(mrb, self, uk);
  r = msgpack_unpacker_peek_next_object_type(mrb, uk);
  if (r < 0) {
    raise_unpacker_error(mrb, r);
  }

  switch ((enum msgpack_unpacker_object_type) r) {
  case TYPE_NIL:
    return mrb_intern(mrb, "nil", 3);
  case TYPE_BOOLEAN:
    return mrb_intern(mrb, "boolean", 7);
  case TYPE_INTEGER:
    return mrb_intern(mrb, "integer", 7);
  case TYPE_FLOAT:
    return mrb_intern(mrb, "float", 5);
  case TYPE_RAW:
    return mrb_intern(mrb, "raw", 3);
  case TYPE_ARRAY:
    return mrb_intern(mrb, "array", 5);
  case TYPE_MAP:
    return mrb_intern(mrb, "map", 3);
  default:
    puts("logically unknown type dfsadf\n");
    //mrb_raisef(mrb, eUnpackError, "logically unknown type %d", r);
  }
}
#endif

static mrb_value
Unpacker_feed(mrb_state *mrb, mrb_value self)
{
  mrb_value str;

  UNPACKER(mrb, self, uk);
  mrb_get_args(mrb, "S", &str);
  msgpack_buffer_append_string(mrb, UNPACKER_BUFFER_(uk), str);

  return self;
}

static mrb_value
Unpacker_each_impl(mrb_state *mrb, mrb_value self)
{
  UNPACKER(mrb, self, uk);

  while (TRUE) {
    int r = msgpack_unpacker_read(mrb, uk, 0);

    if (r < 0) {
      if (r == PRIMITIVE_EOF) {
          return mrb_nil_value();
      }
      raise_unpacker_error(mrb, r);
    }
    //mrb_value v = msgpack_unpacker_get_last_object(uk);
    //mrb_yield(mrb, v);
  }
}

#if 0
static mrb_value Unpacker_rescue_EOFError(mrb_value self)
{
  UNUSED(self);
  return mrb_nil_value();
}
#endif

static mrb_value Unpacker_each(mrb_state *mrb, mrb_value self)
{
  UNPACKER(mrb, self, uk);

#ifdef RETURN_ENUMERATOR
  //RETURN_ENUMERATOR(self, 0, 0);
#endif

  if (msgpack_buffer_has_io(UNPACKER_BUFFER_(uk))) {
    return Unpacker_each_impl(mrb, self);
  } else {
    /* rescue EOFError only if io is set */
#if 0
    return rb_rescue2(Unpacker_each_impl, self,
                      Unpacker_rescue_EOFError, self,
                      rb_eEOFError, NULL);
#endif
    return mrb_nil_value();
  }
}

static mrb_value
Unpacker_feed_each(mrb_state *mrb, mrb_value self)
{
  // TODO optimize
  Unpacker_feed(mrb, self);
  return Unpacker_each(mrb, self);
}

static mrb_value
Unpacker_reset(mrb_state *mrb, mrb_value self)
{
  UNPACKER(mrb, self, uk);
  msgpack_unpacker_reset(mrb, uk);

  return mrb_nil_value();
}

mrb_value
MessagePack_unpack(mrb_state *mrb, mrb_value value)

{
  mrb_value src;
  mrb_value *argv;
  int argc; /* Number of arguments */
  int r;

  mrb_get_args(mrb, "*", &argv, &argc);
  switch (argc) {
  case 1:
    src = argv[0];
    break;
  default:
    mrb_raisef(mrb, E_ARGUMENT_ERROR, "wrong number of arguments (%d for 1)", argc);
  }

  mrb_value io = mrb_nil_value();
  if (mrb_type(src) != MRB_TT_STRING) {
    io = src;
    src = mrb_nil_value();
  }

  mrb_value self = Unpacker_alloc(mrb, cMessagePack_Unpacker);
  UNPACKER(mrb, self, uk);
  //msgpack_unpacker_reset(s_unpacker);
  //msgpack_buffer_reset_io(UNPACKER_BUFFER_(s_unpacker));

  /* Prefer reference than copying */
  msgpack_buffer_set_write_reference_threshold(UNPACKER_BUFFER_(uk), 0);

  if (!mrb_nil_p(io)) {
    MessagePack_Buffer_initialize(mrb, UNPACKER_BUFFER_(uk), io, mrb_nil_value());
  }

  if (!mrb_nil_p(src)) {
    /* Prefer reference than copying; see MessagePack_Unpacker_module_init */
    msgpack_buffer_append_string(mrb, UNPACKER_BUFFER_(uk), src);
  }

  r = msgpack_unpacker_read(mrb, uk, 0);
  if (r < 0) {
    raise_unpacker_error(mrb, r);
  }

  /* Raise if extra bytes follow */
  if (msgpack_buffer_top_readable_size(UNPACKER_BUFFER_(uk)) > 0) {
    //mrb_raise(mrb, eMalformedFormatError, "extra bytes follow after a deserialized object");
  }

#ifdef RB_GC_GUARD
  /* This prevents compilers from optimizing out the `self` variable
   * from stack. Otherwise GC free()s it. */
  RB_GC_GUARD(self);
#endif

  return msgpack_unpacker_get_last_object(uk);
}

static mrb_value
MessagePack_load_module_method(mrb_state *mrb, mrb_value self)
{
  return MessagePack_unpack(mrb, self);
}

static mrb_value
MessagePack_unpack_module_method(mrb_state *mrb, mrb_value self)
{
  return MessagePack_unpack(mrb, self);
}

void
MessagePack_Unpacker_module_init(mrb_state *mrb, struct RClass *mMessagePack)
{
  msgpack_unpacker_static_init(mrb);

  sym_unpacker_data = mrb_intern_lit(mrb, "unpacker_data");

  cMessagePack_Unpacker = mrb_define_class_under(mrb, mMessagePack, "Unpacker", mrb->object_class);

  eUnpackError = mrb_define_class_under(mrb, mMessagePack, "UnpackError", mrb->eStandardError_class);

  eMalformedFormatError = mrb_define_class_under(mrb, mMessagePack, "MalformedFormatError", eUnpackError);

  eStackError = mrb_define_class_under(mrb, mMessagePack, "StackError", eUnpackError);

  eTypeError = mrb_define_class_under(mrb, mMessagePack, "TypeError", mrb->eStandardError_class);

  mrb_define_method(mrb, cMessagePack_Unpacker, "initialize", Unpacker_initialize, MRB_ARGS_ANY());
  mrb_define_method(mrb, cMessagePack_Unpacker, "buffer", Unpacker_buffer, MRB_ARGS_NONE());
  mrb_define_method(mrb, cMessagePack_Unpacker, "read", Unpacker_read, MRB_ARGS_NONE());
  mrb_define_alias(mrb, cMessagePack_Unpacker, "unpack", "read");
  mrb_define_method(mrb, cMessagePack_Unpacker, "skip", Unpacker_skip, MRB_ARGS_NONE());
  mrb_define_method(mrb, cMessagePack_Unpacker, "skip_nil", Unpacker_skip_nil, MRB_ARGS_NONE());
  mrb_define_method(mrb, cMessagePack_Unpacker, "read_array_header", Unpacker_read_array_header, MRB_ARGS_NONE());
  mrb_define_method(mrb, cMessagePack_Unpacker, "read_map_header", Unpacker_read_map_header, MRB_ARGS_NONE());
  //rb_define_method(mrb, cMessagePack_Unpacker, "peek_next_type", Unpacker_peek_next_type, MRB_ARGS_NONE());  // TODO
  mrb_define_method(mrb, cMessagePack_Unpacker, "feed", Unpacker_feed, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, cMessagePack_Unpacker, "each", Unpacker_each, MRB_ARGS_NONE());
  mrb_define_method(mrb, cMessagePack_Unpacker, "feed_each", Unpacker_feed_each, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, cMessagePack_Unpacker, "reset", Unpacker_reset, MRB_ARGS_NONE());

  /* MessagePack.unpack(x) */
  mrb_define_module_function(mrb, mMessagePack, "load", MessagePack_load_module_method, MRB_ARGS_ANY());
  mrb_define_module_function(mrb, mMessagePack, "unpack", MessagePack_unpack_module_method, MRB_ARGS_ANY());
}