#include "fbbs/list.h"
#include "fbbs/pool.h"
#include "fbbs/web.h"
#include "fbbs/xml.h"

struct xml_attr_t {
	int type;
	const char *name;
	union {
		const char *str;
		int integer;
		int64_t bigint;
		bool boolean;
	} value;
	SLIST_FIELD(xml_attr_t) next;
};

SLIST_HEAD(xml_attr_list_t, xml_attr_t);
SLIST_HEAD(xml_node_list_t, xml_node_t);

struct xml_node_t {
	int type;
	const char *name;
	struct xml_attr_list_t attr;
	struct xml_node_list_t child;
	SLIST_FIELD(xml_node_t) next;
};

struct xml_document_t {
	xml_node_t *root;
};

xml_document_t *xml_new_doc(void)
{
	xml_document_t *doc = palloc(sizeof(*doc));
	doc->root = NULL;
	return doc;
}

void xml_set_doc_root(xml_document_t *doc, xml_node_t *root)
{
	doc->root = root;
}

xml_node_t *xml_new_node(const char *name, int type)
{
	xml_node_t *node = palloc(sizeof(*node));
	node->name = name;
	node->type = type;
	SLIST_INIT_HEAD(&node->attr);
	SLIST_INIT_HEAD(&node->child);
	return node;
}

xml_node_t *xml_add_child(xml_node_t *parent, xml_node_t *child)
{
	SLIST_INSERT_HEAD(&parent->child, child, next);
	return child;
}

static xml_attr_t *xml_add_attr(xml_node_t *node, xml_attr_t *attr)
{
	SLIST_INSERT_HEAD(&node->attr, attr, next);
	return attr;
}

#define XML_ATTR_HELPER(attr_type, field, attr_value)  \
	do { \
		xml_attr_t *attr = palloc(sizeof(*attr)); \
		attr->name = name; \
		attr->type = attr_type | as_node; \
		attr->value.field = attr_value; \
		return xml_add_attr(node, attr); \
	} while (0)

xml_attr_t *xml_attr_value(xml_node_t *node, const char *name,
		const char *value, bool as_node)
{
	XML_ATTR_HELPER(XML_ATTR_TYPE_VALUE, str, pstrdup(value));
}

xml_attr_t *xml_attr_integer(xml_node_t *node, const char *name, int value,
		bool as_node)
{
	XML_ATTR_HELPER(XML_ATTR_TYPE_INTEGER, integer, value);
}

xml_attr_t *xml_attr_bigint(xml_node_t *node, const char *name, int64_t value,
		bool as_node)
{
	XML_ATTR_HELPER(XML_ATTR_TYPE_BIGINT, bigint, value);
}

xml_attr_t *xml_attr_boolean(xml_node_t *node, const char *name, bool value,
		bool as_node)
{
	XML_ATTR_HELPER(XML_ATTR_TYPE_BOOLEAN, boolean, value);
}

