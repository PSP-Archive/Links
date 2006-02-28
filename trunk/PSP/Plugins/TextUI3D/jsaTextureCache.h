/*
	Texture cache manager for the Sony PSP.
	Copyright (C) 2005 Jesper Sandberg


	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#ifndef _JSATEXTURECACHE_
#define _JSATEXTURECACHE_

#include <list>
#include <jsaVRAMManager.h>

using namespace std;

class jsaVRAMManager;

class jsaTextureCache
{
public:
	typedef struct jsaTextureInfo
	{
		int	format;			/* The format of the texture (i.e. GU_PSM_8888)		*/
		int	width;			/* Width of the texture.				*/
		int	height;			/* Height of the texture.				*/
		int	swizzle;		/* If this is true the texture will be swizzled when	*/
						/* being loaded to VRAM					*/
	};

public:
	jsaTextureCache() {};
	~jsaTextureCache() {};

	bool jsaTCacheStoreTexture(int ID, jsaTextureInfo *texture_info, void *tbuffer);
	bool jsaTCacheSetTexture(int ID);
	int jsaTCacheLoadPngImage(const char* filename, u32 *ImageBuffer);
	int jsaTCacheLoadRawImage(const char* filename, u32 *ImageBuffer);
	float jsaTCacheTexturePixelSize(int format);

private:
	struct jsaTextureItem
	{
		int		ID;		/* The ID for the texture. Used for reference later	*/
		int		format;		/* The format of the texture (i.e. GU_PSM_8888)		*/
		int		width;		/* Width of the texture.				*/
		int		height;		/* Height of the texture.				*/
		unsigned long	offset;		/* Offset into VRAM where the texture is loaded.	*/
		bool		swizzle;	/* If this is true the texture is swizzled              */
	};

private:
	void jsaTCacheSwizzleUpload(unsigned char *dest, unsigned char *source, int width, int height);

private:
	list<jsaTextureItem> m_TextureList;
};

#endif
