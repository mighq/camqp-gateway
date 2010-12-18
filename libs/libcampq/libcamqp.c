#include <camqp.h>

#include <string.h> // memset, memcpy

#include <libxml/xmlstring.h>
#include <libxml/parser.h>
#include <libxml/xinclude.h>
#include <libxml/xpathInternals.h>

#include <netinet/in.h>

// TODO: what about other platforms?
#define ntohll(x) ( ( (uint64_t)(ntohl( (uint32_t)((x << 32) >> 32) )) << 32) | ntohl( ((uint32_t)(x >> 32)) ) )
#define htonll(x) ntohll(x)

uint64_t twos_complement(int64_t nr) {
	// positive
	if (nr >= 0)
		return nr;

	// negative
	uint64_t nu = llabs(nr) ^ 0xFFFFFFFFFFFFFFFF;
	return nu + 0x01;
}

// TODO: vector a scalar odvodit od primitive
// TODO u vekturu urcit typ MAP/LIST, do listu neprijat nenumericke keys

/// utils

/**
 *	allocate memory for element
 *	make sure all its bytes are set to 0x00
 *
 *	@param integer size
 *		size of allocated data
 *	@return pointer
 *		pointer to newly allocated data
 */
void* camqp_util_new(camqp_size size) {
	void* mem = NULL;

	// check size
	if (size < 1)
		return NULL;

	// allocate
	mem = malloc(size);
	if (mem == NULL)
		return NULL;

	// set to 0x00
	memset(mem, 0x00, size);

	return mem;
}

/**
 *	frees data
 *
 *	@param pointer data
 *		pointer to data for deallocation
 *	@return void
 */
void camqp_util_free(void* data) {
	if (data == NULL)
		return;

	free(data);
}

char* dump_data(unsigned char* pointer, unsigned int length)
{
	if (pointer == NULL || length == 0)
		return NULL;

	char* ret = malloc(length*5+1);
	if (!ret)
		return NULL;

	memset(ret, '_', length*5);

	unsigned int i;
	for (i = 0; i < length; i++) {
		memcpy(ret+(5*i), "0x", 2);
		ret[5*i + 2] = "0123456789ABCDEF"[pointer[i] >> 4];
		ret[5*i + 3] = "0123456789ABCDEF"[pointer[i] & 0x0F];
		ret[5*i + 4] = ' ';
	}
	ret[length*5] = 0x00;

	return ret;
}

camqp_char* camqp_data_dump(camqp_data* data) {
	if (data == NULL)
		return NULL;

	return (camqp_char*) dump_data(data->bytes, data->size);
}

bool camqp_is_numeric(const camqp_char* string) {
	if (string == NULL)
		return false;

	bool ret = true;
	camqp_char* p = (camqp_char*) string;
	while (*p != 0x00) {
		if (*p < 0x30 || *p > 0x39) {
			ret = false;
			break;
		}
		p++;
	}

	return ret;
}
// ---

/// camqp_data

camqp_data camqp_data_static(const camqp_byte* data, camqp_size size) {
	camqp_data ret;
	ret.bytes = (camqp_byte*) data;
	ret.size = size;
	return ret;
}

camqp_data* camqp_data_new(const camqp_byte* bytes, camqp_size size) {
	camqp_data* ret;

	// allocate return structure
	ret = camqp_util_new(sizeof(camqp_data));
	if (!ret)
		return NULL;

	// duplicate data
	ret->bytes = camqp_util_new(size);
	if (!ret->bytes) {
		camqp_util_free(ret);
		return NULL;
	}
	memcpy(ret->bytes, bytes, size);
	ret->size = size;

	return ret;
}

void camqp_data_free(camqp_data* binary) {
	if (binary == NULL)
		return;

	camqp_util_free(binary->bytes);
	camqp_util_free(binary);
}
// ---

/// camqp_context
/**
 * TODO: filename is not UTF-8 ready!
 */
camqp_context* camqp_context_new(const camqp_char* protocol, const camqp_char* definition) {
	// allocate result structure
	camqp_context* ctx = camqp_util_new(sizeof(camqp_context));
	if (!ctx)
		return NULL;

	// save protocol file and name
	ctx->protocol = xmlStrdup(protocol);
	ctx->definition = xmlStrdup(definition);

	// init libxml
	xmlInitParser();
	LIBXML_TEST_VERSION

	// load XML document
	ctx->xml = xmlParseFile((const char*) definition);

	if (ctx->xml == NULL) {
		fprintf(stderr, "Unable to parse file\n");
		xmlCleanupParser();
		camqp_context_free(ctx);
		return NULL;
	}

	if (xmlXIncludeProcess(ctx->xml) <= 0) {
		fprintf(stderr, "XInclude processing failed\n");
		xmlCleanupParser();
		xmlFreeDoc(ctx->xml);
		camqp_context_free(ctx);
		return NULL;
	}

	// Create xpath evaluation context
	ctx->xpath = xmlXPathNewContext(ctx->xml);
	if (ctx->xpath == NULL) {
		fprintf(stderr, "Error: unable to create new XPath context\n");
		xmlCleanupParser();
		xmlFreeDoc(ctx->xml);
		camqp_context_free(ctx);
		return NULL;
	}

	xmlXPathRegisterNs(ctx->xpath, (xmlChar*) "amqp", (xmlChar*) "http://www.amqp.org/schema/amqp.xsd");

	// shutdown libxml
	xmlCleanupParser();

	// now we have parsed XML definition

	// check for correct protocol definition
	xmlNodePtr root = xmlDocGetRootElement(ctx->xml);
	xmlChar* protocol_def = xmlGetProp(root, (xmlChar*) "name");
	xmlChar* protocol_req = xmlStrdup(protocol);

	if (!xmlStrEqual(protocol_def, protocol_req)) {
		fprintf(stderr, "Requested protocol is not the one defined in XML definition!\n");
		xmlFree(protocol_def);
		xmlFree(protocol_req);
		camqp_context_free(ctx);
		return NULL;
	}

	xmlFree(protocol_def);
	xmlFree(protocol_req);

	return ctx;
}

void camqp_context_free(camqp_context* context) {
	if (context == NULL)
		return;

	// free XML document
	xmlXPathFreeContext(context->xpath);
	xmlFreeDoc(context->xml);

	// free string members
	camqp_util_free(context->protocol);
	camqp_util_free(context->definition);

	// free structure
	camqp_util_free(context);
}
// ---

/// camqp_element

void camqp_element_free(camqp_element* element) {
	if (element == NULL)
		return;

	if (element->class == CAMQP_CLASS_PRIMITIVE) {
		camqp_primitive_free((camqp_primitive*) element);
	} else if (element->class == CAMQP_CLASS_COMPOSITE) {
		camqp_composite_free((camqp_composite*) element, true);
	}
}

camqp_primitive* camqp_primitive_null(camqp_context* context) {
	camqp_primitive* tp = camqp_primitive_new(context);
	if (!tp)
		return NULL;

	tp->type = CAMQP_TYPE_NULL;

	return tp;
}

bool camqp_element_is_null(camqp_element* element) {
	if (element->class == CAMQP_CLASS_PRIMITIVE && element->multiple == CAMQP_MULTIPLICITY_SCALAR) {
		camqp_primitive* primitive = (camqp_primitive*) element;
		return (primitive->type == CAMQP_TYPE_NULL);
	}

	return false;
}

// ---

/// camqp_primitive
camqp_primitive* camqp_primitive_new(camqp_context* context) {
	camqp_primitive* ret = camqp_util_new(sizeof(camqp_primitive));
	if (!ret)
		return NULL;

	ret->base.context = context;
	ret->base.class = CAMQP_CLASS_PRIMITIVE;
	ret->base.multiple = CAMQP_MULTIPLICITY_SCALAR;

	return ret;
}

void camqp_primitive_free(camqp_primitive* element) {
	if (element == NULL)
		return;

	if (element->base.multiple == CAMQP_MULTIPLICITY_SCALAR) {
		// for dynamic elements free data memory
		if (
			element->type == CAMQP_TYPE_STRING
				||
			element->type == CAMQP_TYPE_CHAR
				||
			element->type == CAMQP_TYPE_SYMBOL
		)
			camqp_util_free(element->data.str);

		if (element->type == CAMQP_TYPE_BINARY)
			camqp_data_free(element->data.bin);

		// free element
		camqp_util_free(element);
	} else if (element->base.multiple == CAMQP_MULTIPLICITY_VECTOR) {
		camqp_vector_free((camqp_vector*) element, true);
	}
}

bool camqp_element_is_primitive(camqp_element* element) {
	return element->class == CAMQP_CLASS_PRIMITIVE;
}

bool camqp_element_is_scalar(camqp_element* element) {
	return element->multiple == CAMQP_MULTIPLICITY_SCALAR;
}

/// bool
camqp_primitive* camqp_primitive_bool(camqp_context* context, bool value) {
	camqp_primitive* tp = camqp_primitive_new(context);
	if (!tp)
		return NULL;

	tp->type = CAMQP_TYPE_BOOLEAN;
	tp->data.b = value;

	return tp;
}

bool camqp_value_bool(camqp_primitive* element) {
	if (!camqp_element_is_primitive((camqp_element*) element))
		return false;

	if (!camqp_element_is_scalar((camqp_element*) element))
		return false;

	if (element->type != CAMQP_TYPE_BOOLEAN)
		return false;

	return element->data.b;
}
// ---

/// int

// BYTE, SHORT, INT, LONG, TIMESTAMP
camqp_primitive* camqp_primitive_int(camqp_context* context, camqp_type type, int64_t value) {
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

	camqp_primitive* tp = camqp_primitive_new(context);
	if (!tp)
		return NULL;

	tp->type = type;
	tp->data.i = value;

	return tp;
}

int64_t camqp_value_int(camqp_primitive* element) {
	if (!camqp_element_is_primitive((camqp_element*) element))
		return 0;

	if (!camqp_element_is_scalar((camqp_element*) element))
		return 0;

	if (
		element->type != CAMQP_TYPE_BYTE
			&&
		element->type != CAMQP_TYPE_SHORT
			&&
		element->type != CAMQP_TYPE_INT
			&&
		element->type != CAMQP_TYPE_LONG
			&&
		element->type != CAMQP_TYPE_TIMESTAMP
	)
		return 0;

	return element->data.i;
}
// ---

/// uint

// UBYTE, USHORT, UINT, ULONG
camqp_primitive* camqp_primitive_uint(camqp_context* context, camqp_type type, uint64_t value) {
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

	camqp_primitive* tp = camqp_primitive_new(context);
	if (!tp)
		return NULL;

	tp->type = type;
	tp->data.ui = value;

	return tp;
}

uint64_t camqp_value_uint(camqp_primitive* element) {
	if (!camqp_element_is_primitive((camqp_element*) element))
		return 0;

	if (!camqp_element_is_scalar((camqp_element*) element))
		return 0;

	if (
		element->type != CAMQP_TYPE_UBYTE
			&&
		element->type != CAMQP_TYPE_USHORT
			&&
		element->type != CAMQP_TYPE_UINT
			&&
		element->type != CAMQP_TYPE_ULONG
	)
		return 0;

	return element->data.ui;
}
// ---

/// float

// DECIMAL32, FLOAT
camqp_primitive* camqp_primitive_float(camqp_context* context, camqp_type type, float value) {
	if (
		type != CAMQP_TYPE_DECIMAL32
			&&
		type != CAMQP_TYPE_FLOAT
	)
		return NULL;

	camqp_primitive* tp = camqp_primitive_new(context);
	if (!tp)
		return NULL;

	tp->type = type;
	tp->data.f = value;

	return tp;
}

float camqp_value_float(camqp_primitive* element) {
	if (!camqp_element_is_primitive((camqp_element*) element))
		return 0;

	if (!camqp_element_is_scalar((camqp_element*) element))
		return 0;

	if (
		element->type != CAMQP_TYPE_FLOAT
			&&
		element->type != CAMQP_TYPE_DECIMAL32
	)
		return 0;

	return element->data.f;
}
// ---

/// double

// DECIMAL64, DOUBLE
camqp_primitive* camqp_primitive_double(camqp_context* context, camqp_type type, double value) {
	if (
		type != CAMQP_TYPE_DECIMAL64
			&&
		type != CAMQP_TYPE_DOUBLE
	)
		return NULL;

	camqp_primitive* tp = camqp_primitive_new(context);
	if (!tp)
		return NULL;

	tp->type = type;
	tp->data.d = value;

	return tp;
}

double camqp_value_double(camqp_primitive* element) {
	if (!camqp_element_is_primitive((camqp_element*) element))
		return 0;

	if (!camqp_element_is_scalar((camqp_element*) element))
		return 0;

	if (
		element->type != CAMQP_TYPE_DOUBLE
			&&
		element->type != CAMQP_TYPE_DECIMAL64
	)
		return 0;

	return element->data.d;
}
// ---

/// string

// STRING, SYMBOL, CHAR, UUID
camqp_primitive* camqp_primitive_string(camqp_context* context, camqp_type type, const camqp_char* value) {
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

	camqp_primitive* tp = camqp_primitive_new(context);
	if (!tp)
		return NULL;

	tp->type = type;
	tp->data.str = xmlStrdup(value);
	if (!tp->data.str) {
		camqp_util_free(tp);
		return NULL;
	}

	return tp;
}

const camqp_char* camqp_value_string(camqp_primitive* element) {
	if (!camqp_element_is_primitive((camqp_element*) element))
		return NULL;

	if (!camqp_element_is_scalar((camqp_element*) element))
		return NULL;

	if (
		element->type != CAMQP_TYPE_STRING
			&&
		element->type != CAMQP_TYPE_SYMBOL
			&&
		element->type != CAMQP_TYPE_CHAR
	)
		return NULL;

	return element->data.str;
}
// ---

/// binary

camqp_primitive* camqp_primitive_binary(camqp_context* context, camqp_data* value) {
	camqp_primitive* tp = camqp_primitive_new(context);
	if (!tp)
		return NULL;

	tp->type = CAMQP_TYPE_BINARY;
	tp->data.bin = camqp_data_new(value->bytes, value->size);

	return tp;
}

camqp_data* camqp_value_binary(camqp_primitive* element) {
	if (!camqp_element_is_primitive((camqp_element*) element))
		return NULL;

	if (!camqp_element_is_scalar((camqp_element*) element))
		return NULL;

	if (element->type != CAMQP_TYPE_BINARY)
		return NULL;

	return element->data.bin;
}
// ---

/// uuid
camqp_primitive* camqp_primitive_uuid(camqp_context* context) {
	camqp_primitive* tp = camqp_primitive_new(context);
	if (!tp)
		return NULL;

	tp->type = CAMQP_TYPE_UUID;
	uuid_generate(tp->data.uid);

	return tp;
}

const camqp_uuid* camqp_value_uuid(camqp_primitive* element) {
	if (!camqp_element_is_primitive((camqp_element*) element))
		return NULL;

	if (!camqp_element_is_scalar((camqp_element*) element))
		return NULL;

	if (element->type != CAMQP_TYPE_UUID)
		return NULL;

	return (const camqp_uuid*) &element->data.uid;
}
// ---

// ---

/// camqp_vector
camqp_vector_item* camqp_vector_item_new(const camqp_char* key, camqp_element* value) {
	camqp_vector_item* itm = camqp_util_new(sizeof(camqp_vector_item));
	if (!itm)
		return NULL;

	itm->key = xmlStrdup(key);
	if (!itm->key) {
		camqp_util_free(itm);
		return NULL;
	}

	itm->value = value;
	itm->next = NULL;

	return itm;
}

void camqp_vector_item_free(camqp_vector_item* item, bool free_value) {
	if (item == NULL)
		return;

	if (free_value)
		camqp_element_free(item->value);

	camqp_util_free(item->key);
	camqp_util_free(item);
}

camqp_vector* camqp_vector_new(camqp_context* ctx) {
	camqp_vector* vec = camqp_util_new(sizeof(camqp_vector));
	if (!vec)
		return NULL;

	vec->base.context = ctx;

	vec->base.class = CAMQP_CLASS_PRIMITIVE;
	vec->base.multiple = CAMQP_MULTIPLICITY_VECTOR;

	vec->data = NULL;

	return vec;
}

void camqp_vector_free(camqp_vector* vector, bool free_values) {
	if (vector == NULL)
		return;

	// delete all elements from vector
	camqp_vector_item* to_del = vector->data;
	while (to_del) {
		camqp_vector_item* to_del_next = (camqp_vector_item*) to_del->next;
		camqp_vector_item_free(to_del, free_values);
		to_del = to_del_next;
	}

	// delete vector itself
	camqp_util_free(vector);
}

void camqp_vector_item_put(camqp_vector* vector, const camqp_char* key, camqp_element* element) {
	// check if contexts are matching
	if (vector->base.context != element->context)
		return;

	// create new item
	camqp_vector_item* el = camqp_vector_item_new(key, element);
	if (!el)
		return;

	// find position in vector

	// pointer to first item
	camqp_vector_item* before  = vector->data;

	// pointer to second item
	camqp_vector_item* after = NULL;
	if (before) {
		// pointer to second item if first exists
		after = before->next;
	} else {
		// insert into empty vector
		vector->data = el;
		return;
	}

	do {
		// at the beginning
		if (before == vector->data) {
			// compare with first item
			int cmp1 = xmlStrcmp(key, before->key);
			if (cmp1 < 0) {
				// insert to the beginning
				el->next = before;
				vector->data = el;
				return;
			}

			// keys cannot be duplicate!
			if (cmp1 == 0) {
				camqp_vector_item_free(el, false);
				return;
			}
		}

		// after exists?
		if (after == NULL) {
			// at to the end
			before->next = el;
			return;
		}

		// compare with after
		int cmp2 = xmlStrcmp(key, after->key);

		// don't allow duplicate keys
		if (cmp2 == 0) {
			camqp_vector_item_free(el, false);
			return;
		}

		// new is greater than after
		if (cmp2 > 0) {
			// move pointers
			before = after;
			after = after->next;
			continue;
		}

		// this is desired position
		if (cmp2 < 0) {
			// insert before "after"
			before->next = el;
			el->next = after;
			return;
		}
	} while (true);
}

camqp_element* camqp_vector_item_get(camqp_vector* vector, const camqp_char* key) {
	camqp_element* ret = NULL;

	camqp_vector_item* curr = vector->data;
	while (curr) {
		int cmp = xmlStrcmp(key, curr->key);
		if (cmp == 0) {
			ret = curr->value;
			break;
		}

		curr = curr->next;
	}

	return ret;
}

// ---

/// camqp_composite
/**
 *	one of type_name or type_code has to be specified
 *	if both, they must match
 *	validity is checked against protocol definition
 */
camqp_composite* camqp_composite_new(camqp_context* context, const camqp_char* type_name, camqp_code type_code) {
	// check context
	if (context == NULL)
		return NULL;

	// don't know what do create
	if (type_name == NULL && type_code == 0)
		return NULL;

	// TODO: check for valid name & type
	const uint16_t max_expr_length = 128;
	camqp_char* expr = NULL;

	if (type_name != NULL && type_code == 0) {
		uint32_t str_len = (max_expr_length + strlen((char*)type_name)) * sizeof(camqp_char);
		expr = camqp_util_new(str_len + 1);
		snprintf((char*)expr, str_len, "//amqp:type[@name='%s' and @class='composite']/amqp:descriptor[not(@name) or @name='%s']", type_name, type_name);
	}

	if (type_name == NULL && type_code != 0) {
		uint32_t str_len = (max_expr_length) * sizeof(camqp_char);
		expr = camqp_util_new(str_len + 1);
		snprintf((char*)expr, str_len, "//amqp:type[@class='composite']/amqp:descriptor[@code='0x%08X']", type_code);
	}

	if (type_name != NULL && type_code != 0) {
		uint32_t str_len = (max_expr_length) * sizeof(camqp_char);
		expr = camqp_util_new(str_len + 1);
		snprintf((char*)expr, str_len, "//amqp:type[@name='%s' and @class='composite']/amqp:descriptor[@code='0x%08X' and (not(@name) or @name='%s')]", type_name, type_code, type_name);
	}

	xmlXPathObjectPtr xp = xmlXPathEvalExpression(expr, context->xpath);
	camqp_util_free(expr);

	if (!xp)
		return NULL;

	// no such type found, or found many
	if (xp->nodesetval->nodeNr != 1) {
		xmlXPathFreeObject(xp);
		return NULL;
	}

	camqp_char* real_name;
	if (type_name == NULL) {
		// detect from XML
		xmlChar* name_str = xmlGetProp(xp->nodesetval->nodeTab[0], (xmlChar*) "name");
		if (!name_str) {
			// get it from descriptor element
			xmlNodePtr type_elem = xp->nodesetval->nodeTab[0]->parent;
			xmlChar* name_str2 = xmlGetProp(type_elem, (xmlChar*) "name");
			if (!name_str2) {
				// should be set!
				xmlXPathFreeObject(xp);
				return NULL;
			}
			real_name = (camqp_char*) name_str2;
		} else {
			// get it from type element
			real_name = (camqp_char*) name_str;
		}
	} else {
		real_name = xmlStrdup(type_name);
	}

	camqp_code real_code;
	if (type_code == 0) {
		// detect from XML
		xmlChar* code_str = xmlGetProp(xp->nodesetval->nodeTab[0], (xmlChar*) "code");
		if (!code_str) {
			camqp_util_free(real_name);
			xmlXPathFreeObject(xp);
			return NULL;
		}

		sscanf((const char*) code_str, "0x%08X", &real_code);
		xmlFree(code_str);
	} else {
		real_code = type_code;
	}

	xmlXPathFreeObject(xp);

	// create representation
	camqp_composite* comp = camqp_util_new(sizeof(camqp_composite));
	if (!comp)
		return NULL;

	comp->base.context = context;

	comp->base.class = CAMQP_CLASS_COMPOSITE;
	comp->base.multiple = CAMQP_MULTIPLICITY_SCALAR;

	comp->name = real_name;
	comp->code = real_code;

	comp->fields = NULL;

	return comp;
}

void camqp_composite_free(camqp_composite* element, bool free_values) {
	if (element == NULL)
		return;

	// delete all elements from fields
	camqp_vector_item* to_del = element->fields;
	while (to_del) {
		camqp_vector_item* to_del_next = (camqp_vector_item*) to_del->next;
		camqp_vector_item_free(to_del, free_values);
		to_del = to_del_next;
	}

	camqp_util_free(element->name);
	camqp_util_free(element);
}

// TODO spolocny codebase s vektorom
void camqp_composite_field_put(camqp_composite* element, const camqp_char* key, camqp_element* item) {
	// check if contexts are matching
	if (element->base.context != item->context)
		return;

	// create new item
	camqp_vector_item* el = camqp_vector_item_new(key, item);
	if (!el)
		return;

	// find position in vector

	// pointer to first item
	camqp_vector_item* before  = element->fields;

	// pointer to second item
	camqp_vector_item* after = NULL;
	if (before) {
		// pointer to second item if first exists
		after = before->next;
	} else {
		// insert into empty vector
		element->fields = el;
		return;
	}

	do {
		// at the beginning
		if (before == element->fields) {
			// compare with first item
			int cmp1 = xmlStrcmp(key, before->key);
			if (cmp1 < 0) {
				// insert to the beginning
				el->next = before;
				element->fields = el;
				return;
			}

			// keys cannot be duplicate!
			if (cmp1 == 0) {
				camqp_vector_item_free(el, false);
				return;
			}
		}

		// after exists?
		if (after == NULL) {
			// at to the end
			before->next = el;
			return;
		}

		// compare with after
		int cmp2 = xmlStrcmp(key, after->key);

		// don't allow duplicate keys
		if (cmp2 == 0) {
			camqp_vector_item_free(el, false);
			return;
		}

		// new is greater than after
		if (cmp2 > 0) {
			// move pointers
			before = after;
			after = after->next;
			continue;
		}

		// this is desired position
		if (cmp2 < 0) {
			// insert before "after"
			before->next = el;
			el->next = after;
			return;
		}
	} while (true);
}

// TODO spolocny codebase s vektorom
camqp_element* camqp_composite_field_get(camqp_composite* element, const camqp_char* key) {
	camqp_element* ret = NULL;

	camqp_vector_item* curr = element->fields;
	while (curr) {
		int cmp = xmlStrcmp(key, curr->key);
		if (cmp == 0) {
			ret = curr->value;
			break;
		}

		curr = curr->next;
	}

	return ret;
}

// ---

/// encoding
camqp_data* camqp_element_encode(camqp_element* element) {
	camqp_data* encoded = NULL;

	if (element->class == CAMQP_CLASS_PRIMITIVE)
		camqp_encode_primitive((camqp_primitive*) element, &encoded);

	return encoded;
}

void camqp_encode_primitive(camqp_primitive* element, camqp_data** buffer) {
	if (element->base.multiple == CAMQP_MULTIPLICITY_SCALAR) {
		switch (element->type) {
			case CAMQP_TYPE_NULL:
				camqp_encode_primitive_null(buffer);
				break;
			case CAMQP_TYPE_BOOLEAN:
				camqp_encode_primitive_bool(element, buffer);
				break;
			case CAMQP_TYPE_UBYTE:
			case CAMQP_TYPE_USHORT:
			case CAMQP_TYPE_UINT:
			case CAMQP_TYPE_ULONG:
				camqp_encode_primitive_uint(element, buffer);
				break;
			case CAMQP_TYPE_BYTE:
			case CAMQP_TYPE_SHORT:
			case CAMQP_TYPE_INT:
			case CAMQP_TYPE_LONG:
			case CAMQP_TYPE_TIMESTAMP:
				camqp_encode_primitive_int(element, buffer);
				break;
			case CAMQP_TYPE_FLOAT:
			case CAMQP_TYPE_DECIMAL32:
				camqp_encode_primitive_float(element, buffer);
				break;
			case CAMQP_TYPE_DOUBLE:
			case CAMQP_TYPE_DECIMAL64:
				camqp_encode_primitive_double(element, buffer);
				break;
			case CAMQP_TYPE_UUID:
				camqp_encode_primitive_uuid(element, buffer);
				break;
			case CAMQP_TYPE_CHAR:
			case CAMQP_TYPE_STRING:
			case CAMQP_TYPE_SYMBOL:
				camqp_encode_primitive_string(element, buffer);
				break;
			case CAMQP_TYPE_BINARY:
				camqp_encode_primitive_binary(element, buffer);
				break;
		}
	} else if (element->base.multiple == CAMQP_MULTIPLICITY_VECTOR) {
		camqp_encode_vector((camqp_vector*) element, buffer);
	}
}

void camqp_encode_vector(camqp_vector* set, camqp_data** buffer) {
	// for empty array return NULL element
	if (set->data == NULL) {
		camqp_encode_primitive_null(buffer);
		return;
	}

	// determine if keys are numeric
	bool numeric = true;
	camqp_vector_item* item = set->data;
	while (item != NULL) {
		if (!camqp_is_numeric(item->key)) {
			numeric = false;
			break;
		}
		item = item->next;
	}

	// different types of encoding
	if (numeric)
		camqp_encode_list(set, buffer);
	else
		camqp_encode_map(set, buffer);
}

/// null
void camqp_encode_primitive_null(camqp_data** buffer) {
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

/// bool
void camqp_encode_primitive_bool(camqp_primitive* primitive, camqp_data** buffer) {
	if (primitive->type != CAMQP_TYPE_BOOLEAN)
		return;

	// allocate working data
	camqp_byte* wk = camqp_util_new(1*sizeof(camqp_byte));
	if (!wk)
		return;

	if (primitive->data.b == true)
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
void camqp_encode_primitive_uint(camqp_primitive* element, camqp_data** buffer) {
	if (
		element->type != CAMQP_TYPE_UBYTE
			&&
		element->type != CAMQP_TYPE_USHORT
			&&
		element->type != CAMQP_TYPE_UINT
			&&
		element->type != CAMQP_TYPE_ULONG
	)
		return;

	// allocate working data
	camqp_byte* wk = camqp_util_new(9*sizeof(camqp_byte));
	if (!wk)
		return;

	uint8_t len = 0;
	if (element->type == CAMQP_TYPE_UBYTE) {
		len = 2;
		memcpy(wk, "\x50", 1);
		memcpy(wk+1, (void*) &element->data.ui, 1);
	} else if (element->type == CAMQP_TYPE_USHORT) {
		len = 3;
		uint16_t conv = (uint16_t) htons((uint16_t) element->data.ui);
		memcpy(wk, "\x60", 1);
		memcpy(wk+1, (void*) &conv, 2);
	} else if (element->type == CAMQP_TYPE_UINT) {
		len = 5;
		uint32_t conv = (uint32_t) htonl((uint32_t) element->data.ui);
		memcpy(wk, "\x70", 1);
		memcpy(wk+1, (void*) &conv, 4);
	} else if (element->type == CAMQP_TYPE_ULONG) {
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
void camqp_encode_primitive_int(camqp_primitive* element, camqp_data** buffer) {
	if (
		element->type != CAMQP_TYPE_BYTE
			&&
		element->type != CAMQP_TYPE_SHORT
			&&
		element->type != CAMQP_TYPE_INT
			&&
		element->type != CAMQP_TYPE_LONG
			&&
		element->type != CAMQP_TYPE_TIMESTAMP
	)
		return;

	// allocate working data
	camqp_byte* wk = camqp_util_new(9*sizeof(camqp_byte));
	if (!wk)
		return;

	uint8_t len = 0;
	if (element->type == CAMQP_TYPE_BYTE) {
		len = 2;
		uint8_t conv = (uint8_t) twos_complement(element->data.i);
		memcpy(wk, "\x51", 1);
		memcpy(wk+1, &conv, 1);
	} else if (element->type == CAMQP_TYPE_SHORT) {
		len = 3;
		uint16_t conv1 = (uint16_t) twos_complement(element->data.i);
		uint16_t conv = htons(conv1);
		memcpy(wk, "\x61", 1);
		memcpy(wk+1, (void*) &conv, 2);
	} else if (element->type == CAMQP_TYPE_INT) {
		len = 5;
		uint32_t conv1 = (uint32_t) twos_complement(element->data.i);
		uint32_t conv = htonl(conv1);
		memcpy(wk, "\x71", 1);
		memcpy(wk+1, (void*) &conv, 4);
	} else if (element->type == CAMQP_TYPE_LONG) {
		len = 9;
		uint64_t conv1 = twos_complement(element->data.i);
		uint64_t conv = htonll(conv1);
		memcpy(wk, "\x81", 1);
		memcpy(wk+1, (void*) &conv, 8);
	} else if (element->type == CAMQP_TYPE_TIMESTAMP) {
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
void camqp_encode_primitive_float(camqp_primitive* element, camqp_data** buffer) {
	if (
		element->type != CAMQP_TYPE_FLOAT
			&&
		element->type != CAMQP_TYPE_DECIMAL32
	)
		return;

	// allocate working data
	camqp_byte* wk = camqp_util_new(5*sizeof(camqp_byte));
	if (!wk)
		return;

	uint32_t* conv1 = (uint32_t*) &element->data.f;
	uint32_t  conv2 = htonl(*conv1);

	if (element->type == CAMQP_TYPE_FLOAT)
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
void camqp_encode_primitive_double(camqp_primitive* element, camqp_data** buffer) {
	if (
		element->type != CAMQP_TYPE_DOUBLE
			&&
		element->type != CAMQP_TYPE_DECIMAL64
	)
		return;

	// allocate working data
	camqp_byte* wk = camqp_util_new(9*sizeof(camqp_byte));
	if (!wk)
		return;

	uint64_t* conv1 = (uint64_t*) &element->data.d;
	uint64_t  conv2 = htonll(*conv1);

	if (element->type == CAMQP_TYPE_DOUBLE)
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

void camqp_encode_primitive_uuid(camqp_primitive* element, camqp_data** buffer) {
	if (element->type != CAMQP_TYPE_UUID)
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

/// string
void camqp_encode_primitive_string(camqp_primitive* element, camqp_data** buffer) {
	if (
		element->type != CAMQP_TYPE_CHAR
			&&
		element->type != CAMQP_TYPE_STRING
			&&
		element->type != CAMQP_TYPE_SYMBOL
	)
		return;

	// fixed size
	if (element->type == CAMQP_TYPE_CHAR) {
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
		if (element->type == CAMQP_TYPE_STRING)
			code = 0xA1; // str8-utf8
		else
			code = 0xA3; // sym8

		size = 1+1+len;
	} else {
		if (element->type == CAMQP_TYPE_STRING)
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
void camqp_encode_primitive_binary(camqp_primitive* element, camqp_data** buffer) {
	if (element->type != CAMQP_TYPE_BINARY)
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

void camqp_encode_list(camqp_vector* set, camqp_data** buffer) {
	
}

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
		camqp_primitive* pt_key = camqp_primitive_string(set->base.context, CAMQP_TYPE_STRING, k->key);
		enc_keys[j] = camqp_element_encode((camqp_element*) pt_key);
		camqp_primitive_free(pt_key);

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

