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
#define ntohll(x) ( ( (uint64_t)(ntohl( (uint32_t)((x << 32) >> 32) )) << 32) | ntohl( ((uint32_t)(x >> 32)) ) )
#define htonll(x) ntohll(x)
uint64_t		twos_complement(int64_t nr);
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

bool				camqp_is_numeric(const camqp_char* string);

#endif
