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

int memcmp(const void *_s1, const void *_s2, size_t n)
{
	const char *s1 = _s1;
	const char *s2 = _s2;
	size_t i;

	for (i = 0; i < n; i++) {
		if (s1[i] != s2[i])
			return s1[i] - s2[i];
	}

	return 0;
}
