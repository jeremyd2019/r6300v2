#ifndef BYTE_DWORD
#define BYTE_DWORD

/*  Copyright 2007,2009 Alain Knaff.
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
 */

static Dword byte2dword(Byte* val)
{
	Dword l;
	l = (val[0] << 24) + (val[1] << 16) + (val[2] << 8) + val[3];
	
#ifdef DEBUG
	fprintf(stderr, "byte2dword(): %ld, 0x%x\n", l, (unsigned int) l);
#endif		
	
	return l;
}	

static void dword2byte(Dword parm, Byte* rval)
{
	rval[0] = (parm >> 24) & 0xff;
	rval[1] = (parm >> 16) & 0xff;
	rval[2] = (parm >> 8)  & 0xff;
	rval[3] = parm         & 0xff;
}

#endif
