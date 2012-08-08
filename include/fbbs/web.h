#ifndef FB_WEB_H
#define FB_WEB_H

#include <fcgi_stdio.h>

#include "fbbs/cfg.h"
#include "fbbs/convert.h"
#include "fbbs/dbi.h"
#include "fbbs/pool.h"

enum {
	MAX_PARAMETERS = 32,
	MAX_CONTENT_LENGTH = 1 * 1024 * 1024,

	PARSE_NOSIG = 0x1,
	PARSE_NOQUOTEIMG = 0x2,
	PARSE_NOSIGIMG = 0x4,

	REQUEST_API = 0x1,
	REQUEST_PARSED = 0x2,
	REQUEST_MOBILE = 0x4,
	REQUEST_UTF8 = 0x8,
};

typedef struct pair_t {
	char *key;
	char *val;
} pair_t;

extern bool web_ctx_init(void);
extern void web_ctx_destroy(void);

extern const char *get_param(const char *name);
extern const pair_t *get_param_pair(int idx);
extern int parse_post_data(void);
extern bool request_type(int type);

extern void html_header(void);
extern void xml_header(const char *xslfile);

extern void xml_print(const char *s);
extern int xml_print_post(const char *file, int option);

extern const unsigned char *calc_digest(const void *s, size_t size);

extern void *palloc(size_t size);
extern char *pstrdup(const char *s);

#endif // FB_WEB_H
