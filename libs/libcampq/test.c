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

	camqp_context_free(ctx1);

	// ---

	return EXIT_SUCCESS;
}
