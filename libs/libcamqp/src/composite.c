#include "camqp.h"
#include "internals.h"

#include <string.h> // memset, memcpy

#include <libxml/xmlstring.h> // xmlStrdup
#include <libxml/parser.h>

/// new & free
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

	// check for valid name & type
	const uint16_t max_expr_length = 128;
	camqp_char* expr = NULL;

	if (type_name != NULL && type_code == 0) {
		uint32_t str_len = (max_expr_length + (2*strlen((char*)type_name))) * sizeof(camqp_char);
		expr = camqp_util_new(str_len + 1);
		snprintf((char*)expr, str_len, "//amqp:type[@name='%s' and @class='composite']/amqp:descriptor[not(@name) or @name='%s']", type_name, type_name);
	}

	if (type_name == NULL && type_code != 0) {
		uint32_t str_len = (max_expr_length) * sizeof(camqp_char);
		expr = camqp_util_new(str_len + 1);
		snprintf((char*)expr, str_len, "//amqp:type[@class='composite']/amqp:descriptor[@code='0x%08X']", type_code);
	}

	if (type_name != NULL && type_code != 0) {
		uint32_t str_len = (max_expr_length + (2*strlen((char*)type_name))) * sizeof(camqp_char);
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

	// save type element pointer
	xmlNodePtr type_element = xp->nodesetval->nodeTab[0]->parent;

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

	comp->name = real_name;
	comp->code = real_code;

	comp->type_def = type_element;

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
// ---

/// camqp_composite_field_put
// TODO spolocny codebase s vektorom
bool camqp_composite_field_put(camqp_composite* element, const camqp_char* key, camqp_element* item) {
	// check if contexts are matching
	if (element->base.context != item->context)
		return false;

	// check if element with key can be in that composite according to XML definition
	{
		// expression
		const uint16_t max_expr_length = 128;
		camqp_char* expr = NULL;

		uint32_t str_len = (max_expr_length + strlen((char*) key)) * sizeof(camqp_char);
		expr = camqp_util_new(str_len + 1);
		snprintf((char*) expr, str_len, "./amqp:field[@name='%s']", key);

		// relative xpath query
		xmlNodePtr backup = element->base.context->xpath->node;
		element->base.context->xpath->node = element->type_def;
		xmlXPathObjectPtr xp = xmlXPathEvalExpression((xmlChar*) expr, element->base.context->xpath);
		element->base.context->xpath->node = backup;
		camqp_util_free(expr);

		// check that key is valid
		if (xp->nodesetval->nodeNr != 1) {
			xmlXPathFreeObject(xp);
			return false;
		}

		// save field element location
		xmlNodePtr field_elem = xp->nodesetval->nodeTab[0];
		xmlXPathFreeObject(xp);

		bool null_allowed;
		xmlChar* mandatory_str = xmlGetProp(field_elem, (xmlChar*) "mandatory");
		if (mandatory_str) {
			// mandatory?
			null_allowed = xmlStrcmp((xmlChar*) "true", mandatory_str) != 0;
			xmlFree(mandatory_str);
		} else {
			// mandatory not specified
			null_allowed = true;
		}

		// check if null against rules
		bool is_null = camqp_element_is_null(item);
		if (is_null) {
			// null

			// check if it can be null
			if (!null_allowed)
				return false;
		} else {
			// not null
			// check type of field
			xmlChar* type_str = xmlGetProp(field_elem, (xmlChar*) "type");
			if (!type_str)
				return false;

			// provided type
			camqp_char* used = camqp_element_type_name(item);

			// compare
			int cmp = xmlStrcmp(type_str, used);
			if (cmp != 0) {
				// invalid type used!
				xmlFree(type_str);
				camqp_util_free(used);

				return false;
			}

			xmlFree(type_str);
			camqp_util_free(used);
		}
	}

	// create new item
	camqp_vector_item* el = camqp_vector_item_new(key, item);
	if (!el)
		return false;

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
		return true;
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
				return true;
			}

			// keys cannot be duplicate!
			if (cmp1 == 0) {
				camqp_vector_item_free(el, false);
				return false;
			}
		}

		// after exists?
		if (after == NULL) {
			// at to the end
			before->next = el;
			return true;
		}

		// compare with after
		int cmp2 = xmlStrcmp(key, after->key);

		// don't allow duplicate keys
		if (cmp2 == 0) {
			camqp_vector_item_free(el, false);
			return false;
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
			return true;
		}
	} while (true);
}
// ---

/// camqp_composite_field_get
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

