/*
 * Copyright (c) 2024 Free Software Foundation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

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
