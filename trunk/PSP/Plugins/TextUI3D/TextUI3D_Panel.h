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

#ifndef _TEXTUI3D_PANEL_
#define _TEXTUI3D_PANEL_

using namespace std;


class CTextUI3D_Panel
{

public:
	typedef struct FrameTextures
		{
		int	width, height;
		int	texture_ids[9];
		};

	typedef struct PanelState
		{
		int		x, y, z;
		int		w, h;
		float	opacity;
		float	scale;
		};

public:
	CTextUI3D_Panel();
	~CTextUI3D_Panel();

	void SetState(PanelState *state);
	CTextUI3D_Panel::PanelState *GetState();

	void SetPosition(int x, int y, int z);
	int	 GetPositionX() {return (int)m_xpos;};
	int	 GetPositionY() {return (int)m_ypos;};

	void SetSize(int width, int height);

	void SetFrameTexture(FrameTextures &textures);

	void SetOpacity(float opacity);
	float GetOpacity() {return m_opacity;};

	void SetScale(float scale);
	float GetScale() {return m_scale;};

	void ResetView();

	void Render(int color);

private:

	typedef struct Vertex
	{
		float u, v;
		unsigned int color;
		float x,y,z;
	};

private:
	void UpdateVertexArray();

private:
	float			m_xpos, m_ypos, m_zpos;
	float			m_width, m_height;
	float			m_widthscale, m_heightscale;
	float			m_scale;
	float			m_opacity;

	PanelState		m_state;
	void			*m_data;

	FrameTextures	m_frametextures;
	Vertex			*m_vertex_array;
	unsigned int	m_alpha;
};

#endif
