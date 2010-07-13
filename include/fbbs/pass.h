#ifndef FB_PASS_H
#define FB_PASS_H

#include <stdbool.h>

enum {
	/** Maximum length for a password. (DES: 8 chars, MD5: also 8.) */
	MAX_PASSWORD_LENGTH = 24,
};

extern const char *generate_passwd(const char *pw);
extern bool check_passwd(const char *pw_crypted, const char *pw_try);

#endif // FB_PASS_H
