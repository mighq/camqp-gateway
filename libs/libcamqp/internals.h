#ifndef __INTERNALS_H__
#define __INTERNALS_H__

#include "camqp.h"

#include <netinet/in.h>

/// debugging
char*				dump_data(unsigned char* pointer, unsigned int length);
camqp_char*			camqp_data_dump(camqp_data* data);
// ---

/// conversions

// TODO: what about other platforms?

// 64bit byte swapping
#define		ntohll(x) ( ( (uint64_t)(ntohl( (uint32_t)((x << 32) >> 32) )) << 32) | ntohl( ((uint32_t)(x >> 32)) ) )
#define		htonll(x) ntohll(x)

// two's complement
uint64_t	totc(int64_t nr);
int8_t		fromtc8(uint8_t nr);
int16_t		fromtc16(uint16_t nr);
int32_t		fromtc32(uint32_t nr);
int64_t		fromtc64(uint64_t nr);
// ---

/// new & free
camqp_scalar*		camqp_scalar_new(camqp_context* context, camqp_type type);

void				camqp_primitive_free(camqp_primitive* element);
void				camqp_scalar_free(camqp_scalar* vector);
void				camqp_vector_free(camqp_vector* vector, bool free_values);

void				camqp_composite_free(camqp_composite* element, bool free_values);

camqp_vector_item*	camqp_vector_item_new(const camqp_char* key, camqp_element* value);
void				camqp_vector_item_free(camqp_vector_item* item, bool free_value);
// ---

/// types
camqp_char*			camqp_element_type_name(camqp_element* element);
// ---

/// encoding
void			camqp_encode_primitive(camqp_primitive* element, camqp_data** buffer);

void			camqp_encode_null(camqp_data** buffer);

void			camqp_encode_scalar(camqp_scalar* element, camqp_data** buffer);
void			camqp_encode_scalar_bool(camqp_scalar* element, camqp_data** buffer);
void			camqp_encode_scalar_uint(camqp_scalar* element, camqp_data** buffer);
void			camqp_encode_scalar_int(camqp_scalar* element, camqp_data** buffer);
void			camqp_encode_scalar_float(camqp_scalar* element, camqp_data** buffer);
void			camqp_encode_scalar_double(camqp_scalar* element, camqp_data** buffer);
void			camqp_encode_scalar_string(camqp_scalar* element, camqp_data** buffer);
void			camqp_encode_scalar_uuid(camqp_scalar* element, camqp_data** buffer);
void			camqp_encode_scalar_binary(camqp_scalar* element, camqp_data** buffer);

void			camqp_encode_vector(camqp_vector* set, camqp_data** buffer);
void			camqp_encode_list(camqp_vector* set, camqp_data** buffer);
void			camqp_encode_map(camqp_vector* set, camqp_data** buffer);

void			camqp_encode_composite(camqp_composite* element, camqp_data** buffer);
// ---

/// decoding
camqp_element* camqp_decode_primitive(camqp_context* context, camqp_data* binary, camqp_data* left);

camqp_element* camqp_decode_scalar(camqp_context* context, camqp_data* binary, camqp_data* left);

camqp_element* camqp_decode_scalar_bool(camqp_context* context, camqp_data* binary, camqp_data* left, camqp_byte type_code);
camqp_element* camqp_decode_scalar_uint(camqp_context* context, camqp_data* binary, camqp_data* left, camqp_byte type_code);
camqp_element* camqp_decode_scalar_int(camqp_context* context, camqp_data* binary, camqp_data* left, camqp_byte type_code);
camqp_element* camqp_decode_scalar_float(camqp_context* context, camqp_data* binary, camqp_data* left, camqp_byte type_code);
camqp_element* camqp_decode_scalar_double(camqp_context* context, camqp_data* binary, camqp_data* left, camqp_byte type_code);
camqp_element* camqp_decode_scalar_uuid(camqp_context* context, camqp_data* binary, camqp_data* left);
camqp_element* camqp_decode_scalar_binary(camqp_context* context, camqp_data* binary, camqp_data* left, camqp_byte type_code);
camqp_element* camqp_decode_scalar_string(camqp_context* context, camqp_data* binary, camqp_data* left, camqp_byte type_code);

camqp_element* camqp_decode_vector(camqp_context* context, camqp_data* binary, camqp_data* left);

camqp_element* camqp_decode_composite(camqp_context* context, camqp_data* binary, camqp_data* left);
// ---

bool				camqp_is_numeric(const camqp_char* string);

#endif
