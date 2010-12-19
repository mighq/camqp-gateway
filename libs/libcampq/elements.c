#include "camqp.h"
#include "internals.h"

#include <libxml/xmlstring.h> // xmlStrdup

/// camqp_element

/// new & free
void camqp_element_free(camqp_element* element) {
	if (element == NULL)
		return;

	if (element->class == CAMQP_CLASS_PRIMITIVE) {
		camqp_primitive_free((camqp_primitive*) element);
	} else if (element->class == CAMQP_CLASS_COMPOSITE) {
		camqp_composite_free((camqp_composite*) element, true);
	}
}
// ---

/// subclass detection
bool camqp_element_is_primitive(camqp_element* element) {
	return element->class == CAMQP_CLASS_PRIMITIVE;
}

bool camqp_element_is_null(camqp_element* element) {
	if (!camqp_element_is_primitive(element))
		return false;

	camqp_primitive* primitive = (camqp_primitive*) element;
	return primitive->type == CAMQP_TYPE_NULL;
}

bool camqp_element_is_scalar(camqp_element* element) {
	if (!camqp_element_is_primitive(element))
		return false;

	camqp_primitive* primitive = (camqp_primitive*) element;
	return primitive->multiple == CAMQP_MULTIPLICITY_SCALAR;
}

bool camqp_element_is_vector(camqp_element* element) {
	if (!camqp_element_is_primitive(element))
		return false;

	camqp_primitive* primitive = (camqp_primitive*) element;
	return primitive->multiple == CAMQP_MULTIPLICITY_VECTOR;
}

bool camqp_element_is_composite(camqp_element* element) {
	return element->class == CAMQP_CLASS_COMPOSITE;
}
// ---

/// type string representation
camqp_char* camqp_element_type_name(camqp_element* element) {
	if (element == NULL)
		return NULL;

	if (camqp_element_is_null(element))
		return xmlStrdup((xmlChar*) "null");

	if (camqp_element_is_primitive(element)) {
		camqp_primitive* p = (camqp_primitive*) element;
		switch (p->type) {
			case CAMQP_TYPE_BOOLEAN:
				return xmlStrdup((xmlChar*) "boolean");
			case CAMQP_TYPE_UBYTE:
				return xmlStrdup((xmlChar*) "ubyte");
			case CAMQP_TYPE_USHORT:
				return xmlStrdup((xmlChar*) "ushort");
			case CAMQP_TYPE_UINT:
				return xmlStrdup((xmlChar*) "uint");
			case CAMQP_TYPE_ULONG:
				return xmlStrdup((xmlChar*) "ulong");
			case CAMQP_TYPE_BYTE:
				return xmlStrdup((xmlChar*) "byte");
			case CAMQP_TYPE_SHORT:
				return xmlStrdup((xmlChar*) "short");
			case CAMQP_TYPE_INT:
				return xmlStrdup((xmlChar*) "int");
			case CAMQP_TYPE_LONG:
				return xmlStrdup((xmlChar*) "long");
			case CAMQP_TYPE_FLOAT:
				return xmlStrdup((xmlChar*) "float");
			case CAMQP_TYPE_DECIMAL32:
				return xmlStrdup((xmlChar*) "decimal32");
			case CAMQP_TYPE_DOUBLE:
				return xmlStrdup((xmlChar*) "double");
			case CAMQP_TYPE_DECIMAL64:
				return xmlStrdup((xmlChar*) "decimal64");
			case CAMQP_TYPE_CHAR:
				return xmlStrdup((xmlChar*) "char");
			case CAMQP_TYPE_TIMESTAMP:
				return xmlStrdup((xmlChar*) "timestamp");
			case CAMQP_TYPE_UUID:
				return xmlStrdup((xmlChar*) "uuid");
			case CAMQP_TYPE_STRING:
				return xmlStrdup((xmlChar*) "string");
			case CAMQP_TYPE_SYMBOL:
				return xmlStrdup((xmlChar*) "symbol");
			case CAMQP_TYPE_BINARY:
				return xmlStrdup((xmlChar*) "binary");
			case CAMQP_TYPE_LIST:
				return xmlStrdup((xmlChar*) "list");
			case CAMQP_TYPE_MAP:
				return xmlStrdup((xmlChar*) "map");

			default:
				return NULL;
		}
	} else if (camqp_element_is_composite(element)) {
		camqp_composite* c = (camqp_composite*) element;
		return xmlStrdup(c->name);
	}

	return NULL;
}
// ---

// ---

/// camqp_primitive

/// new & free
void camqp_primitive_free(camqp_primitive* element) {
	if (element == NULL)
		return;

	if (element->multiple == CAMQP_MULTIPLICITY_SCALAR) {
		camqp_scalar_free((camqp_scalar*) element);
	} else if (element->multiple == CAMQP_MULTIPLICITY_VECTOR) {
		camqp_vector_free((camqp_vector*) element, true);
	}
}
// ---

/// null
camqp_primitive* camqp_primitive_null(camqp_context* context) {
	camqp_primitive* ret = camqp_util_new(sizeof(camqp_primitive));
	if (!ret)
		return NULL;

	ret->base.context = context;
	ret->base.class = CAMQP_CLASS_PRIMITIVE;
	ret->multiple = CAMQP_MULTIPLICITY_SCALAR;
	ret->type = CAMQP_TYPE_NULL;

	return ret;
}
// ---

// ---
