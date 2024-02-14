#include <string.h>
#include <sys/types.h>

/* Nothing Aarch64-specific about these.  */

void *memset(void *_s, int c, size_t n)
{
	char *s = _s;
	size_t i;

	for (i = 0; i < n ; i++)
		s[i] = c;

	return _s;
}

void *memcpy(void *_d, const void *_s, size_t n)
{
	char *d = _d;
	const char *s = _s;
	size_t i;

	for (i = 0; i < n; i++)
		d[i] = s[i];

	return _d;
}
