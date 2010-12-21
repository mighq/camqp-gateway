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

uint64_t totc(int64_t nr) {
	// positive
	if (nr >= 0)
		return nr;

	// negative
	uint64_t nu = llabs(nr) ^ 0xFFFFFFFFFFFFFFFF;
	return nu + 0x01;
}

int8_t fromtc8(uint8_t nr) {
	int8_t ret;

	bool negative = (nr & 0x80) != 0;
	if (negative) {
		uint8_t wk = nr ^ 0xFF;
		wk++;
		ret = -wk;
	} else {
		// positive
		ret = (int8_t) nr;
	}

	return ret;
}

int16_t fromtc16(uint16_t nr) {
	int16_t ret;

	bool negative = (nr & 0x8000) != 0;
	if (negative) {
		uint16_t wk = nr ^ 0xFFFF;
		wk++;
		ret = -wk;
	} else {
		// positive
		ret = (int16_t) nr;
	}

	return ret;
}

int32_t fromtc32(uint32_t nr) {
	int32_t ret;

	bool negative = (nr & 0x80000000) != 0;
	if (negative) {
		uint32_t wk = nr ^ 0xFFFFFFFF;
		wk++;
		ret = -wk;
	} else {
		// positive
		ret = (int32_t) nr;
	}

	return ret;
}

int64_t fromtc64(uint64_t nr) {
	int64_t ret;

	bool negative = (nr & 0x8000000000000000) != 0;
	if (negative) {
		uint64_t wk = nr ^ 0xFFFFFFFFFFFFFFFF;
		wk++;
		ret = -wk;
	} else {
		// positive
		ret = (int64_t) nr;
	}

	return ret;
}

// ---

