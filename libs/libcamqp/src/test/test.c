#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <camqp.h>

#define err(txt) { fprintf(stderr, (txt)); fprintf(stderr, "\n"); return EXIT_FAILURE; }

char*				dump_data(unsigned char* pointer, unsigned int length);
camqp_char*			camqp_data_dump(camqp_data* data);

int main(int argc, char* argv[]) {
	/// camqp_data
	camqp_data* dt1 = camqp_data_new((camqp_byte*)"ABCDEFGH", 8);
	if (!dt1)
		err("T001");
	camqp_data_free(dt1);
	// ---

	/// camqp_context
	camqp_char* file = (camqp_char*) "messaging-v1.0.xml";
	camqp_char* proto = (camqp_char*) "messaging-v1.0";

	// create context
	camqp_context* ctx1 = camqp_context_new(proto, file);

	if (!ctx1)
		err("T003");

	// ---

	/// camqp_primitive

	// boolean
	camqp_scalar* pt1 = camqp_scalar_bool(ctx1, true);
	bool vl1 = camqp_value_bool((camqp_element*) pt1);
	printf("boolean: %d\n", vl1);
	camqp_element_free((camqp_element*) pt1);

	// int
	camqp_scalar* pt2 = camqp_scalar_int(ctx1, CAMQP_TYPE_BYTE, -36);
	int vl2 = camqp_value_int((camqp_element*) pt2);
	printf("int: %d\n", vl2);
	camqp_element_free((camqp_element*) pt2);

	// uint
	camqp_scalar* pt3 = camqp_scalar_uint(ctx1, CAMQP_TYPE_UINT, 126);
	int vl3 = camqp_value_uint((camqp_element*) pt3);
	printf("uint: %d\n", vl3);
	camqp_element_free((camqp_element*) pt3);

	// float
	camqp_scalar* pt4 = camqp_scalar_float(ctx1, CAMQP_TYPE_FLOAT, 3.14);
	float vl4 = camqp_value_float((camqp_element*) pt4);
	printf("float: %f\n", vl4);
	camqp_element_free((camqp_element*) pt4);

	// double
	camqp_scalar* pt5 = camqp_scalar_double(ctx1, CAMQP_TYPE_DOUBLE, 198.32165489);
	double vl5 = camqp_value_double((camqp_element*) pt5);
	printf("double: %0.8f\n", vl5);
	camqp_element_free((camqp_element*) pt5);

	// string
	camqp_scalar* pt6 = camqp_scalar_string(ctx1, CAMQP_TYPE_STRING, (camqp_char*) "pela hopa");
	const camqp_char* vl6 = camqp_value_string((camqp_element*) pt6);
	printf("%s\n", vl6);
	camqp_element_free((camqp_element*) pt6);

	// binary
	camqp_data* dt2 = camqp_data_new((camqp_byte*) "XYZ", 3);
	if (!dt2)
		err("T004");

	camqp_scalar* pt7 = camqp_scalar_binary(ctx1, dt2);
	camqp_data* vl7 = camqp_value_binary((camqp_element*) pt7);
	printf("%.3s:%d\n", vl7->bytes, vl7->size);
	camqp_element_free((camqp_element*) pt7);

	camqp_data_free(dt2);

	// null
	camqp_element* tp_n = (camqp_element*) camqp_primitive_null(ctx1);
	printf("is null: %d\n", camqp_element_is_null(tp_n));
	camqp_element_free(tp_n);

	// uuid
	camqp_scalar* tp_id = camqp_scalar_uuid(ctx1);
	{
		const camqp_uuid* uid2 = camqp_value_uuid((camqp_element*) tp_id);
		char* wk = dump_data((void*) uid2, 16);
		printf("uuid: %s\n", wk);
		free(wk);
	}

	camqp_element_free((camqp_element*) tp_id);
	// ---

	/// camqp_vector
	camqp_scalar* pt8  = camqp_scalar_uint(ctx1, CAMQP_TYPE_UINT, 418);
	camqp_scalar* pt9  = camqp_scalar_uint(ctx1, CAMQP_TYPE_UINT, 325);
	camqp_scalar* pt10 = camqp_scalar_uint(ctx1, CAMQP_TYPE_UINT, 65);
	camqp_scalar* pt11 = camqp_scalar_uint(ctx1, CAMQP_TYPE_UINT, 123);
	camqp_scalar* pt12 = camqp_scalar_uint(ctx1, CAMQP_TYPE_UINT, 456);

	camqp_vector* vec1 = camqp_vector_new(ctx1);
	camqp_vector_item_put(vec1, (camqp_char*) "key1", (camqp_element*) pt8);
	camqp_vector_item_put(vec1, (camqp_char*) "key0", (camqp_element*) pt9);
	camqp_vector_item_put(vec1, (camqp_char*) "key5", (camqp_element*) pt10);
	camqp_vector_item_put(vec1, (camqp_char*) "key3", (camqp_element*) pt11);
	camqp_vector_item_put(vec1, (camqp_char*) "key3", (camqp_element*) pt12);
	camqp_element_free((camqp_element*) pt12);

	{
		camqp_vector_item* to_del = vec1->data;
		while (to_del) {
			uint64_t x = camqp_value_uint((camqp_element*) to_del->value);
			printf("%s:%d\n", to_del->key, (int) x);
			to_del = to_del->next;
		}
	}

	{
		camqp_element* el1 = camqp_vector_item_get(vec1, (camqp_char*) "key3");
		uint64_t y = camqp_value_uint((camqp_element*) el1);
		printf("item3: 123=%d?\n", (int)y);
	}

	camqp_element_free((camqp_element*) vec1);
	// ---

	/// camqp_composite
	camqp_composite* comp1 = camqp_composite_new(ctx1, (camqp_char*) "response", 2);
	if (!comp1) {
		camqp_context_free(ctx1);
		err("T008");
	}

	camqp_scalar* pt13 = camqp_scalar_uint(ctx1, CAMQP_TYPE_UINT, 325);
	if (camqp_composite_field_put(comp1, (camqp_char*) "cislo", (camqp_element*) pt13))
	{
		camqp_element* el2 = camqp_composite_field_get(comp1, (camqp_char*) "cislo");
		if (el2) {
			uint64_t y = camqp_value_uint((camqp_element*) el2);
			printf("field 'cislo': 325=%d?\n", (int) y);
		}
	} else {
		camqp_element_free((camqp_element*) pt13);
	}

	camqp_element_free((camqp_element*) comp1);
	// ---

	/// encoding

	// null
	{
		camqp_primitive* pt = camqp_primitive_null(ctx1);

		camqp_data* enc = camqp_element_encode((camqp_element*) pt);
		camqp_char* dump = camqp_data_dump(enc);

		printf("%s\n", dump);

		camqp_util_free(dump);
		camqp_data_free(enc);
		camqp_element_free((camqp_element*) pt);
	}

	// bool
	{
		camqp_scalar* pt = camqp_scalar_bool(ctx1, false);

		camqp_data* enc = camqp_element_encode((camqp_element*) pt);
		camqp_char* dump = camqp_data_dump(enc);

		printf("%s\n", dump);

		camqp_util_free(dump);
		camqp_data_free(enc);
		camqp_element_free((camqp_element*) pt);
	}

	// ubyte
	{
		camqp_scalar* pt = camqp_scalar_uint(ctx1, CAMQP_TYPE_UBYTE, 45);

		camqp_data* enc = camqp_element_encode((camqp_element*) pt);
		camqp_char* dump = camqp_data_dump(enc);

		printf("%s\n", dump);

		camqp_util_free(dump);
		camqp_data_free(enc);
		camqp_element_free((camqp_element*) pt);
	}

	// ushort
	{
		camqp_scalar* pt = camqp_scalar_uint(ctx1, CAMQP_TYPE_USHORT, 612);

		camqp_data* enc = camqp_element_encode((camqp_element*) pt);
		camqp_char* dump = camqp_data_dump(enc);

		printf("%s\n", dump);

		camqp_util_free(dump);
		camqp_data_free(enc);
		camqp_element_free((camqp_element*) pt);
	}

	// uint
	{
		camqp_scalar* pt = camqp_scalar_uint(ctx1, CAMQP_TYPE_UINT, 76543);

		camqp_data* enc = camqp_element_encode((camqp_element*) pt);
		camqp_char* dump = camqp_data_dump(enc);

		printf("%s\n", dump);

		camqp_util_free(dump);
		camqp_data_free(enc);
		camqp_element_free((camqp_element*) pt);
	}

	// ulong
	{
		camqp_element* pt = (camqp_element*) camqp_scalar_uint(ctx1, CAMQP_TYPE_ULONG, 9294967297);

		camqp_data* enc = camqp_element_encode(pt);
		camqp_char* dump = camqp_data_dump(enc);

		printf("%s\n", dump);

		camqp_util_free(dump);
		camqp_data_free(enc);
		camqp_element_free(pt);
	}

	// byte
	{
		camqp_element* pt = (camqp_element*) camqp_scalar_int(ctx1, CAMQP_TYPE_BYTE, -45);

		camqp_data* enc = camqp_element_encode((camqp_element*) pt);
		camqp_char* dump = camqp_data_dump(enc);

		printf("%s\n", dump);

		camqp_util_free(dump);
		camqp_data_free(enc);
		camqp_element_free((camqp_element*) pt);
	}

	// short
	{
		camqp_element* pt = (camqp_element*) camqp_scalar_int(ctx1, CAMQP_TYPE_SHORT, -1367);

		camqp_data* enc = camqp_element_encode((camqp_element*) pt);
		camqp_char* dump = camqp_data_dump(enc);

		printf("%s\n", dump);

		camqp_util_free(dump);
		camqp_data_free(enc);
		camqp_element_free((camqp_element*) pt);
	}

	// int
	{
		camqp_element* pt = (camqp_element*) camqp_scalar_int(ctx1, CAMQP_TYPE_INT, -46377105);

		camqp_data* enc = camqp_element_encode((camqp_element*) pt);
		camqp_char* dump = camqp_data_dump(enc);

		printf("%s\n", dump);

		camqp_util_free(dump);
		camqp_data_free(enc);
		camqp_element_free((camqp_element*) pt);
	}

	// long
	{
		camqp_element* pt = (camqp_element*) camqp_scalar_int(ctx1, CAMQP_TYPE_LONG, 80181598485);

		camqp_data* enc = camqp_element_encode((camqp_element*) pt);
		camqp_char* dump = camqp_data_dump(enc);

		printf("%s\n", dump);

		camqp_util_free(dump);
		camqp_data_free(enc);
		camqp_element_free((camqp_element*) pt);
	}

	// float
	{
		camqp_element* pt = (camqp_element*) camqp_scalar_float(ctx1, CAMQP_TYPE_FLOAT, 3.14);

		camqp_data* enc = camqp_element_encode((camqp_element*) pt);
		camqp_char* dump = camqp_data_dump(enc);

		printf("%s\n", dump);

		camqp_util_free(dump);
		camqp_data_free(enc);
		camqp_element_free((camqp_element*) pt);
	}

	// double
	{
		camqp_element* pt = (camqp_element*) camqp_scalar_double(ctx1, CAMQP_TYPE_DOUBLE, 3.1415);

		camqp_data* enc = camqp_element_encode((camqp_element*) pt);
		camqp_char* dump = camqp_data_dump(enc);

		printf("%s\n", dump);

		camqp_util_free(dump);
		camqp_data_free(enc);
		camqp_element_free((camqp_element*) pt);
	}

	// uuid
	{
		camqp_element* pt = (camqp_element*) camqp_scalar_uuid(ctx1);

		camqp_data* enc = camqp_element_encode((camqp_element*) pt);
		camqp_char* dump = camqp_data_dump(enc);

		{
			const camqp_uuid* uid = camqp_value_uuid(pt);
			char* wk = dump_data((void*) uid, 16);
			printf("%s\n", wk);
			free(wk);
		}

		printf("%s\n", dump);

		camqp_util_free(dump);
		camqp_data_free(enc);
		camqp_element_free((camqp_element*) pt);
	}

	// char
	{
		camqp_element* pt = (camqp_element*) camqp_scalar_string(ctx1, CAMQP_TYPE_CHAR, (camqp_char*) "Z");

		camqp_data* enc = camqp_element_encode((camqp_element*) pt);
		camqp_char* dump = camqp_data_dump(enc);

		printf("%s\n", dump);

		camqp_util_free(dump);
		camqp_data_free(enc);
		camqp_element_free((camqp_element*) pt);
	}

	// string
	{
		camqp_scalar* pt = camqp_scalar_string(ctx1, CAMQP_TYPE_SYMBOL, (camqp_char*) "AJS da peli peli!");

		camqp_data* enc = camqp_element_encode((camqp_element*) pt);
		camqp_char* dump = camqp_data_dump(enc);

		printf("%s\n", dump);

		camqp_util_free(dump);
		camqp_data_free(enc);
		camqp_element_free((camqp_element*) pt);
	}

	// binary
	{
		camqp_data dt = camqp_data_static((camqp_byte*) "\x00_ABCD_\x00", 8);
		camqp_scalar* pt = camqp_scalar_binary(ctx1, &dt);

		camqp_data* enc = camqp_element_encode((camqp_element*) pt);
		camqp_char* dump = camqp_data_dump(enc);

		printf("%s\n", dump);

		camqp_util_free(dump);
		camqp_data_free(enc);
		camqp_element_free((camqp_element*) pt);
	}

	// map
	{
		camqp_scalar* pt1 = camqp_scalar_uint(ctx1, CAMQP_TYPE_UINT, 418);
		camqp_scalar* pt2 = camqp_scalar_uint(ctx1, CAMQP_TYPE_UINT, 325);
		camqp_scalar* pt3 = camqp_scalar_uint(ctx1, CAMQP_TYPE_UINT, 65);

		camqp_vector* vec = camqp_vector_new(ctx1);
		camqp_vector_item_put(vec, (camqp_char*) "item_1", (camqp_element*) pt1);
		camqp_vector_item_put(vec, (camqp_char*) "item_2", (camqp_element*) pt2);
		camqp_vector_item_put(vec, (camqp_char*) "item_3", (camqp_element*) pt3);

		camqp_data* enc = camqp_element_encode((camqp_element*) vec);
		if (enc) {
			camqp_char* dump = camqp_data_dump(enc);
			printf("%s\n", dump);
			camqp_util_free(dump);

			camqp_data_free(enc);
		} else {
			puts("ERROR!");
		}

		camqp_element_free((camqp_element*) vec);
	}

	// list
	{
		camqp_scalar* pt1 = camqp_scalar_uint(ctx1, CAMQP_TYPE_UINT, 418);
		camqp_scalar* pt2 = camqp_scalar_uint(ctx1, CAMQP_TYPE_UINT, 325);
		camqp_scalar* pt3 = camqp_scalar_uint(ctx1, CAMQP_TYPE_UINT, 65);

		camqp_vector* vec = camqp_vector_new(ctx1);
		camqp_vector_item_put(vec, (camqp_char*) "0", (camqp_element*) pt1);
		camqp_vector_item_put(vec, (camqp_char*) "1", (camqp_element*) pt2);
		camqp_vector_item_put(vec, (camqp_char*) "2", (camqp_element*) pt3);

		camqp_data* enc = camqp_element_encode((camqp_element*) vec);
		if (enc) {
			camqp_char* dump = camqp_data_dump(enc);
			printf("%s\n", dump);
			camqp_util_free(dump);

			camqp_data_free(enc);
		} else {
			puts("ERROR!");
		}

		camqp_element_free((camqp_element*) vec);
	}

	// composite
	{
		camqp_composite* comp = camqp_composite_new(ctx1, (camqp_char*) "response", 2);
		if (comp) {
			camqp_composite_field_put(
				comp,
				(camqp_char*) "id",
				(camqp_element*) camqp_scalar_uuid(ctx1)
			);
			camqp_composite_field_put(
				comp,
				(camqp_char*) "correlation",
				(camqp_element*) camqp_scalar_uuid(ctx1)
			);
			camqp_data dt = camqp_data_static((camqp_byte*) "XYZ", 3);
			camqp_composite_field_put(
				comp,
				(camqp_char*) "content",
				(camqp_element*) camqp_scalar_binary(ctx1, &dt)
			);

			camqp_data* enc = camqp_element_encode((camqp_element*) comp);
			if (enc) {
				camqp_char* dump = camqp_data_dump(enc);
				printf("%s\n", dump);
				camqp_util_free(dump);
				camqp_data_free(enc);
			} else {
				puts("ERROR encoding!");
			}

			camqp_element_free((camqp_element*) comp);
		}
	}

	// ---

	/// decoding
	{
		/// composite
		camqp_data left;
		camqp_data bin_data = camqp_data_static((const camqp_byte*) "\x00\x70\x00\x00\x00\x02\xC0\x29\x04\x98\x9E\x2F\x77\xBF\x07\x83\x47\x42\xAC\x96\x9E\xFD\x0C\x93\xFD\x47\x98\xCA\x3D\x6D\xA7\x0A\x6E\x4E\x10\xB5\xA8\xA0\xB6\xE4\x7D\xA3\x15\x40\xA0\x03\x58\x59\x5A", 49);

		{
			camqp_char* dump = camqp_data_dump(&bin_data);
			printf("TODO: %s\n", dump);
			camqp_util_free(dump);
		}

		camqp_element* result = camqp_element_decode(ctx1, &bin_data, &left);
		if (result) {
			// check that nothing left
			if (left.bytes != NULL || left.size != 0) {
				puts("no everything decoded!");
				camqp_char* dump = camqp_data_dump(&left);
				printf("LEFT: %s\n", dump);
				camqp_util_free(dump);
			}

			// encode again
			camqp_data* enc = camqp_element_encode((camqp_element*) result);
			if (enc) {
				camqp_char* dump = camqp_data_dump(enc);
				printf("ENCODED AGAIN:%s\n", dump);
				camqp_util_free(dump);
				camqp_data_free(enc);
			} else {
				puts("ERROR encoding!");
			}

			camqp_element_free(result);
		} else {
			puts("ERROR!");
		}
		// ---
	}

	{
		/// scalar
		camqp_data left;
		camqp_data bin_data = camqp_data_static((const camqp_byte*) "\x82\x40\x09\x21\xCA\xC0\x83\x12\x6F", 9);

		{
			camqp_char* dump = camqp_data_dump(&bin_data);
			printf("TODO: %s\n", dump);
			camqp_util_free(dump);
		}

		camqp_element* result = camqp_element_decode(ctx1, &bin_data, &left);
		if (result) {
			printf("L:%f\n", (float) camqp_value_double(result));

			// check that nothing left
			if (left.bytes != NULL || left.size != 0) {
				puts("no everything decoded!");
				camqp_char* dump = camqp_data_dump(&left);
				printf("LEFT: %s\n", dump);
				camqp_util_free(dump);
			}

			// encode again
			camqp_data* enc = camqp_element_encode((camqp_element*) result);
			if (enc) {
				camqp_char* dump = camqp_data_dump(enc);
				printf("ENCODED AGAIN: %s\n", dump);
				camqp_util_free(dump);
				camqp_data_free(enc);
			} else {
				puts("ERROR encoding!");
			}

			camqp_element_free(result);
		} else {
			puts("ERROR!");
		}
		// ---
	}

	{
		/// vector
		camqp_data left;
		camqp_data bin_data = camqp_data_static((const camqp_byte*) "\xC1\x28\x06\xA1\x06\x69\x74\x65\x6D\x5F\x31\x70\x00\x00\x01\xA2\xA1\x06\x69\x74\x65\x6D\x5F\x32\x70\x00\x00\x01\x45\xA1\x06\x69\x74\x65\x6D\x5F\x33\x70\x00\x00\x00\x41", 42);

		{
			camqp_char* dump = camqp_data_dump(&bin_data);
			printf("TODO: %s\n", dump);
			camqp_util_free(dump);
		}

		camqp_element* result = camqp_element_decode(ctx1, &bin_data, &left);
		if (result) {
			// check that nothing left
			if (left.bytes != NULL || left.size != 0) {
				puts("no everything decoded!");
				camqp_char* dump = camqp_data_dump(&left);
				printf("LEFT: %s\n", dump);
				camqp_util_free(dump);
			}

			// encode again
			camqp_data* enc = camqp_element_encode((camqp_element*) result);
			if (enc) {
				camqp_char* dump = camqp_data_dump(enc);
				printf("ENCODED AGAIN: %s\n", dump);
				camqp_util_free(dump);
				camqp_data_free(enc);
			} else {
				puts("ERROR encoding!");
			}

			camqp_element_free(result);
		} else {
			puts("ERROR!");
		}
		// ---
	}
	// ---

	camqp_context_free(ctx1);

	return EXIT_SUCCESS;
}
