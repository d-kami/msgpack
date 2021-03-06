/*
 * MessagePack for Ruby unpacking routine
 *
 * Copyright (C) 2008-2010 FURUHASHI Sadayuki
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
#include "ruby.h"
#include "msgpack/unpack_define.h"

static ID s_sysread;
static ID s_readpartial;

typedef struct {
	int finished;
	VALUE source;
	size_t offset;
	size_t parsed;
	VALUE buffer;
	VALUE stream;
	VALUE streambuf;
	ID stream_append_method;
} unpack_user;


#define msgpack_unpack_struct(name) \
	struct template ## name

#define msgpack_unpack_func(ret, name) \
	ret template ## name

#define msgpack_unpack_callback(name) \
	template_callback ## name

#define msgpack_unpack_object VALUE

#define msgpack_unpack_user unpack_user


struct template_context;
typedef struct template_context msgpack_unpack_t;

static void template_init(msgpack_unpack_t* u);

static VALUE template_data(msgpack_unpack_t* u);

static int template_execute(msgpack_unpack_t* u,
		const char* data, size_t len, size_t* off);


static inline VALUE template_callback_root(unpack_user* u)
{ return Qnil; }

static inline int template_callback_uint8(unpack_user* u, uint8_t d, VALUE* o)
{ *o = INT2FIX(d); return 0; }

static inline int template_callback_uint16(unpack_user* u, uint16_t d, VALUE* o)
{ *o = INT2FIX(d); return 0; }

static inline int template_callback_uint32(unpack_user* u, uint32_t d, VALUE* o)
{ *o = UINT2NUM(d); return 0; }

static inline int template_callback_uint64(unpack_user* u, uint64_t d, VALUE* o)
{ *o = rb_ull2inum(d); return 0; }

static inline int template_callback_int8(unpack_user* u, int8_t d, VALUE* o)
{ *o = INT2FIX((long)d); return 0; }

static inline int template_callback_int16(unpack_user* u, int16_t d, VALUE* o)
{ *o = INT2FIX((long)d); return 0; }

static inline int template_callback_int32(unpack_user* u, int32_t d, VALUE* o)
{ *o = INT2NUM((long)d); return 0; }

static inline int template_callback_int64(unpack_user* u, int64_t d, VALUE* o)
{ *o = rb_ll2inum(d); return 0; }

static inline int template_callback_float(unpack_user* u, float d, VALUE* o)
{ *o = rb_float_new(d); return 0; }

static inline int template_callback_double(unpack_user* u, double d, VALUE* o)
{ *o = rb_float_new(d); return 0; }

static inline int template_callback_nil(unpack_user* u, VALUE* o)
{ *o = Qnil; return 0; }

static inline int template_callback_true(unpack_user* u, VALUE* o)
{ *o = Qtrue; return 0; }

static inline int template_callback_false(unpack_user* u, VALUE* o)
{ *o = Qfalse; return 0;}

static inline int template_callback_array(unpack_user* u, unsigned int n, VALUE* o)
{ *o = rb_ary_new2(n); return 0; }

static inline int template_callback_array_item(unpack_user* u, VALUE* c, VALUE o)
{ rb_ary_push(*c, o); return 0; }  // FIXME set value directry RARRAY_PTR(obj)[RARRAY_LEN(obj)++]

static inline int template_callback_map(unpack_user* u, unsigned int n, VALUE* o)
{ *o = rb_hash_new(); return 0; }

static inline int template_callback_map_item(unpack_user* u, VALUE* c, VALUE k, VALUE v)
{ rb_hash_aset(*c, k, v); return 0; }

#ifdef RSTRING_EMBED_LEN_MAX
#define COW_MIN_SIZE RSTRING_EMBED_LEN_MAX
#else
#define COW_MIN_SIZE ((sizeof(VALUE)*3)/sizeof(char)-1)
#endif

static inline int template_callback_raw(unpack_user* u, const char* b, const char* p, unsigned int l, VALUE* o)
{ *o = (l <= COW_MIN_SIZE) ? rb_str_new(p, l) : rb_str_substr(u->source, p - b, l); return 0; }


#include "msgpack/unpack_template.h"


#define UNPACKER(from, name) \
	msgpack_unpack_t *name = NULL; \
	Data_Get_Struct(from, msgpack_unpack_t, name); \
	if(name == NULL) { \
		rb_raise(rb_eArgError, "NULL found for " # name " when shouldn't be."); \
	}

#define CHECK_STRING_TYPE(value) \
	value = rb_check_string_type(value); \
	if( NIL_P(value) ) { \
		rb_raise(rb_eTypeError, "instance of String needed"); \
	}

static VALUE cUnpacker;
static VALUE eUnpackError;

// FIXME slow operation
static void init_stack(msgpack_unpack_t* mp)
{
	size_t i;
	for(i=0; i < MSGPACK_MAX_STACK_SIZE; ++i) {
		mp->stack[i].map_key = Qnil;  /* GC */
	}
}

static void MessagePack_Unpacker_free(void* data)
{
	if(data) { free(data); }
}

static void MessagePack_Unpacker_mark(msgpack_unpack_t *mp)
{
	unsigned int i;
	rb_gc_mark(mp->user.buffer);
	rb_gc_mark(mp->user.stream);
	rb_gc_mark(mp->user.streambuf);
	for(i=0; i < mp->top; ++i) {
		rb_gc_mark(mp->stack[i].obj);
		rb_gc_mark(mp->stack[i].map_key); /* maybe map_key is not initialized */
	}
}

static VALUE MessagePack_Unpacker_alloc(VALUE klass)
{
	VALUE obj;
	msgpack_unpack_t* mp = ALLOC_N(msgpack_unpack_t, 1);
	obj = Data_Wrap_Struct(klass, MessagePack_Unpacker_mark,
			MessagePack_Unpacker_free, mp);
	return obj;
}

static VALUE MessagePack_Unpacker_reset(VALUE self)
{
	UNPACKER(self, mp);
	template_init(mp);
	init_stack(mp);
	mp->user.finished = 0;
	return self;
}

static ID append_method_of(VALUE stream)
{
	if(rb_respond_to(stream, s_sysread)) {
		return s_sysread;
	} else {
		return s_readpartial;
	}
}

static VALUE MessagePack_Unpacker_initialize(int argc, VALUE *argv, VALUE self)
{
	VALUE stream;
	switch(argc) {
	case 0:
		stream = Qnil;
		break;
	case 1:
		stream = argv[0];
		break;
	default:
		rb_raise(rb_eArgError, "wrong number of arguments (%d for 0)", argc);
	}

	MessagePack_Unpacker_reset(self);
	UNPACKER(self, mp);
	mp->user.offset = 0;
	mp->user.parsed = 0;
	mp->user.buffer = rb_str_new("",0);
	mp->user.stream = stream;
	mp->user.streambuf = rb_str_new("",0);
	mp->user.stream_append_method = append_method_of(stream);
	return self;
}


static VALUE MessagePack_Unpacker_execute_impl(VALUE args)
{
	VALUE self = ((VALUE*)args)[0];
	VALUE data = ((VALUE*)args)[1];

	UNPACKER(self, mp);
	size_t from = NUM2UINT(((VALUE*)args)[2]);
	char* dptr = RSTRING_PTR(data);
	long dlen = FIX2LONG(((VALUE*)args)[3]);
	int ret;

	if(from >= dlen) {
		rb_raise(eUnpackError, "offset is bigger than data buffer size.");
	}

	mp->user.source = data;
	ret = template_execute(mp, dptr, (size_t)dlen, &from);
	mp->user.source = Qnil;

	if(ret < 0) {
		rb_raise(eUnpackError, "parse error.");
	} else if(ret > 0) {
		mp->user.finished = 1;
		return ULONG2NUM(from);
	} else {
		mp->user.finished = 0;
		return ULONG2NUM(from);
	}
}

static VALUE MessagePack_Unpacker_execute_rescue(VALUE nouse)
{
	rb_gc_enable();
#ifdef RUBY_VM
	rb_exc_raise(rb_errinfo());
#else
	rb_exc_raise(ruby_errinfo);
#endif
}

static VALUE MessagePack_Unpacker_execute_limit(VALUE self, VALUE data,
		VALUE off, VALUE limit)
{
	// FIXME execute実行中はmp->topが更新されないのでGC markが機能しない
	rb_gc_disable();
	VALUE args[4] = {self, data, off, limit};
	VALUE ret = rb_rescue(MessagePack_Unpacker_execute_impl, (VALUE)args,
			MessagePack_Unpacker_execute_rescue, Qnil);
	rb_gc_enable();
	return ret;
}

static VALUE MessagePack_Unpacker_execute(VALUE self, VALUE data, VALUE off)
{
	return MessagePack_Unpacker_execute_limit(self, data, off,
			LONG2FIX(RSTRING_LEN(data)));
}

static VALUE MessagePack_Unpacker_finished_p(VALUE self)
{
	UNPACKER(self, mp);
	if(mp->user.finished) {
		return Qtrue;
	}
	return Qfalse;
}

static VALUE MessagePack_Unpacker_data(VALUE self)
{
	UNPACKER(self, mp);
	return template_data(mp);
}


static VALUE MessagePack_Unpacker_feed(VALUE self, VALUE data)
{
	UNPACKER(self, mp);
	StringValue(data);
	rb_str_cat(mp->user.buffer, RSTRING_PTR(data), RSTRING_LEN(data));
	return Qnil;
}

static VALUE MessagePack_Unpacker_stream_get(VALUE self)
{
	UNPACKER(self, mp);
	return mp->user.stream;
}

static VALUE MessagePack_Unpacker_stream_set(VALUE self, VALUE val)
{
	UNPACKER(self, mp);
	mp->user.stream = val;
	mp->user.stream_append_method = append_method_of(val);
	return val;
}

static VALUE MessagePack_Unpacker_fill(VALUE self)
{
	UNPACKER(self, mp);

	if(mp->user.stream == Qnil) {
		return Qnil;
	}

	size_t len;
	if(RSTRING_LEN(mp->user.buffer) == 0) {
		rb_funcall(mp->user.stream, mp->user.stream_append_method, 2,
				LONG2FIX(64*1024), mp->user.buffer);
		len = RSTRING_LEN(mp->user.buffer);
	} else {
		rb_funcall(mp->user.stream, mp->user.stream_append_method, 2,
				LONG2FIX(64*1024), mp->user.streambuf);
		len = RSTRING_LEN(mp->user.streambuf);
		rb_str_cat(mp->user.buffer, RSTRING_PTR(mp->user.streambuf), RSTRING_LEN(mp->user.streambuf));
	}

	return LONG2FIX(len);
}

static VALUE MessagePack_Unpacker_each(VALUE self)
{
	UNPACKER(self, mp);
	int ret;

#ifdef RETURN_ENUMERATOR
	RETURN_ENUMERATOR(self, 0, 0);
#endif

	while(1) {
		if(RSTRING_LEN(mp->user.buffer) <= mp->user.offset) {
			do_fill:
			{
				VALUE len = MessagePack_Unpacker_fill(self);
				if(len == Qnil || FIX2LONG(len) == 0) {
					break;
				}
			}
		}

		mp->user.source = mp->user.buffer;
		ret = template_execute(mp, RSTRING_PTR(mp->user.buffer), RSTRING_LEN(mp->user.buffer), &mp->user.offset);
		mp->user.source = Qnil;

		if(ret < 0) {
			rb_raise(eUnpackError, "parse error.");
		} else if(ret > 0) {
			VALUE data = template_data(mp);
			template_init(mp);
			init_stack(mp);
			rb_yield(data);
		} else {
			goto do_fill;
		}
	}

	return Qnil;
}


static VALUE MessagePack_unpack_impl(VALUE args)
{
	msgpack_unpack_t* mp = (msgpack_unpack_t*)((VALUE*)args)[0];
	VALUE data = ((VALUE*)args)[1];

	size_t from = 0;
	char* dptr = RSTRING_PTR(data);
	long dlen = FIX2LONG(((VALUE*)args)[2]);
	int ret;

	mp->user.source = data;
	ret = template_execute(mp, dptr, (size_t)dlen, &from);
	mp->user.source = Qnil;

	if(ret < 0) {
		rb_raise(eUnpackError, "parse error.");
	} else if(ret == 0) {
		rb_raise(eUnpackError, "insufficient bytes.");
	} else {
		if(from < dlen) {
			rb_raise(eUnpackError, "extra bytes.");
		}
		return template_data(mp);
	}
}

static VALUE MessagePack_unpack_rescue(VALUE args)
{
	rb_gc_enable();
#ifdef RUBY_VM
	rb_exc_raise(rb_errinfo());
#else
	rb_exc_raise(ruby_errinfo);
#endif
}

static VALUE MessagePack_unpack_limit(VALUE self, VALUE data, VALUE limit)
{
	CHECK_STRING_TYPE(data);

	msgpack_unpack_t mp;
	template_init(&mp);
	init_stack(&mp);
	unpack_user u = {0, Qnil, 0, 0, Qnil, Qnil, Qnil};
	mp.user = u;

	rb_gc_disable();
	VALUE args[3] = {(VALUE)&mp, data, limit};
	VALUE ret = rb_rescue(MessagePack_unpack_impl, (VALUE)args,
			MessagePack_unpack_rescue, Qnil);
	rb_gc_enable();

	return ret;
}

static VALUE MessagePack_unpack(VALUE self, VALUE data)
{
	return MessagePack_unpack_limit(self, data,
			LONG2FIX(RSTRING_LEN(data)));
}


void Init_msgpack_unpack(VALUE mMessagePack)
{
	s_sysread = rb_intern("sysread");
	s_readpartial = rb_intern("readpartial");
	eUnpackError = rb_define_class_under(mMessagePack, "UnpackError", rb_eStandardError);
	cUnpacker = rb_define_class_under(mMessagePack, "Unpacker", rb_cObject);
	rb_define_alloc_func(cUnpacker, MessagePack_Unpacker_alloc);
	rb_define_method(cUnpacker, "initialize", MessagePack_Unpacker_initialize, -1);
	rb_define_method(cUnpacker, "execute", MessagePack_Unpacker_execute, 2);
	rb_define_method(cUnpacker, "execute_limit", MessagePack_Unpacker_execute_limit, 3);
	rb_define_method(cUnpacker, "finished?", MessagePack_Unpacker_finished_p, 0);
	rb_define_method(cUnpacker, "data", MessagePack_Unpacker_data, 0);
	rb_define_method(cUnpacker, "reset", MessagePack_Unpacker_reset, 0);
	rb_define_method(cUnpacker, "feed", MessagePack_Unpacker_feed, 1);
	rb_define_method(cUnpacker, "fill", MessagePack_Unpacker_fill, 0);
	rb_define_method(cUnpacker, "each", MessagePack_Unpacker_each, 0);
	rb_define_method(cUnpacker, "stream", MessagePack_Unpacker_stream_get, 0);
	rb_define_method(cUnpacker, "stream=", MessagePack_Unpacker_stream_set, 1);
	rb_define_module_function(mMessagePack, "unpack", MessagePack_unpack, 1);
	rb_define_module_function(mMessagePack, "unpack_limit", MessagePack_unpack_limit, 2);
}

