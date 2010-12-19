#include "camqp.h"
#include "internals.h"

#include <string.h> // memset, memcpy

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

uint64_t twos_complement(int64_t nr) {
	// positive
	if (nr >= 0)
		return nr;

	// negative
	uint64_t nu = llabs(nr) ^ 0xFFFFFFFFFFFFFFFF;
	return nu + 0x01;
}
// ---

