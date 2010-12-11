#ifndef __CAMQP_H__
#define __CAMQP_H__

/// dependencies

// standard integer sizes
#include <stdint.h>
#include <stdbool.h>

#include <libxml/tree.h>
// ---

/// enumerations
typedef enum {
	CAMQP_TYPE_NULL = 0,

	CAMQP_TYPE_BOOLEAN,
	CAMQP_TYPE_UBYTE,
	CAMQP_TYPE_USHORT,
	CAMQP_TYPE_UINT,
	CAMQP_TYPE_ULONG,
	CAMQP_TYPE_BYTE,
	CAMQP_TYPE_SHORT,
	CAMQP_TYPE_INT,
	CAMQP_TYPE_LONG,
	CAMQP_TYPE_FLOAT, // 4B
	CAMQP_TYPE_DOUBLE, // 8B
	CAMQP_TYPE_DECIMAL32, //?
	CAMQP_TYPE_DECIMAL64, //?
	CAMQP_TYPE_CHAR,
	CAMQP_TYPE_TIMESTAMP,
	CAMQP_TYPE_UUID,
	CAMQP_TYPE_BINARY,
	CAMQP_TYPE_STRING, // UTF-8 string
	CAMQP_TYPE_SYMBOL // ASCII string
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
typedef uint8_t			camqp_byte;
typedef uint32_t		camqp_size;
typedef unsigned char	camqp_char;
// ---

/// types

/// camqp_data
typedef struct {
	camqp_byte*		bytes;
	camqp_size		size; // max size of 4GB for 32b integer
} camqp_data;

camqp_data*	camqp_data_new(const camqp_byte* data, camqp_size size);
void		camqp_data_free(camqp_data* binary);
// ---

/// camqp_string
typedef struct {
	camqp_char*		chars;
	camqp_size		length; // max size of 4GB for 32b integer
} camqp_string;

camqp_string*	camqp_string_new(const camqp_char* string, camqp_size length);
camqp_string*	camqp_string_duplicate(const camqp_string* original);
void			camqp_string_free(camqp_string* string);
// ---

/// camqp_context
typedef struct {
	// protocol name
	camqp_string*	protocol;

	// definition location
	camqp_string*	definition;

	// parsed XML definition handle
	xmlDocPtr		xml;
} camqp_context;

/**
 *	creates context for CAMQP manipulation
 *
 *	@param string protocol
 *		protocol name
 *	@param string definition
 *		definition file name (XML)
 */
camqp_context*	camqp_context_new(camqp_string* protocol, camqp_string* definition);
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

camqp_element*	camqp_element_create_composite(camqp_context* context, camqp_string* type_name);
// ---

/// camqp_vector

/**
 *	vector item
 */
typedef struct {
	camqp_string*	key;
	camqp_element*	data;
} camqp_element_vector_item;

/**
 * structure for holding
 * lists and maps
 */
typedef struct {
	camqp_element*				base;

	camqp_size					count;
	camqp_element_vector_item*	data;
} camqp_element_vector;
// ---

/// camqp_element_primitive
typedef struct {
	// element base
	camqp_element*		base;

	// primitive type indicator
	camqp_type			type;

	// data storage for primitives
	struct {
		int8_t			i8;
		uint8_t			ui8;
		float			f;
		double			d;
		camqp_string*	str;
		camqp_data		bin;
	} data;
} camqp_element_primitive;

camqp_element*	camqp_element_create_primitive_bool(camqp_context* context, bool value);
// ---

/// camqp_element_composite
/**
 *
 */
typedef struct {
	// composites
	camqp_string*			name;

	// data storage structure
	camqp_element_vector*	fields;
} camqp_element_composite;

camqp_element*	camqp_element_create_composite(camqp_context* context, camqp_string* type_name);
// ---

// ---

/// encoding, decoding & querying
camqp_data*		camqp_element_encode(camqp_element* element);
camqp_element*	camqp_element_decode(camqp_context* context, camqp_data* binary);
camqp_element*	camqp_query(camqp_context* context, camqp_data* binary);
// ---

#endif
