/*
	PSPRadio / Music streaming client for the PSP. (Initial Release: Sept. 2005)
	PSPRadio Copyright (C) 2005 Rafael Cabezas a.k.a. Raf
	TextUI3D Copyright (C) 2005 Jesper Sandberg & Raf


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
#include <list>
#include <PSPApp.h>
#include <PSPSound.h>
#include <Logging.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <malloc.h>
#include <math.h>
#include <time.h>
#include <iniparser.h>
#include <Tools.h>
#include <stdarg.h>
#include <Logging.h>

#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspgum.h>

#include "TextUI3D.h"
#include "TextUI3D_Panel.h"
#include "jsaTextureCache.h"

/* Global texture cache */
extern jsaTextureCache tcache;

CTextUI3D_Panel::CTextUI3D_Panel()
{
	m_xpos 			= 0;
	m_ypos 			= 0;
	m_zpos 			= 0;

	m_width			= 0;
	m_height		= 0;
	m_widthscale	= 0;
	m_heightscale	= 0;
	m_scale			= 1.0f;
	m_opacity		= 1.0f;
	m_alpha			= 0xFF000000;
	ModuleLog(LOG_VERYLOW, "Panel:Created");
}

CTextUI3D_Panel::~CTextUI3D_Panel()
{
	ModuleLog(LOG_VERYLOW, "Panel:Destroyed");
}

void CTextUI3D_Panel::SetState(PanelState *state)
{
	SetPosition(state->x, state->y, state->z);
	SetSize(state->w, state->h);
	SetScale(state->scale);
	SetOpacity(state->opacity);
}

CTextUI3D_Panel::PanelState *CTextUI3D_Panel::GetState()
{
	m_state.x	= (int) m_xpos;
	m_state.y	= (int) m_ypos;
	m_state.z	= (int) m_zpos;

	m_state.w	= (int) m_width;
	m_state.h	= (int) m_height;

	m_state.scale	= m_scale;
	m_state.opacity	= m_opacity;

	return &m_state;
}

void CTextUI3D_Panel::SetPosition(int x, int y, int z = 0)
{
	m_xpos = (float)x;
	m_ypos = (float)y;
	m_zpos = (float)z;
}

void CTextUI3D_Panel::SetSize(int width, int height)
{
	m_width			= (float)width;
	m_height		= (float)height;
	m_widthscale	= m_width;
	m_heightscale	= m_height;
}

void CTextUI3D_Panel::SetFrameTexture(FrameTextures &textures)
{
	m_frametextures = textures;
}

void CTextUI3D_Panel::SetOpacity(float opacity)
{
	m_opacity = opacity;

	/* Calculate new alpha value */
	m_alpha = (unsigned int)(255.0f * opacity);
	m_alpha = m_alpha << 24;
}

void CTextUI3D_Panel::SetScale(float scale)
{
	m_widthscale	= m_width * scale;
	m_heightscale	= m_height * scale;
	m_scale			= scale;
}

void CTextUI3D_Panel::ResetView()
{
	m_widthscale	= m_width;
	m_heightscale	= m_height;
	m_scale 		= 1.0f;
}

void CTextUI3D_Panel::Render(int color)
{

	sceGuEnable(GU_TEXTURE_2D);

	sceGuAlphaFunc( GU_GREATER, 0, 0xff );
	sceGuEnable( GU_ALPHA_TEST );

	sceGuTexFunc(GU_TFX_BLEND,GU_TCC_RGBA);
	sceGuTexEnvColor(color);

	sceGuDepthFunc(GU_ALWAYS);

	sceGuBlendFunc( GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0 );
	sceGuEnable( GU_BLEND );

	sceGuTexWrap(GU_REPEAT, GU_REPEAT);
	sceGuTexFilter(GU_LINEAR, GU_LINEAR);

	m_vertex_array = (Vertex *) sceGuGetMemory(18 * sizeof(Vertex));
	UpdateVertexArray();

	for (int i = 0 ; i < 9 ; i++)
	{
		(void)tcache.jsaTCacheSetTexture(m_frametextures.texture_ids[i]);
		sceGuDrawArray(GU_SPRITES,GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_2D, 2, 0, &m_vertex_array[i*2]);
	}

	sceGuDisable(GU_BLEND);
	sceGuDisable(GU_ALPHA_TEST);
	sceGuDisable(GU_TEXTURE_2D);
	sceGuDepthFunc(GU_GEQUAL);
}

void CTextUI3D_Panel::UpdateVertexArray()
{
	float	xpos_array[] = 	{
							m_xpos,										m_xpos + m_frametextures.width,
							m_xpos + m_frametextures.width,				m_xpos + m_frametextures.width + m_width,
							m_xpos + m_frametextures.width + m_width,	m_xpos + 2 * m_frametextures.width + m_width
							};
	float	xpos_scale[6];

	float	ypos_array[] = 	{
							m_ypos,										m_ypos + m_frametextures.height,
							m_ypos + m_frametextures.height,			m_ypos + m_frametextures.height + m_height,
							m_ypos + m_frametextures.height + m_height,	m_ypos + 2 * m_frametextures.height + m_height
							};
	float	ypos_scale[7];

	int		index = 0;
	int		x_coord = 0, y_coord = 0;

	/* Scale the panel. Don't scale the starting point */
	for (int i = 0 ; i < 6 ; i++)
		{
		xpos_scale[i] = ((xpos_array[i] - m_xpos) * m_scale) + m_xpos;
		ypos_scale[i] = ((ypos_array[i] - m_ypos) * m_scale) + m_ypos;
		}

	/* Set zpos for all vertices */
	for (int i = 0 ; i < 18 ; i++)
	{
		m_vertex_array[i].z		= m_zpos;
		m_vertex_array[i].color = m_alpha;

		/* Every second vertex have (u,v) set to (0,0) */
		if (!(i % 2))
		{
			m_vertex_array[i].u = 0;
			m_vertex_array[i].v = 0;
		}
	}

	/* Set x and y positions and texture coords */
	for (int y = 0 ; y < 3 ; y++)
	{
		for (int x = 0 ; x < 3 ; x++)
		{
			m_vertex_array[index + 0].x = xpos_scale[x_coord + 0];
			m_vertex_array[index + 1].x = xpos_scale[x_coord + 1];

			m_vertex_array[index + 0].y = ypos_scale[y_coord + 0];
			m_vertex_array[index + 1].y = ypos_scale[y_coord + 1];

			/* Texture coords */
			m_vertex_array[index + 1].u = xpos_array[x_coord + 1] - xpos_array[x_coord + 0];
			m_vertex_array[index + 1].v = ypos_array[y_coord + 1] - ypos_array[y_coord + 0];

			x_coord = (x_coord + 2) % 6;
			index += 2;
		}
		y_coord += 2;
	}
}
