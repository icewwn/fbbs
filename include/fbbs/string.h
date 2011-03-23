#ifndef FB_STRING_H
#define FB_STRING_H

#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#define streq(a, b)          (!strcmp(a, b))
#define strneq(a, b, n)      (!strncmp(a, b, n))
#define strcaseeq(a, b)      (!strcasecmp(a, b))
#define strcaseneq(a, b, n)  (!strcasencmp(a, b, n))

extern char *strtolower(char *dst, const char *src);
extern char *strtoupper(char *dst, const char *src);
extern char *strcasestr_gbk(const char *haystack, const char *needle);
extern char *ansi_filter(char *dst, const char *src);
extern int ellipsis(char *str, int len);
extern char *rtrim(char *str);
extern char *trim(char *str);
extern size_t strlcpy(char *dst, const char *src, size_t siz);
extern void strtourl(char *url, const char *str);
extern void strappend(char **dst, size_t *size, const char *src);
extern void printable_filter(char *str);
extern int valid_gbk(unsigned char *str, int len, int replace);
extern const char *check_gbk(const char *title);

static inline bool isprint2(int ch)
{
	unsigned char c = ch;
	return (((c & 0x80) && c != 0xFF) || isprint(c));
}

#endif // FB_STRING_H
