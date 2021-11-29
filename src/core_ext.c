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
#include "mruby/variable.h"
#include "core_ext.h"
#include "packer.h"
#include "packer_class.h"

static void
Packer_free(mrb_state *mrb, void* pk)
{
  if (pk == NULL) {
    return;
  }
  msgpack_packer_destroy(mrb, (void*)pk);
  mrb_free(mrb, pk);
}

static struct mrb_data_type mrb_data_type_msgpack_packer_t = {"msgpack_packer_t", Packer_free};


static inline mrb_value
delegete_to_pack(mrb_state *mrb, int argc, mrb_value* argv, mrb_value self)
{
  mrb_get_args(mrb, "*", &argv, &argc);
  if (argc == 0) {
    return MessagePack_pack_v(mrb, 1, &self);
  } else if (argc == 1) {
    /* Write to io */
    mrb_value argv2[2];
    argv2[0] = self;
    argv2[1] = argv[0];
    return MessagePack_pack_v(mrb, 2, argv2);
  } else {
    mrb_raisef(mrb, E_ARGUMENT_ERROR, "wrong number of arguments (%d for 0..1)", argc);
  }
  /* Pragram does not reach */
  return mrb_nil_value();
}


#define ENSURE_PACKER(mrb, argc, argv, packer, pk) \
  if (argc != 1 /*|| mrb_class_of(argv[0]) != cMessagePack_Packer*/){ \
    return delegete_to_pack(mrb, argc, argv, self); \
  } \
  mrb_value packer = argv[0]; \
  msgpack_packer_t *pk; \
  Data_Get_Struct(mrb, packer, &mrb_data_type_msgpack_packer_t, pk);

static mrb_value
NilClass_to_msgpack(mrb_state *mrb, mrb_value self)
{
  mrb_value *argv;
  int argc; /* Number of arguments */

  mrb_get_args(mrb, "*", &argv, &argc);

  ENSURE_PACKER(mrb, argc, argv, packer, pk);
  msgpack_packer_write_nil(mrb, pk);
  return packer;
}

static mrb_value
TrueClass_to_msgpack(mrb_state *mrb, mrb_value self)
{
  mrb_value *argv;
  int argc; /* Number of arguments */

  mrb_get_args(mrb, "*", &argv, &argc);
  ENSURE_PACKER(mrb, argc, argv, packer, pk);
  msgpack_packer_write_true(mrb, pk);
  return packer;
}

static mrb_value
FalseClass_to_msgpack(mrb_state *mrb, mrb_value self)
{
  mrb_value *argv;
  int argc; /* Number of arguments */

  mrb_get_args(mrb, "*", &argv, &argc);
  ENSURE_PACKER(mrb, argc, argv, packer, pk);
  msgpack_packer_write_false(mrb, pk);
  return packer;
}

static mrb_value
Fixnum_to_msgpack(mrb_state *mrb, mrb_value self)
{
  mrb_value *argv;
  int argc; /* Number of arguments */

  mrb_get_args(mrb, "*", &argv, &argc);

  ENSURE_PACKER(mrb, argc, argv, packer, pk);
  msgpack_packer_write_fixnum_value(mrb, pk, self);
  return packer;
}

#if 0
static mrb_value Bignum_to_msgpack(int argc, mrb_value* argv, mrb_value self)
{
  mrb_value *argv;
  int argc; // Number of arguments

  mrb_get_args(mrb, "*", &argv, &argc);

  ENSURE_PACKER(mrb, argc, argv, packer, pk);
  msgpack_packer_write_bignum_value(mrb, pk, self);
  return packer;
}
#endif

static mrb_value
Float_to_msgpack(mrb_state *mrb, mrb_value self)
{
  mrb_value *argv;
  int argc; /* Number of arguments */

  mrb_get_args(mrb, "*", &argv, &argc);
  ENSURE_PACKER(mrb, argc, argv, packer, pk);
  msgpack_packer_write_float_value(mrb, pk, self);
  return packer;
}

static mrb_value
String_to_msgpack(mrb_state *mrb, mrb_value self)
{
  mrb_value *argv;
  int argc; /* Number of arguments */

  mrb_get_args(mrb, "*", &argv, &argc);
  ENSURE_PACKER(mrb, argc, argv, packer, pk);
  msgpack_packer_write_string_value(mrb, pk, self);
  return packer;
}

static mrb_value
Array_to_msgpack(mrb_state *mrb, mrb_value self)
{
  mrb_value *argv;
  int argc; /* Number of arguments */

  mrb_get_args(mrb, "*", &argv, &argc);
  ENSURE_PACKER(mrb, argc, argv, packer, pk);
  msgpack_packer_write_array_value(mrb, pk, self);
  return packer;
}

static mrb_value
Hash_to_msgpack(mrb_state *mrb, mrb_value self)
{
  mrb_value *argv;
  int argc; /* Number of arguments */

  mrb_get_args(mrb, "*", &argv, &argc);
  ENSURE_PACKER(mrb, argc, argv, packer, pk);
  msgpack_packer_write_hash_value(mrb, pk, self);
  return packer;
}

static mrb_value
Symbol_to_msgpack(mrb_state *mrb, mrb_value self)
{
  mrb_value *argv;
  int argc; /* Number of arguments */

  mrb_get_args(mrb, "*", &argv, &argc);
  ENSURE_PACKER(mrb, argc, argv, packer, pk);
  msgpack_packer_write_symbol_value(mrb, pk, self);
  return packer;
}

void MessagePack_core_ext_module_init(mrb_state* mrb)
{
  mrb_define_method(mrb, mrb->nil_class,   "to_msgpack", NilClass_to_msgpack, MRB_ARGS_ANY());
  mrb_define_method(mrb, mrb->true_class,  "to_msgpack", TrueClass_to_msgpack, MRB_ARGS_ANY());
  mrb_define_method(mrb, mrb->false_class, "to_msgpack", FalseClass_to_msgpack, MRB_ARGS_ANY());
  mrb_define_method(mrb, mrb->integer_class, "to_msgpack", Fixnum_to_msgpack, MRB_ARGS_ANY());
  mrb_define_method(mrb, mrb->float_class,  "to_msgpack", Float_to_msgpack, MRB_ARGS_ANY());
  mrb_define_method(mrb, mrb->string_class, "to_msgpack", String_to_msgpack, MRB_ARGS_ANY());
  mrb_define_method(mrb, mrb->array_class,  "to_msgpack", Array_to_msgpack, MRB_ARGS_ANY());
  mrb_define_method(mrb, mrb->hash_class,   "to_msgpack", Hash_to_msgpack, MRB_ARGS_ANY());
  mrb_define_method(mrb, mrb->symbol_class, "to_msgpack", Symbol_to_msgpack, MRB_ARGS_ANY());
}
