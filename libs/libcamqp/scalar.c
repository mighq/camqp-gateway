#include "camqp.h"
#include "internals.h"

#include <string.h> // memset, memcpy

/// new & free
camqp_scalar* camqp_scalar_new(camqp_context* context, camqp_type type) {
	camqp_scalar* ret = camqp_util_new(sizeof(camqp_scalar));
	if (!ret)
		return NULL;

	ret->base.base.context = context;
	ret->base.base.class = CAMQP_CLASS_PRIMITIVE;
	ret->base.multiple = CAMQP_MULTIPLICITY_SCALAR;
	ret->base.type = type;

	return ret;
}

void camqp_scalar_free(camqp_scalar* scalar) {
	// for dynamic elements free data memory
	if (
		scalar->base.type == CAMQP_TYPE_STRING
			||
		scalar->base.type == CAMQP_TYPE_CHAR
			||
		scalar->base.type == CAMQP_TYPE_SYMBOL
	)
		camqp_util_free(scalar->data.str);

	if (scalar->base.type == CAMQP_TYPE_BINARY)
		camqp_data_free(scalar->data.bin);

	// free element
	camqp_util_free(scalar);
}
// ---

/// bool
camqp_scalar* camqp_scalar_bool(camqp_context* context, bool value) {
	camqp_scalar* tp = camqp_scalar_new(context, CAMQP_TYPE_BOOLEAN);
	if (!tp)
		return NULL;

	tp->data.b = value;

	return tp;
}

bool camqp_value_bool(camqp_element* element) {
	if (!camqp_element_is_scalar(element))
		return false;

	camqp_scalar* scalar = (camqp_scalar*) element;

	if (scalar->base.type != CAMQP_TYPE_BOOLEAN)
		return false;

	return scalar->data.b;
}
// ---

/// int

// BYTE, SHORT, INT, LONG, TIMESTAMP
camqp_scalar* camqp_scalar_int(camqp_context* context, camqp_type type, int64_t value) {
	if (
		type != CAMQP_TYPE_BYTE
			&&
		type != CAMQP_TYPE_SHORT
			&&
		type != CAMQP_TYPE_INT
			&&
		type != CAMQP_TYPE_LONG
			&&
		type != CAMQP_TYPE_TIMESTAMP
	)
		return NULL;

	camqp_scalar* tp = camqp_scalar_new(context, type);
	if (!tp)
		return NULL;

	tp->data.i = value;

	return tp;
}

int64_t camqp_value_int(camqp_element* element) {
	if (!camqp_element_is_scalar(element))
		return 0;

	camqp_scalar* scalar = (camqp_scalar*) element;

	if (
		scalar->base.type != CAMQP_TYPE_BYTE
			&&
		scalar->base.type != CAMQP_TYPE_SHORT
			&&
		scalar->base.type != CAMQP_TYPE_INT
			&&
		scalar->base.type != CAMQP_TYPE_LONG
			&&
		scalar->base.type != CAMQP_TYPE_TIMESTAMP
	)
		return 0;

	return scalar->data.i;
}
// ---

/// uint

// UBYTE, USHORT, UINT, ULONG
camqp_scalar* camqp_scalar_uint(camqp_context* context, camqp_type type, uint64_t value) {
	if (
		type != CAMQP_TYPE_UBYTE
			&&
		type != CAMQP_TYPE_USHORT
			&&
		type != CAMQP_TYPE_UINT
			&&
		type != CAMQP_TYPE_ULONG
	)
		return NULL;

	camqp_scalar* tp = camqp_scalar_new(context, type);
	if (!tp)
		return NULL;

	tp->data.ui = value;

	return tp;
}

uint64_t camqp_value_uint(camqp_element* element) {
	if (!camqp_element_is_scalar(element))
		return 0;

	camqp_scalar* scalar = (camqp_scalar*) element;

	if (
		scalar->base.type != CAMQP_TYPE_UBYTE
			&&
		scalar->base.type != CAMQP_TYPE_USHORT
			&&
		scalar->base.type != CAMQP_TYPE_UINT
			&&
		scalar->base.type != CAMQP_TYPE_ULONG
	)
		return 0;

	return scalar->data.ui;
}
// ---

/// float

// DECIMAL32, FLOAT
camqp_scalar* camqp_scalar_float(camqp_context* context, camqp_type type, float value) {
	if (
		type != CAMQP_TYPE_DECIMAL32
			&&
		type != CAMQP_TYPE_FLOAT
	)
		return NULL;

	camqp_scalar* tp = camqp_scalar_new(context, type);
	if (!tp)
		return NULL;

	tp->data.f = value;

	return tp;
}

float camqp_value_float(camqp_element* element) {
	if (!camqp_element_is_scalar(element))
		return 0;

	camqp_scalar* scalar = (camqp_scalar*) element;

	if (
		scalar->base.type != CAMQP_TYPE_FLOAT
			&&
		scalar->base.type != CAMQP_TYPE_DECIMAL32
	)
		return 0;

	return scalar->data.f;
}
// ---

/// double

// DECIMAL64, DOUBLE
camqp_scalar* camqp_scalar_double(camqp_context* context, camqp_type type, double value) {
	if (
		type != CAMQP_TYPE_DECIMAL64
			&&
		type != CAMQP_TYPE_DOUBLE
	)
		return NULL;

	camqp_scalar* tp = camqp_scalar_new(context, type);
	if (!tp)
		return NULL;

	tp->data.d = value;

	return tp;
}

double camqp_value_double(camqp_element* element) {
	if (!camqp_element_is_scalar(element))
		return 0;

	camqp_scalar* scalar = (camqp_scalar*) element;

	if (
		scalar->base.type != CAMQP_TYPE_DOUBLE
			&&
		scalar->base.type != CAMQP_TYPE_DECIMAL64
	)
		return 0;

	return scalar->data.d;
}
// ---

/// string

// STRING, SYMBOL, CHAR, UUID
camqp_scalar* camqp_scalar_string(camqp_context* context, camqp_type type, const camqp_char* value) {
	if (
		type != CAMQP_TYPE_STRING
			&&
		type != CAMQP_TYPE_SYMBOL
			&&
		type != CAMQP_TYPE_CHAR
	)
		return NULL;

	if (type == CAMQP_TYPE_CHAR) {
		// check for max 4B (UTF32), only one character // neni uplne dobre
		if (strlen((char*) value) > 4)
			return NULL;
	}

	if (type == CAMQP_TYPE_SYMBOL) {
		// TODO check for only ascii charcters in symbol
	}

	if (type == CAMQP_TYPE_STRING) {
		// TODO check, ze to je platny utf-8
	}

	camqp_scalar* tp = camqp_scalar_new(context, type);
	if (!tp)
		return NULL;

	tp->data.str = xmlStrdup(value);
	if (!tp->data.str) {
		camqp_util_free(tp);
		return NULL;
	}

	return tp;
}

const camqp_char* camqp_value_string(camqp_element* element) {
	if (!camqp_element_is_scalar(element))
		return NULL;

	camqp_scalar* scalar = (camqp_scalar*) element;

	if (
		scalar->base.type != CAMQP_TYPE_STRING
			&&
		scalar->base.type != CAMQP_TYPE_SYMBOL
			&&
		scalar->base.type != CAMQP_TYPE_CHAR
	)
		return NULL;

	return scalar->data.str;
}
// ---

/// binary
camqp_scalar* camqp_scalar_binary(camqp_context* context, camqp_data* value) {
	camqp_scalar* tp = camqp_scalar_new(context, CAMQP_TYPE_BINARY);
	if (!tp)
		return NULL;

	tp->data.bin = camqp_data_new(value->bytes, value->size);

	return tp;
}

camqp_data* camqp_value_binary(camqp_element* element) {
	if (!camqp_element_is_scalar(element))
		return NULL;

	camqp_scalar* scalar = (camqp_scalar*) element;

	if (scalar->base.type != CAMQP_TYPE_BINARY)
		return NULL;

	return scalar->data.bin;
}
// ---

/// uuid
camqp_scalar* camqp_scalar_uuid(camqp_context* context) {
	camqp_scalar* tp = camqp_scalar_new(context, CAMQP_TYPE_UUID);
	if (!tp)
		return NULL;

	uuid_generate(tp->data.uid);

	return tp;
}

const camqp_uuid* camqp_value_uuid(camqp_element* element) {
	if (!camqp_element_is_scalar(element))
		return NULL;

	camqp_scalar* scalar = (camqp_scalar*) element;

	if (scalar->base.type != CAMQP_TYPE_UUID)
		return NULL;

	return (const camqp_uuid*) &scalar->data.uid;
}
// ---

