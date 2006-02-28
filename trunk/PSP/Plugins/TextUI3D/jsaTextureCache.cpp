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

#include <stdio.h>
#include <PSPApp.h>
#include <TextUI3D.h>

#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspge.h>
#include <png.h>

#include "jsaTextureCache.h"


bool jsaTextureCache::jsaTCacheStoreTexture(int ID, jsaTextureInfo *texture_info, void *tbuffer)
{
	bool		ret_value = false;
	unsigned long	texture_address;
	jsaTextureItem	Texture;
	unsigned long	tsize;
	float		bytes_pr_pixel;

	ModuleLog(LOG_VERYLOW, "TC:Storing Texture : %d", ID);

	bytes_pr_pixel	= jsaTCacheTexturePixelSize(texture_info->format);
	tsize		= (unsigned long)(bytes_pr_pixel * texture_info->width * texture_info->height);

	texture_address = (unsigned long)jsaVRAMManager::jsaVRAMManagerMalloc(tsize);

	ModuleLog(LOG_VERYLOW, "TC:Memory allocated : %d", ID);

	if (texture_address)
	{
		/* Add texture to list */
		Texture.ID	= ID;
		Texture.format	= texture_info->format;
		Texture.width	= texture_info->width;
		Texture.height	= texture_info->height;
		Texture.offset	= texture_address;
		Texture.swizzle	= texture_info->swizzle;
		m_TextureList.push_back(Texture);

		/* Upload texture to VRAM */
		if (texture_info->swizzle)
		{
			ModuleLog(LOG_VERYLOW, "TC:Swizzle upload : %d", ID);
			jsaTCacheSwizzleUpload((unsigned char *)texture_address, (unsigned char *)tbuffer, (int)(texture_info->width * bytes_pr_pixel), texture_info->height);
		}
		else
		{
			ModuleLog(LOG_VERYLOW, "TC:Normal upload : %d", ID);
			memcpy((void *)texture_address, tbuffer, tsize);
		}
		ret_value 	= true;
		ModuleLog(LOG_VERYLOW, "TC:Texture stored in VRAM : %d", ID);
	}
	else
	{
		ModuleLog(LOG_ERROR, "TC:Couldn't get VRAM for texture : %d", ID);
	}
	return ret_value;
}

bool jsaTextureCache::jsaTCacheSetTexture(int ID)
{
	list<jsaTextureItem>::iterator 	TextureIterator;
	bool				found = false;

	if (m_TextureList.size() > 0)
	{
		for (TextureIterator = m_TextureList.begin() ; TextureIterator != m_TextureList.end() ; TextureIterator++)
		{
			if ((*TextureIterator).ID == ID)
			{
				/* setup texture */
				if ((*TextureIterator).swizzle)
				{
					sceGuTexMode((*TextureIterator).format,0,0,GU_TRUE);
				}
				else
				{
					sceGuTexMode((*TextureIterator).format,0,0,GU_FALSE);
				}
				sceGuTexImage(0,(*TextureIterator).width, (*TextureIterator).height,(*TextureIterator).width, (void *)((*TextureIterator).offset));
				found = true;
			}
/*
			else
			{
				ModuleLog(LOG_ERROR, "TC:Texture not in cache : %d", ID);
			}
*/
		}
	}
	else
	{
		ModuleLog(LOG_ERROR, "TC:No textures stored in VRAM.");
	}
	return found;
}

float jsaTextureCache::jsaTCacheTexturePixelSize(int format)
{
	float	size;

	switch (format)
		{
		case	GU_PSM_5650:
			size = 2;
			break;
		case	GU_PSM_5551:
			size = 2;
			break;
		case	GU_PSM_4444:
			size = 2;
			break;
		case	GU_PSM_8888:
			size = 4;
			break;
		case	GU_PSM_T4:
			size = 0.5;
			break;
		case	GU_PSM_T8:
			size = 1;
			break;
		default:
			size = 0;
			break;
		}

	return size;
}

/* This code is originally done by chp from ps2dev.org. */
void jsaTextureCache::jsaTCacheSwizzleUpload(unsigned char *dest, unsigned char *source, int width, int height)
{
	int i,j;
	int rowblocks = (width / 16);

	for (j = 0; j < height; ++j)
	{
		for (i = 0; i < width; ++i)
		{
			unsigned int blockx = i / 16;
			unsigned int blocky = j / 8;

			unsigned int x = (i - blockx*16);
			unsigned int y = (j - blocky*8);
			unsigned int block_index = blockx + ((blocky) * rowblocks);
			unsigned int block_address = block_index * 16 * 8;

			dest[block_address + x + y * 16] = source[i+j*width];
		}
	}
}

static void user_warning_fn(png_structp png_ptr, png_const_charp warning_msg)
{
	// ignore PNG warnings
}

/* Load a texture from a png image */
int jsaTextureCache::jsaTCacheLoadPngImage(const char* filename, u32 *ImageBuffer)
{
	png_structp png_ptr;
	png_infop info_ptr;
	unsigned int sig_read = 0;
	png_uint_32 width, height;
	int bit_depth, color_type, interlace_type;
	size_t x, y;
	u32* line;
	FILE *fp;

	if ((fp = fopen(filename, "rb")) == NULL) return -1;
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png_ptr == NULL) {
		fclose(fp);
		return -1;
	}
	png_set_error_fn(png_ptr, (png_voidp) NULL, (png_error_ptr) NULL, user_warning_fn);
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) {
		fclose(fp);
		png_destroy_read_struct(&png_ptr, png_infopp_NULL, png_infopp_NULL);
		return -1;
	}
	png_init_io(png_ptr, fp);
	png_set_sig_bytes(png_ptr, sig_read);
	png_read_info(png_ptr, info_ptr);
	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, int_p_NULL, int_p_NULL);
	png_set_strip_16(png_ptr);
	png_set_packing(png_ptr);
	if (color_type == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png_ptr);
	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) png_set_gray_1_2_4_to_8(png_ptr);
	if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png_ptr);
	png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);
	line = (u32*) malloc(width * 4);
	if (!line) {
		fclose(fp);
		png_destroy_read_struct(&png_ptr, png_infopp_NULL, png_infopp_NULL);
		return -1;
	}
	for (y = 0; y < height; y++)
	{
		png_read_row(png_ptr, (u8*) line, png_bytep_NULL);
		for (x = 0; x < width; x++)
		{
			ImageBuffer[y*width+x] = line[x];
		}
	}
	free(line);
	png_read_end(png_ptr, info_ptr);
	png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
	fclose(fp);
	return 0;
}

/* Load a texture from a raw image */
int jsaTextureCache::jsaTCacheLoadRawImage(const char* filename, u32 *ImageBuffer)
{
	FILE			*fhandle;

	fhandle = fopen(filename, "r");
	if (fhandle != NULL)
	{
		int bytes;
		(void)fseek(fhandle, 0, SEEK_END);
		int filesize = ftell(fhandle);

		if (filesize != -1)
		{
			(void)fseek(fhandle, 0, SEEK_SET);

			bytes = fread(ImageBuffer, 1, filesize, fhandle);
			if (bytes != filesize)
			{
				ModuleLog(LOG_ERROR, "TC:Wrong filesize for %s : %d, %d", filename, filesize, bytes);
				return -1;
			}
		}
		else
		{
			ModuleLog(LOG_ERROR, "TC:Couldn't get filesize for %s", filename);
			return -1;
		}
		fclose(fhandle);
	}
	else
	{
		ModuleLog(LOG_ERROR, "TC:Couldn't load file : %s", filename);
		return -1;
	}
	return 0;
}
