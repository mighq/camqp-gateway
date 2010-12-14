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
	camqp_char* file = "messaging-v1.0.xml";
	camqp_char* proto = "messaging-v1.0";

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
	camqp_primitive* pt6 = camqp_primitive_string(ctx1, CAMQP_TYPE_STRING, "pela hopa");
	const camqp_char* vl6 = camqp_value_string(pt6);
	printf("%s\n", vl6);
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
	camqp_primitive* pt8 = camqp_primitive_uint(ctx1, CAMQP_TYPE_UINT, 418);

	camqp_vector* vec1 = camqp_vector_new(ctx1);
	camqp_vector_item_put(vec1, "key1", (camqp_element*) pt8);
	camqp_vector_free(vec1);

	camqp_primitive_free(pt8);
	// ---

	camqp_context_free(ctx1);

	return EXIT_SUCCESS;
}
