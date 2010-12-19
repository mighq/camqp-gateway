#include "camqp.h"
#include "internals.h"

#include <string.h> // memset, memcpy

/// element
camqp_data* camqp_element_encode(camqp_element* element) {
	camqp_data* encoded = NULL;

	if (element->class == CAMQP_CLASS_PRIMITIVE)
		camqp_encode_primitive((camqp_primitive*) element, &encoded);
	else if (element->class == CAMQP_CLASS_COMPOSITE)
		camqp_encode_composite((camqp_composite*) element, &encoded);
	else
		camqp_encode_null(&encoded);

	return encoded;
}
// ---

/// primitive
void camqp_encode_primitive(camqp_primitive* element, camqp_data** buffer) {
	if (camqp_element_is_null((camqp_element*) element)) {
		camqp_encode_null(buffer);
		return;
	}

	if (element->multiple == CAMQP_MULTIPLICITY_SCALAR)
		camqp_encode_scalar((camqp_scalar*) element, buffer);
	else if (element->multiple == CAMQP_MULTIPLICITY_VECTOR)
		camqp_encode_vector((camqp_vector*) element, buffer);
	else
		camqp_encode_null(buffer);
}

void camqp_encode_null(camqp_data** buffer) {
	// allocate working data
	camqp_byte* wk = camqp_util_new(1*sizeof(camqp_byte));
	if (!wk)
		return;

	memcpy(wk, "\x40", 1);

	// copy working data to camqp_data structure
	camqp_data* data = camqp_data_new(wk, 1);
	// free working data
	camqp_util_free(wk);
	// set the result
	*buffer = data;
}
// ---

/// scalar
void camqp_encode_scalar(camqp_scalar* element, camqp_data** buffer) {
	switch (element->base.type) {
		case CAMQP_TYPE_NULL:
			camqp_encode_null(buffer);
			break;
		case CAMQP_TYPE_BOOLEAN:
			camqp_encode_scalar_bool(element, buffer);
			break;
		case CAMQP_TYPE_UBYTE:
		case CAMQP_TYPE_USHORT:
		case CAMQP_TYPE_UINT:
		case CAMQP_TYPE_ULONG:
			camqp_encode_scalar_uint(element, buffer);
			break;
		case CAMQP_TYPE_BYTE:
		case CAMQP_TYPE_SHORT:
		case CAMQP_TYPE_INT:
		case CAMQP_TYPE_LONG:
		case CAMQP_TYPE_TIMESTAMP:
			camqp_encode_scalar_int(element, buffer);
			break;
		case CAMQP_TYPE_FLOAT:
		case CAMQP_TYPE_DECIMAL32:
			camqp_encode_scalar_float(element, buffer);
			break;
		case CAMQP_TYPE_DOUBLE:
		case CAMQP_TYPE_DECIMAL64:
			camqp_encode_scalar_double(element, buffer);
			break;
		case CAMQP_TYPE_UUID:
			camqp_encode_scalar_uuid(element, buffer);
			break;
		case CAMQP_TYPE_CHAR:
		case CAMQP_TYPE_STRING:
		case CAMQP_TYPE_SYMBOL:
			camqp_encode_scalar_string(element, buffer);
			break;
		case CAMQP_TYPE_BINARY:
			camqp_encode_scalar_binary(element, buffer);
			break;
		default:
			camqp_encode_null(buffer);
	}
}

/// bool
void camqp_encode_scalar_bool(camqp_scalar* element, camqp_data** buffer) {
	if (element->base.type != CAMQP_TYPE_BOOLEAN)
		return;

	// allocate working data
	camqp_byte* wk = camqp_util_new(1*sizeof(camqp_byte));
	if (!wk)
		return;

	if (element->data.b == true)
		memcpy(wk, "\x41", 1);
	else
		memcpy(wk, "\x42", 1);

	// copy working data to camqp_data structure
	camqp_data* data = camqp_data_new(wk, 1);
	// free working data
	camqp_util_free(wk);
	// set the result
	*buffer = data;
}
// ---

/// uint
void camqp_encode_scalar_uint(camqp_scalar* element, camqp_data** buffer) {
	if (
		element->base.type != CAMQP_TYPE_UBYTE
			&&
		element->base.type != CAMQP_TYPE_USHORT
			&&
		element->base.type != CAMQP_TYPE_UINT
			&&
		element->base.type != CAMQP_TYPE_ULONG
	)
		return;

	// allocate working data
	camqp_byte* wk = camqp_util_new(9*sizeof(camqp_byte));
	if (!wk)
		return;

	uint8_t len = 0;
	if (element->base.type == CAMQP_TYPE_UBYTE) {
		len = 2;
		memcpy(wk, "\x50", 1);
		memcpy(wk+1, (void*) &element->data.ui, 1);
	} else if (element->base.type == CAMQP_TYPE_USHORT) {
		len = 3;
		uint16_t conv = (uint16_t) htons((uint16_t) element->data.ui);
		memcpy(wk, "\x60", 1);
		memcpy(wk+1, (void*) &conv, 2);
	} else if (element->base.type == CAMQP_TYPE_UINT) {
		len = 5;
		uint32_t conv = (uint32_t) htonl((uint32_t) element->data.ui);
		memcpy(wk, "\x70", 1);
		memcpy(wk+1, (void*) &conv, 4);
	} else if (element->base.type == CAMQP_TYPE_ULONG) {
		len = 9;
		uint64_t conv = (uint64_t) htonll((uint64_t) element->data.ui);
		memcpy(wk, "\x80", 1);
		memcpy(wk+1, (void*) &conv, 8);
	}

	// copy working data to camqp_data structure
	camqp_data* data = camqp_data_new(wk, len);
	// free working data
	camqp_util_free(wk);
	// set the result
	*buffer = data;
}
// ---

/// int
void camqp_encode_scalar_int(camqp_scalar* element, camqp_data** buffer) {
	if (
		element->base.type != CAMQP_TYPE_BYTE
			&&
		element->base.type != CAMQP_TYPE_SHORT
			&&
		element->base.type != CAMQP_TYPE_INT
			&&
		element->base.type != CAMQP_TYPE_LONG
			&&
		element->base.type != CAMQP_TYPE_TIMESTAMP
	)
		return;

	// allocate working data
	camqp_byte* wk = camqp_util_new(9*sizeof(camqp_byte));
	if (!wk)
		return;

	uint8_t len = 0;
	if (element->base.type == CAMQP_TYPE_BYTE) {
		len = 2;
		uint8_t conv = (uint8_t) twos_complement(element->data.i);
		memcpy(wk, "\x51", 1);
		memcpy(wk+1, &conv, 1);
	} else if (element->base.type == CAMQP_TYPE_SHORT) {
		len = 3;
		uint16_t conv1 = (uint16_t) twos_complement(element->data.i);
		uint16_t conv = htons(conv1);
		memcpy(wk, "\x61", 1);
		memcpy(wk+1, (void*) &conv, 2);
	} else if (element->base.type == CAMQP_TYPE_INT) {
		len = 5;
		uint32_t conv1 = (uint32_t) twos_complement(element->data.i);
		uint32_t conv = htonl(conv1);
		memcpy(wk, "\x71", 1);
		memcpy(wk+1, (void*) &conv, 4);
	} else if (element->base.type == CAMQP_TYPE_LONG) {
		len = 9;
		uint64_t conv1 = twos_complement(element->data.i);
		uint64_t conv = htonll(conv1);
		memcpy(wk, "\x81", 1);
		memcpy(wk+1, (void*) &conv, 8);
	} else if (element->base.type == CAMQP_TYPE_TIMESTAMP) {
		len = 9;
		uint64_t conv1 = twos_complement(element->data.i);
		uint64_t conv = htonll(conv1);
		memcpy(wk, "\x83", 1);
		memcpy(wk+1, (void*) &conv, 8);
	}

	// copy working data to camqp_data structure
	camqp_data* data = camqp_data_new(wk, len);
	// free working data
	camqp_util_free(wk);
	// set the result
	*buffer = data;
}
// ---

/// float
void camqp_encode_scalar_float(camqp_scalar* element, camqp_data** buffer) {
	if (
		element->base.type != CAMQP_TYPE_FLOAT
			&&
		element->base.type != CAMQP_TYPE_DECIMAL32
	)
		return;

	// allocate working data
	camqp_byte* wk = camqp_util_new(5*sizeof(camqp_byte));
	if (!wk)
		return;

	uint32_t* conv1 = (uint32_t*) &element->data.f;
	uint32_t  conv2 = htonl(*conv1);

	if (element->base.type == CAMQP_TYPE_FLOAT)
		memcpy(wk, "\x72", 1);
	else
		memcpy(wk, "\x74", 1);

	memcpy(wk+1, (void*) &conv2, 4);

	// copy working data to camqp_data structure
	camqp_data* data = camqp_data_new(wk, 5);
	// free working data
	camqp_util_free(wk);
	// set the result
	*buffer = data;
}
// ---

/// double
void camqp_encode_scalar_double(camqp_scalar* element, camqp_data** buffer) {
	if (
		element->base.type != CAMQP_TYPE_DOUBLE
			&&
		element->base.type != CAMQP_TYPE_DECIMAL64
	)
		return;

	// allocate working data
	camqp_byte* wk = camqp_util_new(9*sizeof(camqp_byte));
	if (!wk)
		return;

	uint64_t* conv1 = (uint64_t*) &element->data.d;
	uint64_t  conv2 = htonll(*conv1);

	if (element->base.type == CAMQP_TYPE_DOUBLE)
		memcpy(wk, "\x82", 1);
	else
		memcpy(wk, "\x84", 1);

	memcpy(wk+1, (void*) &conv2, 8);

	// copy working data to camqp_data structure
	camqp_data* data = camqp_data_new(wk, 9);
	// free working data
	camqp_util_free(wk);
	// set the result
	*buffer = data;
}
// ---

/// string
void camqp_encode_scalar_string(camqp_scalar* element, camqp_data** buffer) {
	if (
		element->base.type != CAMQP_TYPE_CHAR
			&&
		element->base.type != CAMQP_TYPE_STRING
			&&
		element->base.type != CAMQP_TYPE_SYMBOL
	)
		return;

	// fixed size
	if (element->base.type == CAMQP_TYPE_CHAR) {
		// allocate working data
		camqp_byte* wk = camqp_util_new(5*sizeof(camqp_byte));
		if (!wk)
			return;

		// TODO: niesom isty tym UTF-32 encodingom jak to ma byt
		memcpy(wk, "\x73", 1);
		memcpy(wk+1, element->data.str, strlen((char*)element->data.str));

		// copy working data to camqp_data structure
		camqp_data* data = camqp_data_new(wk, 5);
		// free working data
		camqp_util_free(wk);
		// set the result
		*buffer = data;

		return;
	}

	// variable size


	// determine length
	uint32_t len = strlen((char*) element->data.str);

	// choose type according to length
	uint8_t code;
	uint32_t size;
	if (len <= 0xFF) {
		if (element->base.type == CAMQP_TYPE_STRING)
			code = 0xA1; // str8-utf8
		else
			code = 0xA3; // sym8

		size = 1+1+len;
	} else {
		if (element->base.type == CAMQP_TYPE_STRING)
			code = 0xB1; // str32-utf8
		else
			code = 0xB3; // sym32

		size = 1+4+len;
	}

	// allocate working data
	camqp_byte* wk = camqp_util_new(size*sizeof(camqp_byte));
	if (!wk)
		return;

	memcpy(wk, &code, 1);
	if (len <= 0xFF) {
		memcpy(wk+1, &len, 1);
		memcpy(wk+2, element->data.str, len);
	} else {
		uint32_t len_conv = htonl(len);
		memcpy(wk+1, &len_conv, 4);
		memcpy(wk+5, element->data.str, len);
	}

	// copy working data to camqp_data structure
	camqp_data* data = camqp_data_new(wk, size);
	// free working data
	camqp_util_free(wk);
	// set the result
	*buffer = data;

	return;
}
// ---

/// binary
void camqp_encode_scalar_binary(camqp_scalar* element, camqp_data** buffer) {
	if (element->base.type != CAMQP_TYPE_BINARY)
		return;

	// determine length
	uint32_t len = element->data.bin->size;

	// choose type according to length
	uint8_t code;
	uint32_t size;
	if (len <= 0xFF) {
		code = 0xA0; // vbin8
		size = 1+1+len;
	} else {
		code = 0xB0; // vbin32
		size = 1+4+len;
	}

	// allocate working data
	camqp_byte* wk = camqp_util_new(size*sizeof(camqp_byte));
	if (!wk)
		return;

	memcpy(wk, &code, 1);
	if (len <= 0xFF) {
		memcpy(wk+1, &len, 1);
		memcpy(wk+2, element->data.bin->bytes, len);
	} else {
		uint32_t len_conv = htonl(len);
		memcpy(wk+1, &len_conv, 4);
		memcpy(wk+5, element->data.bin->bytes, len);
	}
	// copy working data to camqp_data structure
	camqp_data* data = camqp_data_new(wk, size);
	// free working data
	camqp_util_free(wk);
	// set the result
	*buffer = data;

	return;
}
// ---

/// uuid
void camqp_encode_scalar_uuid(camqp_scalar* element, camqp_data** buffer) {
	if (element->base.type != CAMQP_TYPE_UUID)
		return;

	// allocate working data
	camqp_byte* wk = camqp_util_new(17*sizeof(camqp_byte));
	if (!wk)
		return;

	memcpy(wk, "\x98", 1);
	memcpy(wk+1, (void*) &element->data.uid, 16);

	// copy working data to camqp_data structure
	camqp_data* data = camqp_data_new(wk, 17);
	// free working data
	camqp_util_free(wk);
	// set the result
	*buffer = data;
}
// ---

// ---

/// vector
void camqp_encode_vector(camqp_vector* set, camqp_data** buffer) {
	// for empty array return NULL element
	if (set->data == NULL) {
		camqp_encode_null(buffer);
		return;
	}

	if (set->base.type == CAMQP_TYPE_LIST)
		camqp_encode_list(set, buffer);
	else if (set->base.type == CAMQP_TYPE_MAP)
		camqp_encode_map(set, buffer);
	else
		camqp_encode_null(buffer);
}

/// list
void camqp_encode_list(camqp_vector* set, camqp_data** buffer) {
	uint32_t count = 0;

	camqp_vector_item* i = set->data;
	while (i != NULL) {
		count++;
		i = i->next;
	}

	camqp_data** enc_values = camqp_util_new(count*sizeof(camqp_data*));
	if (!enc_values)
		return;

	// create encoded representation of map data
	camqp_vector_item* k = set->data;
	for (
		uint32_t j = 0
			;
		j < count && k != NULL
			;
		j++,
		k = k->next
	) {
		// encode element
		enc_values[j] = camqp_element_encode(k->value);
	}

	// count encoded items
	uint32_t e_len = 0;
	for (uint32_t x = 0; x < count; x++)
		e_len += enc_values[x]->size;

	uint8_t code;
	uint32_t f_len = 0;
	uint32_t t_len = e_len + 4; // count size
	if (t_len <= 0xFF) {
		code = 0xC0; // list8
		f_len = 1 + 1 + 1 + e_len;
	} else {
		code = 0xD0; // list32
		f_len = 1 + 4 + 4 + e_len;
	}

	// allocate working data
	camqp_byte* wk = camqp_util_new(f_len*sizeof(camqp_byte));
	if (!wk) {
		// cleanup
		for (uint32_t j = 0; j < count; j++)
			camqp_data_free(enc_values[j]);

		camqp_util_free(enc_values);
		return;
	}

	camqp_byte* content = NULL;
	memcpy(wk, &code, 1);
	if (t_len <= 0xFF) {
		uint8_t len = e_len + 1;
		memcpy(wk+1, &len, 1);
		memcpy(wk+2, &count, 1);
		content = wk+3;
	} else {
		uint32_t len_conv = htonl((e_len + 4));
		memcpy(wk+1, &len_conv, 4);
		uint32_t cnt_conv = htonl(count);
		memcpy(wk+5, &cnt_conv, 4);
		content = wk+9;
	}

	for (uint32_t x = 0; x < count; x++) {
		memcpy(content, enc_values[x]->bytes, enc_values[x]->size);
		content += enc_values[x]->size;
	}

	// cleanup
	for (uint32_t j = 0; j < count; j++)
		camqp_data_free(enc_values[j]);

	camqp_util_free(enc_values);

	// copy working data to camqp_data structure
	camqp_data* data = camqp_data_new(wk, f_len);
	// free working data
	camqp_util_free(wk);
	// set the result
	*buffer = data;
}
// ---

/// map
void camqp_encode_map(camqp_vector* set, camqp_data** buffer) {
	uint32_t count = 0;

	camqp_vector_item* i = set->data;
	while (i != NULL) {
		count++;
		i = i->next;
	}

	camqp_data** enc_keys = (camqp_data**) camqp_util_new(count*sizeof(camqp_data*));
	if (!enc_keys)
		return;

	camqp_data** enc_values = camqp_util_new(count*sizeof(camqp_data*));
	if (!enc_values) {
		camqp_util_free(enc_keys);
		return;
	}

	// create encoded representation of map data
	camqp_vector_item* k = set->data;
	for (
		uint32_t j = 0
			;
		j < count && k != NULL
			;
		j++,
		k = k->next
	) {
		// encode key
		camqp_scalar* pt_key = camqp_scalar_string(set->base.base.context, CAMQP_TYPE_STRING, k->key);
		enc_keys[j] = camqp_element_encode((camqp_element*) pt_key);
		camqp_scalar_free(pt_key);

		// encode element
		enc_values[j] = camqp_element_encode(k->value);
	}

	// count encoded items
	uint32_t e_count = 2*count; // keys & values
	uint32_t e_len = 0;

	for (uint32_t x = 0; x < count; x++) {
		e_len += enc_keys[x]->size;
		e_len += enc_values[x]->size;
	}

	uint8_t code;
	uint32_t f_len = 0;
	uint32_t t_len = e_len + 4; // count size
	if (t_len <= 0xFF) {
		code = 0xC1; // map8
		f_len = 1 + 1 + 1 + e_len;
	} else {
		code = 0xD1; // map32
		f_len = 1 + 4 + 4 + e_len;
	}

	// allocate working data
	camqp_byte* wk = camqp_util_new(f_len*sizeof(camqp_byte));
	if (!wk) {
		// cleanup
		for (uint32_t j = 0; j < count; j++) {
			camqp_data_free(enc_keys[j]);
			camqp_data_free(enc_values[j]);
		}

		camqp_util_free(enc_keys);
		camqp_util_free(enc_values);
		return;
	}

	camqp_byte* content = NULL;
	memcpy(wk, &code, 1);
	if (t_len <= 0xFF) {
		uint8_t len = e_len + 1;
		memcpy(wk+1, &len, 1);
		memcpy(wk+2, &e_count, 1);
		content = wk+3;
	} else {
		uint32_t len_conv = htonl((e_len + 4));
		memcpy(wk+1, &len_conv, 4);
		uint32_t cnt_conv = htonl(e_count);
		memcpy(wk+5, &cnt_conv, 4);
		content = wk+9;
	}

	for (uint32_t x = 0; x < count; x++) {
		memcpy(content, enc_keys[x]->bytes, enc_keys[x]->size);
		content += enc_keys[x]->size;

		memcpy(content, enc_values[x]->bytes, enc_values[x]->size);
		content += enc_values[x]->size;
	}

	// cleanup
	for (uint32_t j = 0; j < count; j++) {
		camqp_data_free(enc_keys[j]);
		camqp_data_free(enc_values[j]);
	}

	camqp_util_free(enc_keys);
	camqp_util_free(enc_values);

	// copy working data to camqp_data structure
	camqp_data* data = camqp_data_new(wk, f_len);
	// free working data
	camqp_util_free(wk);
	// set the result
	*buffer = data;
}
// ---

// ---

/// composite
void camqp_encode_composite(camqp_composite* element, camqp_data** buffer) {
	// relative xpath query for field elements
	xmlNodePtr backup = element->base.context->xpath->node;
	element->base.context->xpath->node = element->type_def;
	xmlXPathObjectPtr xp = xmlXPathEvalExpression((xmlChar*) "./amqp:field", element->base.context->xpath);
	element->base.context->xpath->node = backup;

	// treat count of elements
	uint32_t count = xp->nodesetval->nodeNr;
	if (count == 0) {
		camqp_encode_null(buffer);
		xmlXPathFreeObject(xp);
	}

	// create new vector for type representation
	camqp_data** enc_values = camqp_util_new(count*sizeof(camqp_data*));
	if (!enc_values) {
		xmlXPathFreeObject(xp);
		return;
	}

	// we will use this
	camqp_element* el_null = (camqp_element*) camqp_primitive_null(element->base.context);

	// for each field
	for (uint32_t i = 0; i < count; i++) {
		xmlNodePtr node = xp->nodesetval->nodeTab[i];

		// get field name
		xmlChar* fname = xmlGetProp(node, (xmlChar*) "name");

		// get field data from composite
		camqp_element* actual_element = camqp_composite_field_get(element, (camqp_char*) fname);

		xmlFree(fname);

		// missing fields check
		if (actual_element == NULL) {
			// check if it can be missing, because it is
			xmlChar* mandatory = xmlGetProp(node, (xmlChar*) "mandatory");
			if (xmlStrcmp(mandatory, (xmlChar*) "true") == 0) {
				// can't be missing
				xmlFree(mandatory);
				xmlXPathFreeObject(xp);

				for (uint32_t j = 0; j < count; j++)
					camqp_data_free(enc_values[j]);
				camqp_util_free(enc_values);

				camqp_element_free(el_null);

				return;
			}
			xmlFree(mandatory);

			// can be missing, use NULL element
			actual_element = el_null;
		}

		// encode element
		enc_values[i] = camqp_element_encode(actual_element);
	}

	// free xpath
	xmlXPathFreeObject(xp);

	// no need anymore
	camqp_element_free(el_null);

	// count total size of encoded items
	uint32_t e_len = 0;
	for (uint32_t i = 0; i < count; i++)
		e_len += enc_values[i]->size;

	uint8_t e_code; // encoding code
	uint32_t t_code = htonl(element->code); // type code
	uint32_t f_len; // full length

	uint32_t t_len = e_len + 4; // length of list encoded (4 for count)
	if (t_len <= 0xFF) {
		// list8
		f_len = 1 + 1 + 4 + 1 + 1 + 1 + e_len;
		e_code = 0xC0;
	} else {
		// list32
		f_len = 1 + 1 + 4 + 1 + 4 + 4 + e_len;
		e_code = 0xD0;
	}

	// allocate working data
	camqp_byte* wk = camqp_util_new(f_len*sizeof(camqp_byte));
	if (!wk) {
		// cleanup
		for (uint32_t j = 0; j < count; j++)
			camqp_data_free(enc_values[j]);
		camqp_util_free(enc_values);
		return;
	}

	// setup data
	memset(wk,   0x00, 1); // composite type
	memset(wk+1, 0x70, 1); // uint descriptor
	memcpy(wk+2, (void*) &t_code, 4); // type code
	memcpy(wk+6, (void*) &e_code, 1); // list encoding

	camqp_byte* start_1 = wk + 7;
	camqp_byte* start_2;
	if (t_len <= 0xFF) {
		// list8
		uint8_t x_len = e_len + 1;
		uint8_t y_len = count;
		memcpy(start_1, (void*) &x_len, 1);
		memcpy(start_1+1, (void*) &y_len, 1);
		start_2 = start_1+2;
	} else {
		// list32
		uint32_t x_len = htonl(e_len + 4);
		uint32_t y_len = htonl(count);
		memcpy(start_1, (void*) &x_len, 4);
		memcpy(start_1+4, (void*) &y_len, 4);
		start_2 = start_1+8;
	}

	for (uint32_t j = 0; j < count; j++) {
		memcpy(start_2, enc_values[j]->bytes, enc_values[j]->size);
		start_2 += enc_values[j]->size;
	}

	// cleanup
	for (uint32_t j = 0; j < count; j++)
		camqp_data_free(enc_values[j]);
	camqp_util_free(enc_values);

	// copy working data to camqp_data structure
	camqp_data* data = camqp_data_new(wk, f_len);
	// free working data
	camqp_util_free(wk);
	// set the result
	*buffer = data;
}
// ---
