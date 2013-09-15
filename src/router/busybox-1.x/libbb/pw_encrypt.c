/* vi: set sw=4 ts=4: */
/*
 * Utility routine.
 *
 * Copyright (C) 1999-2004 by Erik Andersen <andersen@codepoet.org>
 *
 * Licensed under GPLv2 or later, see file LICENSE in this tarball for details.
 */

#include "libbb.h"
#include <crypt.h>

char *pw_encrypt(const char *clear, const char *salt)
{
	/* Was static char[BIGNUM]. Malloced thing works as well */
	static char *cipher;


	free(cipher);
	cipher = xstrdup(crypt(clear, salt));
	return cipher;
}
