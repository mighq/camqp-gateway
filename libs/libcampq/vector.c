#include "camqp.h"
#include "internals.h"

/// new & free
camqp_vector* camqp_vector_new(camqp_context* ctx) {
	camqp_vector* vec = camqp_util_new(sizeof(camqp_vector));
	if (!vec)
		return NULL;

	vec->base.base.context = ctx;
	vec->base.base.class = CAMQP_CLASS_PRIMITIVE;

	vec->base.multiple = CAMQP_MULTIPLICITY_VECTOR;
	vec->base.type = CAMQP_TYPE_LIST; // will be changed to map if any non-numeric-key items are added

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
// ---

/// vector items
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
// ---

/// camqp_vector_item_put
void camqp_vector_item_put(camqp_vector* vector, const camqp_char* key, camqp_element* element) {
	// check if contexts are matching
	if (vector->base.base.context != element->context)
		return;

	// check that we are using correct AMQP_TYPE
	if (vector->base.type == CAMQP_TYPE_LIST) {
		// switch to CAMQP_TYPE_MAP for any non-numeric key
		if (!camqp_is_numeric(key)) {
			vector->base.type = CAMQP_TYPE_MAP;
		}
	}

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
// ---

/// camqp_vector_item_get
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

