#ifndef __CAMQP_H__
#define __CAMQP_H__

// standard integer sizes
#include <stdint.h>
#include <stdbool.h>

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
	CAMQP_TYPE_FLOAT,
	CAMQP_TYPE_DOUBLE,
	CAMQP_TYPE_DECIMAL32, //?
	CAMQP_TYPE_DECIMAL64, //?
	CAMQP_TYPE_CHAR,
	CAMQP_TYPE_TIMESTAMP,
	CAMQP_TYPE_UUID,
	CAMQP_TYPE_BINARY,
	CAMQP_TYPE_STRING,
	CAMQP_TYPE_SYMBOL, //?

	CAMQP_TYPE_LIST,
	CAMQP_TYPE_MAP
} camqp_type;

typedef enum {
	CAMQP_CLASS_PRIMITIVE = 0,
	CAMQP_CLASS_COMPOSITE
} camqp_class;

typedef uint8_t		camqp_byte;
typedef uint32_t	camqp_size;

typedef struct {
	camqp_byte*		data;
	camqp_size		length; // max size of 4GB for 32b integer
} camqp_data;

camqp_data*	camqp_data_new(camqp_byte* data, camqp_size size, bool duplicate);
void		camqp_data_free(camqp_data* binary);

typedef struct {

	
} camqp_context;

typedef struct {
	// context
	camqp_context*	context;

	camqp_type	type;
	camqp_class	class;

	// primitives
	struct {
		int8_t		vi8;
		uint8_t		vui8;

		const char*	vstr; // todo

		camqp_data	vbin;
	} primitive;

	// composites
	struct {
		const char* name; // todo
		int fields;
	} composite;
} camqp_element;

// todo typ pre const char *

/**
 *	creates context for CAMQP manipulation
 *
 *	@param string protocol
 *		protocol name
 *	@param string definition
 *		definition file name (XML)
 */
camqp_context*	camqp_context_create(const char* protocol, const char* definition);
void			camqp_context_free(camqp_context* context);



camqp_element*	camqp_element_create_primitive_bool(camqp_context* context, bool value);
camqp_element*	camqp_element_create_composite(camqp_context* context, const char* type_name);

void			camqp_element_free(camqp_element* element);

camqp_data*		camqp_element_encode(camqp_element* element);



camqp_element*	camqp_element_decode(camqp_context* context, camqp_data* binary);
camqp_element*	camqp_query(camqp_context* context, camqp_data* binary);

int xyz();

#endif
