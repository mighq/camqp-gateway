#include "camqp.h"
#include "internals.h"

#include <string.h> // memset, memcpy

/// camqp_data

camqp_data camqp_data_static(const camqp_byte* data, camqp_size size) {
	camqp_data ret;
	ret.bytes = (camqp_byte*) data;
	ret.size = size;
	return ret;
}

// TODO: ak je bytes NULL, tak len naalokuj size, pouzit v encodingu potom
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

