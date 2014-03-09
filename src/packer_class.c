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
#include "packer.h"
#include "packer_class.h"
#include "buffer_class.h"


struct RClass *cMessagePack_Packer;

static mrb_sym sym_pack_data;
//static mrb_sym s_to_msgpack;
static mrb_sym s_write;

//static mrb_value s_packer_value;
//static msgpack_packer_t* s_packer;

static void
Packer_free(mrb_state *mrb, void* pk)
{
  if (pk == NULL) {
    return;
  }
  msgpack_packer_destroy(mrb, (msgpack_packer_t*)pk);
  mrb_free(mrb, pk);
}

static struct mrb_data_type mrb_data_type_msgpack_packer_t = {"msgpack_packer_t", Packer_free};

#define PACKER(mrb, from, name) \
  msgpack_packer_t* name; \
  Data_Get_Struct(mrb, mrb_iv_get(mrb, from, sym_pack_data), &mrb_data_type_msgpack_packer_t, name); \
  if (name == NULL) { \
    mrb_raise(mrb, E_ARGUMENT_ERROR, "NULL found for " # name " when shouldn't be."); \
  }

static mrb_value Packer_alloc(mrb_state* mrb, struct RClass *klass)
{
  mrb_value klass_value;
  mrb_value data;

  msgpack_packer_t* pk = (msgpack_packer_t *)mrb_malloc(mrb, sizeof(msgpack_packer_t));
  msgpack_packer_init(mrb, pk);

  data = mrb_obj_value(Data_Wrap_Struct(mrb, klass, &mrb_data_type_msgpack_packer_t, (void *)pk));

  klass_value = mrb_obj_value(klass);
  mrb_iv_set(mrb, klass_value, sym_pack_data, data);
  pk->buffer_ref = MessagePack_Buffer_wrap(mrb, PACKER_BUFFER_(pk), data);

  return klass_value;
}

static mrb_value
Packer_initialize(mrb_state* mrb, mrb_value self)
{
  mrb_value io = mrb_nil_value();
  mrb_value options = mrb_nil_value();
  mrb_value *argv;
  int argc; /* Number of arguments */

  mrb_get_args(mrb, "*", &argv, &argc);
  if (argc == 0 || (argc == 1 && mrb_obj_eq(mrb, argv[0], mrb_nil_value()))) {
    /* mrb_nil_value() */
  } else if (argc == 1) {
    puts("hash");
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
      mrb_raisef(mrb, E_ARGUMENT_ERROR, "expected Hash but found %S.", mrb_str_new_cstr(mrb, "classname"));
    }
  } else {
    mrb_raisef(mrb, E_ARGUMENT_ERROR, "wrong number of arguments (%S for 0..1)", mrb_fixnum_value(argc));
  }

  self = Packer_alloc(mrb, mrb_class_ptr(self));

  PACKER(mrb, self, pk);
  if (!mrb_nil_p(io) || !mrb_nil_p(options)) {
    MessagePack_Buffer_initialize(mrb, PACKER_BUFFER_(pk), io, options);
  }

  // TODO options

  return self;
}

static mrb_value Packer_buffer(mrb_state* mrb, mrb_value self)
{
    PACKER(mrb, self, pk);
    //return pk->buffer_ref;
    return self;
}

static mrb_value Packer_write(mrb_state* mrb, mrb_value self)
{
  mrb_value obj;

  mrb_get_args(mrb, "o", &obj);
  PACKER(mrb, self, pk);
  msgpack_packer_write_value(mrb, pk, obj);
  return self;
}

static mrb_value Packer_write_nil(mrb_state* mrb, mrb_value self)
{
  PACKER(mrb, self, pk);
  msgpack_packer_write_nil(mrb, pk);
  return self;
}

static mrb_value Packer_write_array_header(mrb_state* mrb, mrb_value self)
{
  mrb_int n;

  mrb_get_args(mrb, "i", &n);
  PACKER(mrb, self, pk);
  msgpack_packer_write_array_header(mrb, pk, n);
  return self;
}

static mrb_value Packer_write_map_header(mrb_state* mrb, mrb_value self)
{
  mrb_int n;

  mrb_get_args(mrb, "i", &n);
  PACKER(mrb, self, pk);
  msgpack_packer_write_map_header(mrb, pk, n);
  return self;
}

static mrb_value Packer_flush(mrb_state* mrb, mrb_value self)
{
  PACKER(mrb, self, pk);
  msgpack_buffer_flush(mrb, PACKER_BUFFER_(pk));
  return self;
}

static mrb_value Packer_clear(mrb_state *mrb, mrb_value self)
{
    PACKER(mrb, self, pk);
    msgpack_buffer_clear(mrb, PACKER_BUFFER_(pk));
    return mrb_nil_value();
}

static mrb_value Packer_size(mrb_state *mrb, mrb_value self)
{
    PACKER(mrb, self, pk);
    size_t size = msgpack_buffer_all_readable_size(PACKER_BUFFER_(pk));
    //return SIZET2NUM(size);
    return mrb_fixnum_value(size);
}

static mrb_value Packer_empty_p(mrb_state *mrb, mrb_value self)
{
  PACKER(mrb, self, pk);
  if (msgpack_buffer_top_readable_size(PACKER_BUFFER_(pk)) == 0) {
    return mrb_true_value();
  } else {
    return mrb_false_value();
  }
}

static mrb_value
Packer_to_str(mrb_state *mrb, mrb_value self)
{
  PACKER(mrb, self, pk);
  return msgpack_buffer_all_as_string(mrb, PACKER_BUFFER_(pk));
}

static mrb_value
Packer_to_a(mrb_state *mrb, mrb_value self)
{
  PACKER(mrb, self, pk);
  return msgpack_buffer_all_as_string_array(mrb, PACKER_BUFFER_(pk));
}

#if 0
static mrb_value
Packer_write_to(mrb_state *mrb, mrb_value self)
{
  PACKER(mrb, self, pk);
  size_t sz = msgpack_buffer_flush_to_io(mrb, PACKER_BUFFER_(pk), io, s_write, TRUE);
  return mrb_fixnum_value(sz);
}
#endif

//static VALUE Packer_append(VALUE self, VALUE string_or_buffer)
//{
//    PACKER(self, pk);
//
//    // TODO if string_or_buffer is a Buffer
//    VALUE string = string_or_buffer;
//
//    msgpack_buffer_append_string(PACKER_BUFFER_(pk), string);
//
//    return self;
//}

//mrb_value MessagePack_pack(mrb_state *mrb, int argc, mrb_value* argv)
mrb_value
MessagePack_pack(mrb_state *mrb, mrb_value value) {
  mrb_value *argv;
  int argc; /* Number of arguments */

  mrb_get_args(mrb, "*", &argv, &argc);
  return MessagePack_pack_v(mrb, argc, argv);
}

mrb_value
MessagePack_pack_v(mrb_state *mrb, int argc, mrb_value* argv)
{
  //mrb_value *argv;
  //int argc; /* Number of arguments */
  /* TODO options */
  mrb_value v;
  mrb_value io = mrb_nil_value();

  //mrb_get_args(mrb, "*", &argv, &argc);
  switch (argc) {
  case 2:
    io = argv[1];
    /* Pass-through */
  case 1:
    v = argv[0];
    break;
  default:
    mrb_raisef(mrb, E_ARGUMENT_ERROR, "wrong number of arguments (%S for 1..2)", mrb_fixnum_value(argc));
  }

  mrb_value self = Packer_alloc(mrb, cMessagePack_Packer);

  PACKER(mrb, self, pk);
  //msgpack_packer_reset(s_packer);
  //msgpack_buffer_reset_io(PACKER_BUFFER_(s_packer));

  if (!mrb_nil_p(io)) {
    MessagePack_Buffer_initialize(mrb, PACKER_BUFFER_(pk), io, mrb_nil_value());
  }

  msgpack_packer_write_value(mrb, pk, v);

  mrb_value retval;
  if (!mrb_nil_p(io)) {
    msgpack_buffer_flush(mrb, PACKER_BUFFER_(pk));
    retval = mrb_nil_value();
  } else {
    retval = msgpack_buffer_all_as_string(mrb, PACKER_BUFFER_(pk));
  }

  //msgpack_buffer_clear(mrb, PACKER_BUFFER_(pk)); /* to free rmem before GC */

#ifdef RB_GC_GUARD
  /* This prevents compilers from optimizing out the `self` variable
   * from stack. Otherwise GC free()s it. */
  RB_GC_GUARD(self);
#endif

  return retval;
}

//static mrb_value MessagePack_dump_module_method(mrb_state *mrb, int argc, mrb_value* argv, mrb_value mod)
static mrb_value MessagePack_dump_module_method(mrb_state *mrb, mrb_value self)
{
  //UNUSED(mod);
  //return MessagePack_pack(mrb, argc, argv);
  return MessagePack_pack(mrb, self);
}

//static mrb_value MessagePack_pack_module_method(mrb_state *mrb, int argc, mrb_value* argv, mrb_value mod)
static mrb_value MessagePack_pack_module_method(mrb_state *mrb, mrb_value self)
{
  //UNUSED(mod);
  //return MessagePack_pack(mrb, argc, argv);
  return MessagePack_pack(mrb, self);
}

void MessagePack_Packer_module_init(mrb_state* mrb, struct RClass *mMessagePack)
{
  sym_pack_data = mrb_intern_lit(mrb, "pack_data");
  //s_to_msgpack = mrb_intern_cstr(mrb, "to_msgpack");
  s_write = mrb_intern_lit(mrb, "write");

  cMessagePack_Packer = mrb_define_class_under(mrb, mMessagePack, "Packer", mrb->object_class);

  mrb_define_method(mrb, cMessagePack_Packer, "initialize", Packer_initialize, MRB_ARGS_ANY()/* -1 */);
  mrb_define_method(mrb, cMessagePack_Packer, "buffer", Packer_buffer, MRB_ARGS_NONE()/*0*/);
  mrb_define_method(mrb, cMessagePack_Packer, "write", Packer_write, MRB_ARGS_REQ(1) /*1*/);
  mrb_define_alias(mrb, cMessagePack_Packer,  "pack", "write");
  mrb_define_method(mrb, cMessagePack_Packer, "write_nil", Packer_write_nil,  MRB_ARGS_NONE()/*0*/);
  mrb_define_method(mrb, cMessagePack_Packer, "write_array_header", Packer_write_array_header, MRB_ARGS_REQ(1) /*1*/);
  mrb_define_method(mrb, cMessagePack_Packer, "write_map_header", Packer_write_map_header, MRB_ARGS_REQ(1) /*1*/);
  mrb_define_method(mrb, cMessagePack_Packer, "flush", Packer_flush, MRB_ARGS_NONE()/*0*/);


  /* delegation methods */
  mrb_define_method(mrb, cMessagePack_Packer, "clear", Packer_clear, MRB_ARGS_NONE());
  mrb_define_method(mrb, cMessagePack_Packer, "size", Packer_size, MRB_ARGS_NONE());
  mrb_define_method(mrb, cMessagePack_Packer, "empty?", Packer_empty_p, MRB_ARGS_NONE());
  //mrb_define_method(mrb, cMessagePack_Packer, "write_to", Packer_write_to, MRB_ARGS_REQ(1) );
  mrb_define_method(mrb, cMessagePack_Packer, "to_str", Packer_to_str, MRB_ARGS_NONE());
  mrb_define_alias(mrb, cMessagePack_Packer, "to_s", "to_str");
  mrb_define_method(mrb, cMessagePack_Packer, "to_a", Packer_to_a, MRB_ARGS_NONE());

  /* MessagePack.pack(x) */
  mrb_define_module_function(mrb, mMessagePack, "pack", MessagePack_pack_module_method, MRB_ARGS_ANY()/* -1 */);
  mrb_define_module_function(mrb, mMessagePack, "dump", MessagePack_dump_module_method, MRB_ARGS_ANY()/* -1 */);
}