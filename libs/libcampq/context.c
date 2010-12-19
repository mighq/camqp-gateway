#include "camqp.h"
#include "internals.h"

#include <libxml/xinclude.h>
#include <libxml/xpathInternals.h>

#define	NAMESPACE_AMQP	"http://www.amqp.org/schema/amqp.xsd"

/**
 * TODO: filename is not UTF-8 ready!
 */
camqp_context* camqp_context_new(const camqp_char* protocol, const camqp_char* definition) {
	// allocate result structure
	camqp_context* ctx = camqp_util_new(sizeof(camqp_context));
	if (!ctx)
		return NULL;

	// save protocol file and name
	ctx->protocol = xmlStrdup(protocol);
	ctx->definition = xmlStrdup(definition);

	// init libxml
	xmlInitParser();
	LIBXML_TEST_VERSION

	// load XML document
	ctx->xml = xmlParseFile((const char*) definition);

	if (ctx->xml == NULL) {
		fprintf(stderr, "Unable to parse file\n");
		xmlCleanupParser();
		camqp_context_free(ctx);
		return NULL;
	}

	if (xmlXIncludeProcess(ctx->xml) <= 0) {
		fprintf(stderr, "XInclude processing failed\n");
		xmlCleanupParser();
		xmlFreeDoc(ctx->xml);
		camqp_context_free(ctx);
		return NULL;
	}

	// Create xpath evaluation context
	ctx->xpath = xmlXPathNewContext(ctx->xml);
	if (ctx->xpath == NULL) {
		fprintf(stderr, "Error: unable to create new XPath context\n");
		xmlCleanupParser();
		xmlFreeDoc(ctx->xml);
		camqp_context_free(ctx);
		return NULL;
	}

	xmlXPathRegisterNs(ctx->xpath, (xmlChar*) "amqp", (xmlChar*) NAMESPACE_AMQP);

	// shutdown libxml
	xmlCleanupParser();

	// now we have parsed XML definition

	// check for correct protocol definition
	xmlNodePtr root = xmlDocGetRootElement(ctx->xml);
	xmlChar* protocol_def = xmlGetProp(root, (xmlChar*) "name");
	xmlChar* protocol_req = xmlStrdup(protocol);

	if (!xmlStrEqual(protocol_def, protocol_req)) {
		fprintf(stderr, "Requested protocol is not the one defined in XML definition!\n");
		xmlFree(protocol_def);
		xmlFree(protocol_req);
		camqp_context_free(ctx);
		return NULL;
	}

	xmlFree(protocol_def);
	xmlFree(protocol_req);

	return ctx;
}

void camqp_context_free(camqp_context* context) {
	if (context == NULL)
		return;

	// free XML document
	xmlXPathFreeContext(context->xpath);
	xmlFreeDoc(context->xml);

	// free string members
	camqp_util_free(context->protocol);
	camqp_util_free(context->definition);

	// free structure
	camqp_util_free(context);
}
