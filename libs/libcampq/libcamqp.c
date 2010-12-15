#include <camqp.h>

#include <string.h> // memset, memcpy

#include <libxml/xmlstring.h>
#include <libxml/parser.h>
#include <libxml/xinclude.h>
#include <libxml/xpathInternals.h>

// TODO nejak poriesit exception navratove hodnoty?
// TODO kontrolovanie utf8 stringov
// TODO hashova reprezentacia vektorov

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
// ---

/// camqp_data
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

// TODO
void camqp_element_free(camqp_element* element) {
	if (element == NULL)
		return;

	if (element->class == CAMQP_CLASS_PRIMITIVE)
		camqp_primitive_free((camqp_primitive*) element);
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
			element->type == CAMQP_TYPE_UUID
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
			&&
		type != CAMQP_TYPE_UUID
	)
		return NULL;

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
			&&
		element->type != CAMQP_TYPE_UUID
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

		sscanf(code_str, "0x%08X", &real_code);
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

