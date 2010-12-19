#ifndef __CAMQP_H__
#define __CAMQP_H__

/// dependencies

// standard types
#include <stdint.h>
#include <stdbool.h>

// libxml helpers
#include <libxml/tree.h>
#include <libxml/xpath.h>

// standard uuid
#include <uuid/uuid.h>
// ---

/// enumerations
typedef enum {
	// null
	CAMQP_TYPE_NULL = 0,

	// scalars
	CAMQP_TYPE_BOOLEAN,
	//
	CAMQP_TYPE_UBYTE,
	CAMQP_TYPE_USHORT,
	CAMQP_TYPE_UINT,
	CAMQP_TYPE_ULONG,
	//
	CAMQP_TYPE_BYTE,
	CAMQP_TYPE_SHORT,
	CAMQP_TYPE_INT,
	CAMQP_TYPE_LONG,
	//
	CAMQP_TYPE_FLOAT, // 4B
	CAMQP_TYPE_DECIMAL32, //?
	CAMQP_TYPE_DOUBLE, // 8B
	CAMQP_TYPE_DECIMAL64, //?
	CAMQP_TYPE_TIMESTAMP,
	//
	CAMQP_TYPE_CHAR,   // one character
	CAMQP_TYPE_STRING, // string
	CAMQP_TYPE_SYMBOL, // ASCII string
	//
	CAMQP_TYPE_BINARY,
	//
	CAMQP_TYPE_UUID,

	// vectors
	CAMQP_TYPE_LIST,
	CAMQP_TYPE_MAP
} camqp_type;

typedef enum {
	CAMQP_CLASS_PRIMITIVE = 1,
	CAMQP_CLASS_COMPOSITE
} camqp_class;

typedef enum {
	CAMQP_MULTIPLICITY_SCALAR = 1,
	CAMQP_MULTIPLICITY_VECTOR
} camqp_multiplicity;
// ---

/// type aliases
typedef uint8_t		camqp_byte;
typedef uint32_t	camqp_size;
typedef uint8_t		camqp_char;
typedef uint32_t	camqp_code;
typedef uuid_t		camqp_uuid;
// ---

/// types

/// camqp_data
typedef struct {
	camqp_byte*		bytes;
	camqp_size		size; // max size of 4GB for 32b integer
} camqp_data;

camqp_data	camqp_data_static(const camqp_byte* data, camqp_size size);
camqp_data*	camqp_data_new(const camqp_byte* data, camqp_size size);
void		camqp_data_free(camqp_data* binary);
// ---

/// camqp_context
typedef struct {
	// protocol name
	camqp_char*	protocol;

	// definition location
	camqp_char*	definition;

	// parsed XML definition handle
	xmlDocPtr			xml;
	xmlXPathContextPtr	xpath;
} camqp_context;

/**
 *	creates context for CAMQP manipulation
 *
 *	@param string protocol
 *		protocol name
 *	@param string definition
 *		definition file name (XML)
 */
camqp_context*	camqp_context_new(const camqp_char* protocol, const camqp_char* definition);
void			camqp_context_free(camqp_context* context);

// ---

/// camqp_element
typedef struct {
	// context
	camqp_context*		context;

	// class of element
	camqp_class			class;
} camqp_element;

void camqp_element_free(camqp_element* element);

bool camqp_element_is_primitive(camqp_element* element);
bool camqp_element_is_null(camqp_element* element);
bool camqp_element_is_scalar(camqp_element* element);
bool camqp_element_is_vector(camqp_element* element);
bool camqp_element_is_composite(camqp_element* element);
// ---

/// camqp_primitive
typedef struct {
	// element base
	camqp_element		base;

	// primitive type indicator
	camqp_type			type;

	// multiplicity indicator
	camqp_multiplicity	multiple;
} camqp_primitive;

camqp_primitive* camqp_primitive_null(camqp_context* context);
// ---

/// camqp_scalar
typedef struct {
	// element base
	camqp_primitive		base;

	// data storage for scalar types
	union {
		bool			b;
		int64_t			i;
		uint64_t		ui;
		float			f;
		double			d;
		camqp_char*		str;
		camqp_data*		bin;
		camqp_uuid		uid;
	} data;
} camqp_scalar;

camqp_scalar*		camqp_scalar_bool(camqp_context* context,					bool value);					// BOOL
camqp_scalar*		camqp_scalar_int(camqp_context* context, camqp_type type,	int64_t value);					// BYTE, SHORT, INT, LONG, TIMESTAMP
camqp_scalar*		camqp_scalar_uint(camqp_context* context, camqp_type type,	uint64_t value);				// UBYTE, USHORT, UINT, ULONG
camqp_scalar*		camqp_scalar_float(camqp_context* context, camqp_type type,	float value);					// DECIMAL32, FLOAT
camqp_scalar*		camqp_scalar_double(camqp_context* context, camqp_type type,	double value);				// DECIMAL64, DOUBLE
camqp_scalar*		camqp_scalar_string(camqp_context* context, camqp_type type,	const camqp_char* value);	// STRING, SYMBOL, CHAR
camqp_scalar*		camqp_scalar_binary(camqp_context* context,					camqp_data* value);				// BINARY
camqp_scalar*		camqp_scalar_uuid(camqp_context* context);													// UUID

bool				camqp_value_bool(camqp_element* element);	// BOOL
int64_t				camqp_value_int(camqp_element* element);	// BYTE, SHORT, INT, LONG, TIMESTAMP
uint64_t			camqp_value_uint(camqp_element* element);	// UBYTE, USHORT, UINT, ULONG
float				camqp_value_float(camqp_element* element);	// DECIMAL32, FLOAT
double				camqp_value_double(camqp_element* element);	// DECIMAL64, DOUBLE
const camqp_char*	camqp_value_string(camqp_element* element);	// STRING, SYMBOL, CHAR
camqp_data*			camqp_value_binary(camqp_element* element);	// BINARY
const camqp_uuid*	camqp_value_uuid(camqp_element* element);	// UUID
// ---

/// camqp_vector

/**
 *	vector item
 */
struct camqp_vector_item_t {
	camqp_char*					key;
	camqp_element*				value;
	struct camqp_vector_item_t*	next;
};
typedef struct camqp_vector_item_t camqp_vector_item;

/**
 * structure for holding
 * lists and maps
 */
typedef struct {
	camqp_primitive		base;

	camqp_vector_item*	data;
} camqp_vector;

camqp_vector*	camqp_vector_new(camqp_context* ctx);

void			camqp_vector_item_put(camqp_vector* vector, const camqp_char* key, camqp_element* element);
camqp_element*	camqp_vector_item_get(camqp_vector* vector, const camqp_char* key);

// ---

/// camqp_composite
/**
 *
 */
typedef struct {
	camqp_element		base;

	camqp_char*			name;
	camqp_code			code;

	xmlNodePtr			type_def;

	camqp_vector_item*	fields;
} camqp_composite;

camqp_composite*	camqp_composite_new(camqp_context* context, const camqp_char* type_name, camqp_code type_code);

bool				camqp_composite_field_put(camqp_composite* element, const camqp_char* key, camqp_element* item);
camqp_element*		camqp_composite_field_get(camqp_composite* element, const camqp_char* key);
// ---

// ---

/// memory management
void*				camqp_util_new(camqp_size size);
void				camqp_util_free(void* data);
// ---

/// encoding, decoding & querying
camqp_data*		camqp_element_encode(camqp_element* element);

camqp_element*	camqp_element_decode(camqp_context* context, camqp_data* binary);
camqp_element*	camqp_query(camqp_context* context, const camqp_char* query, camqp_data* binary);
// ---

#endif
