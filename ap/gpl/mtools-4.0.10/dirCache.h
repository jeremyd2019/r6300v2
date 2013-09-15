#ifndef MTOOLS_DIRCACHE_H
#define MTOOLS_DIRCACHE_H

/*  Copyright 1997,1999,2001-2003,2008,2009 Alain Knaff.
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
typedef enum {
	DCET_FREE,
	DCET_USED,
	DCET_END
} dirCacheEntryType_t;

#define DC_BITMAP_SIZE 128

typedef struct dirCacheEntry_t {
	dirCacheEntryType_t type;
	unsigned int beginSlot;
	unsigned int endSlot;
	wchar_t *shortName;
	wchar_t *longName;
	struct directory dir;
} dirCacheEntry_t;

typedef struct dirCache_t {
	struct dirCacheEntry_t **entries;
	int nr_entries;
	unsigned int nrHashed;
	unsigned int bm0[DC_BITMAP_SIZE];
	unsigned int bm1[DC_BITMAP_SIZE];
	unsigned int bm2[DC_BITMAP_SIZE];
} dirCache_t;

int isHashed(dirCache_t *cache, wchar_t *name);
int growDirCache(dirCache_t *cache, int slot);
dirCache_t *allocDirCache(Stream_t *Stream, int slot);
dirCacheEntry_t *addUsedEntry(dirCache_t *Stream, int begin, int end,
			      wchar_t *longName, wchar_t *shortName,
			      struct directory *dir);
void freeDirCache(Stream_t *Stream);
dirCacheEntry_t *addFreeEntry(dirCache_t *Stream, 
			      unsigned int begin, unsigned int end);
dirCacheEntry_t *addEndEntry(dirCache_t *Stream, int pos);
dirCacheEntry_t *lookupInDircache(dirCache_t *Stream, int pos);
#endif
