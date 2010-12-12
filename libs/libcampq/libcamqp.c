#include <camqp.h>

#include <stdlib.h>
#include <string.h>

#include <libxml/parser.h>
#include <libxml/xinclude.h>

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
	if (data != NULL)
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
	camqp_util_free(binary->bytes);
	camqp_util_free(binary);
}
// ---

/// camqp_string
camqp_string* camqp_string_new(const camqp_char* string, camqp_size length) {
	camqp_string* ret;

	// allocate return structure
	ret = camqp_util_new(sizeof(camqp_string));
	if (!ret)
		return NULL;

	// duplicate data
	ret->chars = camqp_util_new(length);
	if (!ret->chars) {
		camqp_util_free(ret);
		return NULL;
	}
	memcpy(ret->chars, string, length);
	ret->length = length;

	return ret;
}

camqp_string* camqp_string_duplicate(const camqp_string* original) {
	return camqp_string_new(original->chars, original->length);
}

void camqp_string_free(camqp_string* string) {
	camqp_util_free(string->chars);
	camqp_util_free(string);
}

char* camqp_string_cstr(const camqp_string* original) {
	char* ret;

	ret = camqp_util_new(original->length + 1);
	if (!ret)
		return NULL;

	memcpy(ret, original->chars, original->length);

	return ret;
}
// ---

/// camqp_context
/**
 * TODO: filename is not UTF-8 ready!
 */
camqp_context* camqp_context_new(camqp_string* protocol, camqp_string* definition) {
	// allocate result structure
	camqp_context* ctx = camqp_util_new(sizeof(camqp_context));
	if (!ctx)
		return NULL;

	// save protocol file and name
	ctx->protocol = camqp_string_duplicate(protocol);
	ctx->definition = camqp_string_duplicate(definition);

	// init libxml
	xmlInitParser();
	LIBXML_TEST_VERSION

	// load XML document
	char* filename = camqp_string_cstr(definition);
	ctx->xml = xmlParseFile(filename);
	camqp_util_free(filename);

	if (ctx->xml == NULL) {
		fprintf(stderr, "Unable to parse file\n");
		xmlCleanupParser();
		camqp_context_free(ctx);
		return NULL;
	}

	if (xmlXIncludeProcess(ctx->xml) <= 0) {
		fprintf(stderr, "XInclude processing failed\n");
		xmlCleanupParser();
		camqp_context_free(ctx);
		return NULL;
	}

	// shutdown libxml
	xmlCleanupParser();

	// now we have parsed XML definition

	// check for correct protocol definition
	xmlNodePtr root = xmlDocGetRootElement(ctx->xml);
	xmlChar* protocol_def = xmlGetProp(root, (xmlChar*) "name");
	xmlChar* protocol_req = xmlUTF8Strndup(protocol->chars, protocol->length);

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
	// free XML document
	xmlFreeDoc(context->xml);

	// free string members
	camqp_string_free(context->protocol);
	camqp_string_free(context->definition);

	// free structure
	camqp_util_free(context);
}
// ---

/// camqp_element
/*
camqp_element* camqp_element_new(camqp_context* ctx, camqp_class cls, camqp_multiplicity multi) {
	camqp_element* ret = camqp_util_new(sizeof(camqp_element));
	if (!ret)
		return NULL;

	ret->context = ctx;
	ret->class = cls;
	ret->multiple = multi;

	return ret;
}

void camqp_element_free(camqp_element* element) {
	camqp_util_free(element);
}
*/
// ---

