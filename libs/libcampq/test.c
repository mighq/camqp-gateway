#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <camqp.h>

#define err(txt) { fprintf(stderr, (txt)); fprintf(stderr, "\n"); return EXIT_FAILURE; }

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
	camqp_primitive* pt1 = camqp_primitive_bool(ctx1, true);
	bool vl1 = camqp_value_bool(pt1);
	printf("boolean: %d\n", vl1);
	camqp_primitive_free(pt1);

	// int
	camqp_primitive* pt2 = camqp_primitive_int(ctx1, CAMQP_TYPE_BYTE, -36);
	int vl2 = camqp_value_int(pt2);
	printf("int: %d\n", vl2);
	camqp_primitive_free(pt2);

	// uint
	camqp_primitive* pt3 = camqp_primitive_uint(ctx1, CAMQP_TYPE_UINT, 126);
	int vl3 = camqp_value_uint(pt3);
	printf("uint: %d\n", vl3);
	camqp_primitive_free(pt3);

	// float
	camqp_primitive* pt4 = camqp_primitive_float(ctx1, CAMQP_TYPE_FLOAT, 3.14);
	float vl4 = camqp_value_float(pt4);
	printf("float: %f\n", vl4);
	camqp_primitive_free(pt4);

	// double
	camqp_primitive* pt5 = camqp_primitive_double(ctx1, CAMQP_TYPE_DOUBLE, 198.32165489);
	double vl5 = camqp_value_double(pt5);
	printf("double: %0.8f\n", vl5);
	camqp_primitive_free(pt5);

	// string
	camqp_primitive* pt6 = camqp_primitive_string(ctx1, CAMQP_TYPE_STRING, (camqp_char*) "pela hopa");
	const camqp_char* vl6 = camqp_value_string(pt6);
	printf("%s\n", vl6);
	camqp_primitive_free(pt6);

	// binary
	camqp_data* dt2 = camqp_data_new((camqp_byte*) "XYZ", 3);
	if (!dt2)
		err("T004");

	camqp_primitive* pt7 = camqp_primitive_binary(ctx1, dt2);
	camqp_data* vl7 = camqp_value_binary(pt7);
	printf("%.3s:%d\n", vl7->bytes, vl7->size);
	camqp_primitive_free(pt7);

	camqp_data_free(dt2);

	// null
	camqp_element* tp_n = (camqp_element*) camqp_primitive_null(ctx1);
	printf("is null: %d\n", camqp_element_is_null(tp_n));
	camqp_element_free(tp_n);

	// uuid
	camqp_primitive* tp_id = camqp_primitive_uuid(ctx1);
	{
		const camqp_uuid* uid2 = camqp_value_uuid(tp_id);
		char* wk = dump_data((void*) uid2, 16);
		printf("uuid: %s\n", wk);
		free(wk);
	}

	camqp_primitive_free(tp_id);
	// ---

	/// camqp_vector
	camqp_primitive* pt8 = camqp_primitive_uint(ctx1, CAMQP_TYPE_UINT, 418);
	camqp_primitive* pt9 = camqp_primitive_uint(ctx1, CAMQP_TYPE_UINT, 325);
	camqp_primitive* pt10 = camqp_primitive_uint(ctx1, CAMQP_TYPE_UINT, 65);
	camqp_primitive* pt11 = camqp_primitive_uint(ctx1, CAMQP_TYPE_UINT, 123);
	camqp_primitive* pt12 = camqp_primitive_uint(ctx1, CAMQP_TYPE_UINT, 456);

	camqp_vector* vec1 = camqp_vector_new(ctx1);
	camqp_vector_item_put(vec1, (camqp_char*) "key1", (camqp_element*) pt8);
	camqp_vector_item_put(vec1, (camqp_char*) "key0", (camqp_element*) pt9);
	camqp_vector_item_put(vec1, (camqp_char*) "key5", (camqp_element*) pt10);
	camqp_vector_item_put(vec1, (camqp_char*) "key3", (camqp_element*) pt11);
	camqp_vector_item_put(vec1, (camqp_char*) "key3", (camqp_element*) pt12);
	camqp_primitive_free(pt12);

	{
		camqp_vector_item* to_del = vec1->data;
		while (to_del) {
			uint64_t x = camqp_value_uint((camqp_primitive*) to_del->value);
			printf("%s:%d\n", to_del->key, (int) x);
			to_del = to_del->next;
		}
	}

	{
		camqp_element* el1 = camqp_vector_item_get(vec1, (camqp_char*) "key3");
		uint64_t y = camqp_value_uint((camqp_primitive*) el1);
		printf("item3: 123=%d?\n", (int)y);
	}

	camqp_vector_free(vec1, true);
	// ---

	/// camqp_composite
	camqp_composite* comp1 = camqp_composite_new(ctx1, (camqp_char*) "response", 2);
	if (!comp1) {
		camqp_context_free(ctx1);
		err("T008");
	}

	camqp_primitive* pt13 = camqp_primitive_uint(ctx1, CAMQP_TYPE_UINT, 325);
	camqp_composite_field_put(comp1, (camqp_char*) "cislo", (camqp_element*) pt13);

	{
		camqp_element* el2 = camqp_composite_field_get(comp1, (camqp_char*) "cislo");
		uint64_t y = camqp_value_uint((camqp_primitive*) el2);
		printf("field 'cislo': 325=%d?\n", (int) y);
	}

	camqp_composite_free(comp1, true);
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
		camqp_primitive* pt = camqp_primitive_bool(ctx1, false);

		camqp_data* enc = camqp_element_encode((camqp_element*) pt);
		camqp_char* dump = camqp_data_dump(enc);

		printf("%s\n", dump);

		camqp_util_free(dump);
		camqp_data_free(enc);
		camqp_element_free((camqp_element*) pt);
	}

	// ubyte
	{
		camqp_primitive* pt = camqp_primitive_uint(ctx1, CAMQP_TYPE_UBYTE, 45);

		camqp_data* enc = camqp_element_encode((camqp_element*) pt);
		camqp_char* dump = camqp_data_dump(enc);

		printf("%s\n", dump);

		camqp_util_free(dump);
		camqp_data_free(enc);
		camqp_element_free((camqp_element*) pt);
	}

	// ushort
	{
		camqp_primitive* pt = camqp_primitive_uint(ctx1, CAMQP_TYPE_USHORT, 612);

		camqp_data* enc = camqp_element_encode((camqp_element*) pt);
		camqp_char* dump = camqp_data_dump(enc);

		printf("%s\n", dump);

		camqp_util_free(dump);
		camqp_data_free(enc);
		camqp_element_free((camqp_element*) pt);
	}

	// uint
	{
		camqp_primitive* pt = camqp_primitive_uint(ctx1, CAMQP_TYPE_UINT, 76543);

		camqp_data* enc = camqp_element_encode((camqp_element*) pt);
		camqp_char* dump = camqp_data_dump(enc);

		printf("%s\n", dump);

		camqp_util_free(dump);
		camqp_data_free(enc);
		camqp_element_free((camqp_element*) pt);
	}

	// ulong
	{
		camqp_primitive* pt = camqp_primitive_uint(ctx1, CAMQP_TYPE_ULONG, 9294967297);

		camqp_data* enc = camqp_element_encode((camqp_element*) pt);
		camqp_char* dump = camqp_data_dump(enc);

		printf("%s\n", dump);

		camqp_util_free(dump);
		camqp_data_free(enc);
		camqp_element_free((camqp_element*) pt);
	}

	// byte
	{
		camqp_primitive* pt = camqp_primitive_int(ctx1, CAMQP_TYPE_BYTE, 45);

		camqp_data* enc = camqp_element_encode((camqp_element*) pt);
		camqp_char* dump = camqp_data_dump(enc);

		printf("%s\n", dump);

		camqp_util_free(dump);
		camqp_data_free(enc);
		camqp_element_free((camqp_element*) pt);
	}

	// short
	{
		camqp_primitive* pt = camqp_primitive_int(ctx1, CAMQP_TYPE_SHORT, -1367);

		camqp_data* enc = camqp_element_encode((camqp_element*) pt);
		camqp_char* dump = camqp_data_dump(enc);

		printf("%s\n", dump);

		camqp_util_free(dump);
		camqp_data_free(enc);
		camqp_element_free((camqp_element*) pt);
	}

	// int
	{
		camqp_primitive* pt = camqp_primitive_int(ctx1, CAMQP_TYPE_INT, -46377105);

		camqp_data* enc = camqp_element_encode((camqp_element*) pt);
		camqp_char* dump = camqp_data_dump(enc);

		printf("%s\n", dump);

		camqp_util_free(dump);
		camqp_data_free(enc);
		camqp_element_free((camqp_element*) pt);
	}

	// long
	{
		camqp_primitive* pt = camqp_primitive_int(ctx1, CAMQP_TYPE_LONG, 80181598485);

		camqp_data* enc = camqp_element_encode((camqp_element*) pt);
		camqp_char* dump = camqp_data_dump(enc);

		printf("%s\n", dump);

		camqp_util_free(dump);
		camqp_data_free(enc);
		camqp_element_free((camqp_element*) pt);
	}

	// float
	{
		camqp_primitive* pt = camqp_primitive_float(ctx1, CAMQP_TYPE_FLOAT, 3.14);

		camqp_data* enc = camqp_element_encode((camqp_element*) pt);
		camqp_char* dump = camqp_data_dump(enc);

		printf("%s\n", dump);

		camqp_util_free(dump);
		camqp_data_free(enc);
		camqp_element_free((camqp_element*) pt);
	}

	// double
	{
		camqp_primitive* pt = camqp_primitive_double(ctx1, CAMQP_TYPE_DOUBLE, 3.1415);

		camqp_data* enc = camqp_element_encode((camqp_element*) pt);
		camqp_char* dump = camqp_data_dump(enc);

		printf("%s\n", dump);

		camqp_util_free(dump);
		camqp_data_free(enc);
		camqp_element_free((camqp_element*) pt);
	}

	// uuid
	{
		camqp_primitive* pt = camqp_primitive_uuid(ctx1);

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
		camqp_primitive* pt = camqp_primitive_string(ctx1, CAMQP_TYPE_CHAR, (camqp_char*) "Z");

		camqp_data* enc = camqp_element_encode((camqp_element*) pt);
		camqp_char* dump = camqp_data_dump(enc);

		printf("%s\n", dump);

		camqp_util_free(dump);
		camqp_data_free(enc);
		camqp_element_free((camqp_element*) pt);
	}

	// string
	{
		camqp_primitive* pt = camqp_primitive_string(ctx1, CAMQP_TYPE_SYMBOL, (camqp_char*) "AJS da peli peli!");

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
		camqp_primitive* pt = camqp_primitive_binary(ctx1, &dt);

		camqp_data* enc = camqp_element_encode((camqp_element*) pt);
		camqp_char* dump = camqp_data_dump(enc);

		printf("%s\n", dump);

		camqp_util_free(dump);
		camqp_data_free(enc);
		camqp_element_free((camqp_element*) pt);
	}

	// ---

	camqp_context_free(ctx1);

	return EXIT_SUCCESS;
}
