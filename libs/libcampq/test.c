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

	/// camqp_string
	camqp_string* str1 = camqp_string_new((camqp_char*)"Mighq", 5);
	if (!str1)
		err("T002");

	camqp_string str2 = camqp_string_static_nt((camqp_char*)"pela");
	printf("%.4s:%d\n", str2.chars, str2.length);

	camqp_string_free(str1);
	// ---

	/// camqp_context
	const char* file = "messaging-v1.0.xml";
	const char* proto = "messaging-v1.0";

	camqp_string* s_file = camqp_string_new((camqp_char*)file, strlen(file));
	camqp_string* s_proto = camqp_string_new((camqp_char*)proto, strlen(proto));

	// create context
	camqp_context* ctx1 = camqp_context_new(s_proto, s_file);
	camqp_string_free(s_file);
	camqp_string_free(s_proto);

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
	camqp_string str3 = camqp_string_static_nt((camqp_char*) "pela_hopa");

	camqp_primitive* pt6 = camqp_primitive_string(ctx1, CAMQP_TYPE_STRING, &str3);
	camqp_string* vl6 = camqp_value_string(pt6);
	printf("%.9s:%d\n", vl6->chars, vl6->length);
	camqp_primitive_free(pt6);

	// binary
	camqp_data* dt2 = camqp_data_new((camqp_byte*)"XYZ", 3);
	if (!dt2)
		err("T004");

	camqp_primitive* pt7 = camqp_primitive_binary(ctx1, dt2);
	camqp_data* vl7 = camqp_value_binary(pt7);
	printf("%.3s:%d\n", vl7->bytes, vl7->size);
	camqp_primitive_free(pt7);

	camqp_data_free(dt2);
	// ---

	/// camqp_vector
	camqp_string str4 = camqp_string_static_nt((camqp_char*) "key1");
	camqp_primitive* pt8 = camqp_primitive_uint(ctx1, CAMQP_TYPE_UINT, 418);

	camqp_vector* vec1 = camqp_vector_new(ctx1);
	camqp_vector_item_put(vec1, &str4, (camqp_element*) pt8);
	camqp_vector_free(vec1);

	camqp_primitive_free(pt8);
	// ---

	camqp_context_free(ctx1);

	return EXIT_SUCCESS;
}
