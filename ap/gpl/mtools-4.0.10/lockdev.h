#ifndef LOCK_DEV
#define LOCK_DEV

/*  Copyright 2005,2009 Alain Knaff.
 *  This file is part of mtools.
 *
 *  Mtools is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Mtools is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Mtools.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Create an advisory lock on the device to prevent concurrent writes.
 * Uses either lockf, flock, or fcntl locking methods.  See the Makefile
 * and the Configure files for how to specify the proper method.
 */

int lock_dev(int fd, int mode, struct device *dev)
{
#if (defined(HAVE_FLOCK) && defined (LOCK_EX) && defined(LOCK_NB))
	/**/
#else /* FLOCK */

#if (defined(HAVE_LOCKF) && defined(F_TLOCK))
	/**/
#else /* LOCKF */

#if (defined(F_SETLK) && defined(F_WRLCK))
	struct flock flk;

#endif /* FCNTL */
#endif /* LOCKF */
#endif /* FLOCK */

	if(IS_NOLOCK(dev))
		return 0;

#if (defined(HAVE_FLOCK) && defined (LOCK_EX) && defined(LOCK_NB))
	if (flock(fd, (mode ? LOCK_EX : LOCK_SH)|LOCK_NB) < 0)
#else /* FLOCK */

#if (defined(HAVE_LOCKF) && defined(F_TLOCK))
	if (mode && lockf(fd, F_TLOCK, 0) < 0)
#else /* LOCKF */

#if (defined(F_SETLK) && defined(F_WRLCK))
	flk.l_type = mode ? F_WRLCK : F_RDLCK;
	flk.l_whence = 0;
	flk.l_start = 0L;
	flk.l_len = 0L;

	if (fcntl(fd, F_SETLK, &flk) < 0)
#endif /* FCNTL */
#endif /* LOCKF */
#endif /* FLOCK */
	{
		if(errno == EINVAL
#ifdef  EOPNOTSUPP 
		   || errno ==  EOPNOTSUPP
#endif
		  )
			return 0;
		else
			return 1;
	}
	return 0;
}


#endif
