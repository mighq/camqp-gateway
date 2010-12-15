#ifndef __CAMQP_H__
#define __CAMQP_H__

/// dependencies

// standard integer sizes
#include <stdint.h>
#include <stdbool.h>

#include <libxml/tree.h>
#include <libxml/xpath.h>
// ---

/// enumerations
typedef enum {
	CAMQP_TYPE_NULL = 0,
	//
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
	CAMQP_TYPE_CHAR,
	CAMQP_TYPE_STRING, // UTF-8 string
	CAMQP_TYPE_SYMBOL, // ASCII string
	CAMQP_TYPE_UUID,
	//
	CAMQP_TYPE_BINARY
} camqp_type;

typedef enum {
	CAMQP_CLASS_PRIMITIVE = 0,
	CAMQP_CLASS_COMPOSITE
} camqp_class;

typedef enum {
	CAMQP_MULTIPLICITY_SCALAR = 0,
	CAMQP_MULTIPLICITY_VECTOR
} camqp_multiplicity;
// ---

/// type aliases
typedef uint8_t		camqp_byte;
typedef uint32_t	camqp_size;
typedef uint8_t		camqp_char;
typedef uint32_t	camqp_code;
// ---

void* camqp_util_new(camqp_size size);
void camqp_util_free(void* data);

/// types

/// camqp_data
typedef struct {
	camqp_byte*		bytes;
	camqp_size		size; // max size of 4GB for 32b integer
} camqp_data;

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

	// multiplicity indicator
	camqp_multiplicity	multiple;
} camqp_element;

void camqp_element_free(camqp_element* element);

bool camqp_element_is_primitive(camqp_element* element); //X
bool camqp_element_is_scalar(camqp_element* element); //X
// ---

/// camqp_primitive
typedef struct {
	// element base
	camqp_element		base;

	// primitive type indicator
	camqp_type			type;

	// data storage for primitives
	union {
		bool			b;
		int64_t			i;
		uint64_t		ui;
		float			f;
		double			d;
		camqp_char*		str;
		camqp_data*		bin;
	} data;
} camqp_primitive;

camqp_primitive* camqp_primitive_new(camqp_context* context); //X
void camqp_primitive_free(camqp_primitive* element); //X

camqp_primitive*	camqp_primitive_null(camqp_context* context);
bool				camqp_element_is_null(camqp_element* element);

camqp_primitive*	camqp_primitive_bool(camqp_context* context,					bool value);				// BOOL
camqp_primitive*	camqp_primitive_int(camqp_context* context, camqp_type type,	int64_t value);				// BYTE, SHORT, INT, LONG, TIMESTAMP
camqp_primitive*	camqp_primitive_uint(camqp_context* context, camqp_type type,	uint64_t value);			// UBYTE, USHORT, UINT, ULONG
camqp_primitive*	camqp_primitive_float(camqp_context* context, camqp_type type,	float value);				// DECIMAL32, FLOAT
camqp_primitive*	camqp_primitive_double(camqp_context* context, camqp_type type,	double value);				// DECIMAL64, DOUBLE
camqp_primitive*	camqp_primitive_string(camqp_context* context, camqp_type type,	const camqp_char* value);	// STRING, SYMBOL, CHAR, UUID
camqp_primitive*	camqp_primitive_binary(camqp_context* context,					camqp_data* value);			// BINARY

bool				camqp_value_bool(camqp_primitive* element);		// BOOL
int64_t				camqp_value_int(camqp_primitive* element);		// BYTE, SHORT, INT, LONG, TIMESTAMP
uint64_t			camqp_value_uint(camqp_primitive* element);		// UBYTE, USHORT, UINT, ULONG
float				camqp_value_float(camqp_primitive* element);	// DECIMAL32, FLOAT
double				camqp_value_double(camqp_primitive* element);	// DECIMAL64, DOUBLE
const camqp_char*	camqp_value_string(camqp_primitive* element);	// STRING, SYMBOL, CHAR, UUID
camqp_data*			camqp_value_binary(camqp_primitive* element);	// BINARY
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

camqp_vector_item* camqp_vector_item_new(const camqp_char* key, camqp_element* value);
void camqp_vector_item_free(camqp_vector_item* item, bool free_value);

/**
 * structure for holding
 * lists and maps
 */
typedef struct {
	camqp_element		base;

	camqp_vector_item*	data;
} camqp_vector;

camqp_vector*	camqp_vector_new(camqp_context* ctx);
void			camqp_vector_free(camqp_vector* vector, bool free_values);

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

	camqp_vector_item*	fields;
} camqp_composite;

camqp_composite*	camqp_composite_new(camqp_context* context, const camqp_char* type_name, camqp_code type_code);
void				camqp_composite_free(camqp_composite* element, bool free_values);

void				camqp_composite_field_put(camqp_composite* element, const camqp_char* key, camqp_element* item);
camqp_element*		camqp_composite_field_get(camqp_composite* element, const camqp_char* key);
// ---

// ---

char* dump_data(unsigned char* pointer, unsigned int length);
camqp_char* camqp_data_dump(camqp_data* data);

/// encoding, decoding & querying
camqp_data*		camqp_element_encode(camqp_element* element);
void			camqp_encode_primitive(camqp_primitive* element, camqp_data** buffer);
void			camqp_encode_primitive_null(camqp_data** buffer);
void			camqp_encode_primitive_bool(camqp_primitive* primitive, camqp_data** buffer);
void			camqp_encode_primitive_uint(camqp_primitive* element, camqp_data** buffer);

camqp_element*	camqp_element_decode(camqp_context* context, camqp_data* binary);
camqp_element*	camqp_query(camqp_context* context, const camqp_char* query, camqp_data* binary);
// ---

#endif
