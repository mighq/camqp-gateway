#include "camqp.h"
#include "internals.h"

#include <string.h> // memcpy

/// element
camqp_element* camqp_element_decode(camqp_context* context, camqp_data* binary, camqp_data* left) {
	camqp_element* ret = NULL;

	memset(left, 0x00, sizeof(camqp_data));

	camqp_byte* ptr = binary->bytes;
	if (*ptr == 0x00)
		ret = camqp_decode_composite(context, binary, left);
	else
		ret = camqp_decode_primitive(context, binary, left);

	if (left->size == 0)
		left->bytes = NULL;

	return ret;
}
// ---

/// primitive
camqp_element* camqp_decode_primitive(camqp_context* context, camqp_data* binary, camqp_data* left) {
	camqp_element* ret = NULL;

	camqp_byte*	ptr = binary->bytes;
	camqp_byte	type_code = *ptr;

	if (type_code == 0x40) {
		// null
		ret = (camqp_element*) camqp_primitive_null(context);

		// skip 1 byte
		left->bytes = binary->bytes + 1;
		left->size = binary->size - 1;
	} else {
		// non-null primitives
		switch (type_code) {
			case 0x41:
			case 0x42:
			case 0x50:
			case 0x60:
			case 0x70:
			case 0x80:
			case 0x51:
			case 0x61:
			case 0x71:
			case 0x81:
			case 0x72:
			case 0x82:
			case 0x74:
			case 0x84:
			case 0x73:
			case 0x83:
			case 0x98:
			case 0xA0:
			case 0xB0:
			case 0xA1:
			case 0xB1:
			case 0xA3:
			case 0xB3:
				// scalar
				ret = camqp_decode_scalar(context, binary, left);
				break;
			case 0xC0:
			case 0xD0:
			case 0xC1:
			case 0xD1:
				// vector
				ret = camqp_decode_vector(context, binary, left);
				break;
			default:
				// unknown or unimplemented type
				return NULL;
		}
	}

	return ret;
}
// ---

/// scalar

/// common
camqp_element* camqp_decode_scalar(camqp_context* context, camqp_data* binary, camqp_data* left) {
	camqp_element* ret = NULL;

	camqp_byte*	ptr = binary->bytes;
	camqp_byte	type_code = *ptr;

	// move pointer to scalar data
	camqp_data scalar_data;
	scalar_data.bytes = binary->bytes + 1;
	scalar_data.size = binary->size - 1;

	// also move pointer to left data
	left->bytes = scalar_data.bytes;
	left->size = scalar_data.size;

	switch (type_code) {
		case 0x41:
		case 0x42:
			ret = camqp_decode_scalar_bool(context, &scalar_data, left, type_code);
			break;
		case 0x50:
		case 0x60:
		case 0x70:
		case 0x80:
			ret = camqp_decode_scalar_uint(context, &scalar_data, left, type_code);
			break;
		case 0x51:
		case 0x61:
		case 0x71:
		case 0x81:
		case 0x83:
			ret = camqp_decode_scalar_int(context, &scalar_data, left, type_code);
			break;
		case 0x72:
		case 0x74:
			ret = camqp_decode_scalar_float(context, &scalar_data, left, type_code);
			break;
		case 0x82:
		case 0x84:
			ret = camqp_decode_scalar_double(context, &scalar_data, left, type_code);
			break;
		case 0x98:
			ret = camqp_decode_scalar_uuid(context, &scalar_data, left);
			break;
		case 0xA0:
		case 0xB0:
			ret = camqp_decode_scalar_binary(context, &scalar_data, left, type_code);
			break;
		case 0x73:
		case 0xA1:
		case 0xB1:
		case 0xA3:
		case 0xB3:
			ret = camqp_decode_scalar_string(context, &scalar_data, left, type_code);
			break;
		default:
			// unknown or unimplemented type

			// return pointer to left data
			left->bytes = binary->bytes;
			left->size = binary->size;

			return NULL;
	}

	return ret;
}
// ---

/// bool
camqp_element* camqp_decode_scalar_bool(camqp_context* context, camqp_data* binary, camqp_data* left, camqp_byte type_code) {
	return (camqp_element*) camqp_scalar_bool(context, type_code == 0x41);
}
// ---

/// uint
camqp_element* camqp_decode_scalar_uint(camqp_context* context, camqp_data* binary, camqp_data* left, camqp_byte type_code) {
	uint64_t	data;
	camqp_type	type;
	uint8_t		offset;

	camqp_byte* ptr = binary->bytes;

	switch (type_code) {
		case 0x50:
			type = CAMQP_TYPE_UBYTE;
			data = *ptr;
			offset = 1;
			break;
		case 0x60:
			type = CAMQP_TYPE_USHORT;
			offset = 2;
			uint16_t x16;
			memcpy(&x16, ptr, offset);
			data = ntohs(x16);
			break;
		case 0x70:
			type = CAMQP_TYPE_UINT;
			offset = 4;
			uint32_t x32;
			memcpy(&x32, ptr, offset);
			data = ntohl(x32);
			break;
		case 0x80:
			type = CAMQP_TYPE_ULONG;
			offset = 8;
			uint64_t x64;
			memcpy(&x64, ptr, offset);
			data = ntohll(x64);
			break;

		default:
			return NULL;
	}

	// create element
	camqp_scalar* el = camqp_scalar_uint(context, type, data);
	if (!el)
		return NULL;

	// shift left
	left->bytes += offset;
	left->size -= offset;

	return (camqp_element*) el;
}
// ---

/// int
camqp_element* camqp_decode_scalar_int(camqp_context* context, camqp_data* binary, camqp_data* left, camqp_byte type_code) {
	int64_t		data;
	camqp_type	type;
	uint8_t		offset;

	camqp_byte* ptr = binary->bytes;

	switch (type_code) {
		case 0x51:
			type = CAMQP_TYPE_BYTE;
			data = (int64_t) fromtc8(*ptr);
			offset = 1;
			break;
		case 0x61:
			type = CAMQP_TYPE_SHORT;
			offset = 2;
			uint16_t x16;
			memcpy(&x16, ptr, offset);
			x16 = ntohs(x16);
			data = (int64_t) fromtc16(x16);
			break;
		case 0x71:
			type = CAMQP_TYPE_INT;
			offset = 4;
			uint32_t x32;
			memcpy(&x32, ptr, offset);
			x32 = ntohl(x32);
			data = (int64_t) fromtc32(x32);
			break;
		case 0x81:
			type = CAMQP_TYPE_LONG;
			offset = 8;
			uint64_t x64;
			memcpy(&x64, ptr, offset);
			x64 = ntohll(x64);
			data = fromtc64(x64);
			break;

		default:
			return NULL;
	}

	// create element
	camqp_scalar* el = camqp_scalar_int(context, type, data);
	if (!el)
		return NULL;

	// shift left
	left->bytes += offset;
	left->size -= offset;

	return (camqp_element*) el;
}
// ---

/// float
camqp_element* camqp_decode_scalar_float(camqp_context* context, camqp_data* binary, camqp_data* left, camqp_byte type_code) {
	camqp_type	type;
	uint8_t		offset = 4;

	switch (type_code) {
		case 0x72:
			type = CAMQP_TYPE_FLOAT;
			break;
		case 0x74:
			type = CAMQP_TYPE_DECIMAL32;
			break;

		default:
			return NULL;
	}

	// copy data
	uint32_t x;
	memcpy(&x, binary->bytes, offset);
	x = ntohl(x);
	float* y = (float*) &x;
	float data = *y;

	// create element
	camqp_scalar* el = camqp_scalar_float(context, type, data);
	if (!el)
		return NULL;

	// shift left
	left->bytes += offset;
	left->size -= offset;

	return (camqp_element*) el;
}
// ---

/// double
camqp_element* camqp_decode_scalar_double(camqp_context* context, camqp_data* binary, camqp_data* left, camqp_byte type_code) {
	camqp_type	type;
	uint8_t		offset = 8;

	switch (type_code) {
		case 0x82:
			type = CAMQP_TYPE_DOUBLE;
			break;
		case 0x84:
			type = CAMQP_TYPE_DECIMAL64;
			break;

		default:
			return NULL;
	}

	// copy data
	uint64_t x;
	memcpy(&x, binary->bytes, offset);
	x = ntohll(x);
	double* y = (double*) &x;
	double data = *y;

	// create element
	camqp_scalar* el = camqp_scalar_double(context, type, data);
	if (!el)
		return NULL;

	// shift left
	left->bytes += offset;
	left->size -= offset;

	return (camqp_element*) el;
}
// ---

/// uuid
camqp_element* camqp_decode_scalar_uuid(camqp_context* context, camqp_data* binary, camqp_data* left) {
	// check
	if (binary->size < 16)
		return NULL;

	camqp_uuid id;
	memcpy(&id, binary->bytes, 16);

	left->bytes = binary->bytes + 16;
	left->size = binary->size - 16;

	camqp_scalar* tp = camqp_scalar_new(context, CAMQP_TYPE_UUID);
	if (!tp)
		return NULL;

	uuid_copy(tp->data.uid, id);

	return (camqp_element*) tp;
}
// ---

/// binary
camqp_element* camqp_decode_scalar_binary(camqp_context* context, camqp_data* binary, camqp_data* left, camqp_byte type_code) {
	if (!binary || binary->size < 1)
		return NULL;

	// pointer to data
	camqp_byte* ptr = binary->bytes;

	// decode type code
	uint32_t	bin_len;
	uint8_t		header_size;

	switch (type_code) {
		case 0xA0:
			bin_len = (*ptr);
			header_size = 1;
			break;
		case 0xB0:
			memcpy(&bin_len, ptr, 4);
			bin_len = ntohl(bin_len);
			header_size = 4;
			break;

		default:
			return NULL;
	}

	// create data pointer
	camqp_data data = camqp_data_static(ptr+header_size, bin_len);

	// crete element
	camqp_scalar* element = camqp_scalar_binary(context, &data);
	if (!element)
		return NULL;

	left->bytes = binary->bytes + (header_size + bin_len);
	left->size = binary->size - (header_size + bin_len);

	return (camqp_element*) element;
}
// ---

/// string
camqp_element* camqp_decode_scalar_string(camqp_context* context, camqp_data* binary, camqp_data* left, camqp_byte type_code) {
	if (!binary || binary->size < 1)
		return NULL;

	// pointer to data
	camqp_byte* ptr = binary->bytes;

	// decode type code
	camqp_byte*	string_start;
	uint32_t	string_len;
	camqp_type	scalar_type;
	uint8_t		header_size;

	switch (type_code) {
		case 0x73:
			string_start = ptr;
			string_len = 4;
			scalar_type = CAMQP_TYPE_CHAR;
			header_size = 0;
			break;
		case 0xA1:
			string_start = ptr+1;
			string_len = (*ptr);
			scalar_type = CAMQP_TYPE_STRING;
			header_size = 1;
			break;
		case 0xB1:
			string_start = ptr+4;
			memcpy(&string_len, ptr, 4);
			string_len = ntohl(string_len);
			scalar_type = CAMQP_TYPE_STRING;
			header_size = 4;
			break;
		case 0xA3:
			string_start = ptr+1;
			string_len = (*ptr);
			scalar_type = CAMQP_TYPE_SYMBOL;
			header_size = 1;
			break;
		case 0xB3:
			string_start = ptr+4;
			memcpy(&string_len, ptr, 4);
			string_len = ntohl(string_len);
			scalar_type = CAMQP_TYPE_SYMBOL;
			header_size = 4;
			break;

		default:
			return NULL;
	}

	// create string
	camqp_char* data = (camqp_char*) camqp_util_new((string_len+1)*sizeof(camqp_char));
	if (!data)
		return NULL;

	// move data to string
	memcpy(data, string_start, string_len);

	// create element
	camqp_scalar* element = camqp_scalar_string(context, scalar_type, data);
	camqp_util_free(data);

	if (!element)
		return NULL;

	left->bytes = binary->bytes + header_size + string_len;
	left->size = binary->size - (header_size + string_len);

	return (camqp_element*) element;
}
// ---

// ---

/// vector
camqp_element* camqp_decode_vector(camqp_context* context, camqp_data* binary, camqp_data* left) {
	return NULL;
}
// ---

/// composite
camqp_element* camqp_decode_composite(camqp_context* context, camqp_data* binary, camqp_data* left) {
	camqp_byte* ptr = binary->bytes;

	// check for primitive
	if (*ptr != 0x00)
		return NULL;

	left->bytes = binary->bytes;
	left->size = binary->size;

	// check that we have uint as type identifier
	if (*(ptr+1) != 0x70)
		return NULL;

	left->bytes = ptr+1;
	left->size = binary->size - 1;

	// get composite_id
	uint32_t composite_id;
	memcpy(&composite_id, ptr+2, 4);
	composite_id = ntohl(composite_id);

	left->bytes = ptr+6;
	left->size = binary->size - 6;

	// detect name from type definition
	const uint8_t str_len = 128;
	camqp_char expr[str_len];
	snprintf((char*) expr, str_len, "//amqp:type[@class='composite']/amqp:descriptor[@code='0x%08X']", composite_id);

	// evaluate xpath
	xmlXPathObjectPtr xp = xmlXPathEvalExpression(expr, context->xpath);
	if (!xp)
		return NULL;

	// no such type found, or found many
	if (xp->nodesetval->nodeNr != 1) {
		xmlXPathFreeObject(xp);
		return NULL;
	}

	// save type element pointer
	xmlNodePtr type_element = xp->nodesetval->nodeTab[0]->parent;
	xmlXPathFreeObject(xp);

	// get name from XML definition
	xmlChar* name_str = xmlGetProp(type_element, (xmlChar*) "name");
	if (!name_str)
		// name is missing!
		return NULL;

	// create composite element
	camqp_composite* composite = camqp_composite_new(context, (const camqp_char*) name_str, composite_id);
	if (!composite) {
		xmlFree(name_str);
		return NULL;
	}
	xmlFree(name_str);

	// setup pointer to type
	composite->type_def = type_element;

	// decode fields

	// only accept list encoding for fields
	uint8_t field_enc = *(ptr+6);
	if (field_enc != 0xC0 && field_enc != 0xD0) {
		camqp_element_free((camqp_element*) composite);
		return NULL;
	}

	left->bytes = ptr+7;
	left->size = binary->size - 7;

	uint32_t total_size;
	uint32_t fields_size;
	uint32_t fields_count;
	camqp_byte* fields_start;
	if (field_enc == 0xC0) {
		// list8
		uint8_t x;
		memcpy(&x, ptr+7, 1);
		total_size = x;
		memcpy(&x, ptr+8, 1);
		fields_count = x;
		fields_start = ptr+9;
		fields_size = total_size-1;

		left->bytes = ptr+9;
		left->size = binary->size - 9;
	} else {
		// list32
		memcpy(&total_size, ptr+7, 4);
		total_size = ntohl(total_size);
		memcpy(&fields_count, ptr+11, 4);
		fields_count = ntohl(fields_count);
		fields_start = ptr+15;
		fields_size = total_size - 4;

		left->bytes = ptr+15;
		left->size = binary->size - 15;
	}

	// decode items
	uint32_t read_size = 0;
	for (uint32_t i = 0; i < fields_count; i++) {
		// prepare pointers to non-decoded data
		camqp_data todo;
		todo.bytes = left->bytes;
		todo.size = left->size;

		// decode element
		camqp_element* decoded = camqp_element_decode(context, &todo, left);
		if (!decoded) {
			puts("nejde dekodovat");
			// unable to decode element, break whole decoding
			camqp_element_free((camqp_element*) composite);
			return NULL;
		}

		// increment already done counter
		read_size += (todo.size - left->size);

		// detect key name for this field
		xmlChar* fld_name = NULL;
		{
			// detect name from type definition
			const uint8_t str_len = 128;
			camqp_char expr[str_len];
			snprintf((char*) expr, str_len, "./amqp:field[position()=%d]", (i+1));

			// relative xpath query for field elements
			xmlNodePtr backup = composite->base.context->xpath->node;
			composite->base.context->xpath->node = composite->type_def;
			xmlXPathObjectPtr xp = xmlXPathEvalExpression((xmlChar*) expr, composite->base.context->xpath);
			composite->base.context->xpath->node = backup;

			if (!xp) {
				camqp_element_free(decoded);
				camqp_element_free((camqp_element*) composite);

				return NULL;
			}

			// no such type found, or found many
			if (xp->type != XPATH_NODESET || xp->nodesetval->nodeNr != 1) {
				xmlXPathFreeObject(xp);

				camqp_element_free(decoded);
				camqp_element_free((camqp_element*) composite);

				return NULL;
			}

			// field element
			xmlNodePtr el_fld = xp->nodesetval->nodeTab[0];
			xmlXPathFreeObject(xp);

			// get name
			fld_name = xmlGetProp(el_fld, (xmlChar*) "name");
			if (!fld_name) {
				// field name missing
				camqp_element_free(decoded);
				camqp_element_free((camqp_element*) composite);

				return NULL;
			}
		}

		// add it to composite
		if (!camqp_composite_field_put(composite, (const camqp_char*) fld_name, decoded)) {
			xmlFree(fld_name);

			camqp_element_free(decoded);
			camqp_element_free((camqp_element*) composite);

			return NULL;
		}

		xmlFree(fld_name);
	}

	if (fields_size != read_size) {
		// warning, no all was read
	}

	return (camqp_element*) composite;
}
// ---

