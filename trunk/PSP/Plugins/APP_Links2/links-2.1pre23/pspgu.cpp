/* PSP GU
 * Linux framebuffer code
 * (c) 2002 Petr 'Brain' Kulhavy
 * This file is a part of the Links program, released under GPL.
 */

#include "cfg.h"

#ifdef GRDRV_PSPGU

#ifdef TEXT
#undef TEXT
#endif

#include <stdio.h>

#include <pspsdk.h>
#include <pspkernel.h>
#include <pspdisplay.h>

extern "C" {
#include "links.h"
};
#include <pthreadlite.h>
#include <pspctrl.h>
#include <pspgu.h>
#include <pspgum.h>
#include <signal.h>
#include "newarrow.inc"

#define DANZEFF_SCEGU
#include <danzeff.h>

#include <psp.h>
#include <valloc.h>

#define printf pspDebugScreenPrintf

static volatile int sf_danzeffOn = 0;
static volatile int s_BbRowOffset = 0, s_BbColOffset = 0;
static volatile int s_bbDirty = falsE;
static volatile int s_Zoom = falsE;

int pspgu_console;

struct itrm *pspgu_kbd;

struct graphics_device *pspgu_old_vd;

typedef struct gu_param_
{
	int virt_width, virt_height, virt_sl_pixelpitch, virt_sl_bytepitch;
	int display_width, display_height, display_sl_pixelpitch, display_sl_bytepitch;
	int virt_pixel_mode, display_pixel_mode;
	int virt_pixel_size, display_pixel_size;
	int guinitialized;

	int framesize, virtframesize;

	unsigned char *virtbuffer;

} gu_param;

gu_param gu_data;

int border_left, border_right, border_top, border_bottom;
int pspgu_colors;

void pspgu_draw_bitmap(struct graphics_device *dev,struct bitmap* hndl, int x, int y);

static unsigned char *pspgu_driver_param;
extern struct graphics_driver pspgu_driver;
volatile int pspgu_active=1;

static volatile int in_gr_operation;

/* mouse */
static int mouse_x, mouse_y;		/* mouse pointer coordinates */
static int mouse_black, mouse_white;
static int background_x, background_y; /* Where was the mouse background taken from */
static unsigned char *mouse_buffer, *background_buffer, *new_background_buffer;
static struct graphics_device *mouse_graphics_device;
static int global_mouse_hidden;

/* external functions */
extern "C" {
void TakeScreenShot();
void wifiChooseConnect();
void psp_init();
};
#define TEST_MOUSE(xl,xh,yl,yh) if (RECTANGLES_INTERSECT(\
					(xl),(xh),\
					background_x,background_x+arrow_width,\
					(yl),(yh),\
					background_y,background_y+arrow_height)\
					&& !global_mouse_hidden){\
					mouse_hidden=1;\
					hide_mouse();\
				}else mouse_hidden=0;

#define END_MOUSE if (mouse_hidden) show_mouse();

#define START_GR in_gr_operation=1;
#define END_GR	\
		END_MOUSE\
		in_gr_operation=0;


#define NUMBER_OF_DEVICES	1



#define TEST_INACTIVITY if (!pspgu_active||dev!=current_virtual_device) return;

#define TEST_INACTIVITY_0 if (!pspgu_active||dev!=current_virtual_device) return 0;

#define RECTANGLES_INTERSECT(xl0, xh0, xl1, xh1, yl0, yh0, yl1, yh1) (\
				   (xl0)<(xh1)\
				&& (xl1)<(xh0)\
				&& (yl0)<(yh1)\
				&& (yl1)<(yh0))

/* This assures that x, y, xs, ys, data will be sane according to clipping
 * rectangle. If nothing lies within this rectangle, the current function
 * returns. The data pointer is automatically advanced by this macro to reflect
 * the right position to start with inside the bitmap. */
#define	CLIP_PREFACE \
	int mouse_hidden;\
	int xs=hndl->x,ys=hndl->y;\
        char *data=(char*)hndl->data;\
\
 	TEST_INACTIVITY\
        if (x>=dev->clip.x2||x+xs<=dev->clip.x1) return;\
        if (y>=dev->clip.y2||y+ys<=dev->clip.y1) return;\
        if (x+xs>dev->clip.x2) xs=dev->clip.x2-x;\
        if (y+ys>dev->clip.y2) ys=dev->clip.y2-y;\
        if (dev->clip.x1-x>0){\
                xs-=(dev->clip.x1-x);\
                data+=gu_data.virt_pixel_size*(dev->clip.x1-x);\
                x=dev->clip.x1;\
        }\
        if (dev->clip.y1-y>0){\
                ys-=(dev->clip.y1-y);\
                data+=hndl->skip*(dev->clip.y1-y);\
                y=dev->clip.y1;\
        }\
        /* xs, ys: how much pixels to paint\
         * data: where to start painting from\
         */\
	START_GR\
	TEST_MOUSE (x,x+xs,y,y+ys)


/* fill_area: 5,5,10,10 fills in 25 pixels! */

/* This assures that left, right, top, bottom will be sane according to the
 * clipping rectangle set up by svga_driver->set_clip_area. If empty region
 * results, return from current function occurs. */
#define FILL_CLIP_PREFACE \
	int mouse_hidden;\
	TEST_INACTIVITY\
	if (left>=right||top>=bottom) return;\
	if (left>=dev->clip.x2||right<=dev->clip.x1||top>=dev->clip.y2||bottom<=dev->clip.y1) return;\
	if (left<dev->clip.x1) left=dev->clip.x1;\
	if (right>dev->clip.x2) right=dev->clip.x2;\
	if (top<dev->clip.y1) top=dev->clip.y1;\
	if (bottom>dev->clip.y2) bottom=dev->clip.y2;\
	START_GR\
	TEST_MOUSE(left,right,top,bottom)


#define HLINE_CLIP_PREFACE \
	int mouse_hidden;\
	TEST_INACTIVITY\
	if (y<dev->clip.y1||y>=dev->clip.y2||right<=dev->clip.x1||left>=dev->clip.x2) return;\
	if (left<dev->clip.x1) left=dev->clip.x1;\
	if (right>dev->clip.x2) right=dev->clip.x2;\
	if (left>=right) return;\
	START_GR\
	TEST_MOUSE (left,right,y,y+1)

#define VLINE_CLIP_PREFACE \
	int mouse_hidden;\
	TEST_INACTIVITY\
	if (x<dev->clip.x1||x>=dev->clip.x2||top>=dev->clip.y2||bottom<=dev->clip.y1) return;\
	if (top<dev->clip.y1) top=dev->clip.y1;\
	if (bottom>dev->clip.y2) bottom=dev->clip.y2;\
	if (top>=bottom) return;\
	START_GR\
	TEST_MOUSE(x,x+1,top,bottom)

#define HSCROLL_CLIP_PREFACE \
	int mouse_hidden;\
	TEST_INACTIVITY_0\
	if (!sc) return 0;\
	if (sc>(dev->clip.x2-dev->clip.x1)||-sc>(dev->clip.x2-dev->clip.x1))\
		return 1;\
	START_GR\
	TEST_MOUSE (dev->clip.x1,dev->clip.x2,dev->clip.y1,dev->clip.y2)

#define VSCROLL_CLIP_PREFACE \
	int mouse_hidden;\
	TEST_INACTIVITY_0\
	if (!sc) return 0;\
	if (sc>dev->clip.y2-dev->clip.y1||-sc>dev->clip.y2-dev->clip.y1) return 1;\
	START_GR\
	TEST_MOUSE (dev->clip.x1, dev->clip.x2, dev->clip.y1, dev->clip.y2)\


#define IS_BUTTON_PRESSED(i,b) (((i) & 0xFFFF) == (b)) /* i = read button mask b = expected match */

#include <Screen.h>
CScreen *gScreen  = NULL;
void psp_init()
{
	CScreen *temp = gScreen;
	
	gScreen = new CScreen(false, 480, 272, 512, PSPSCR_PIXEL_FORMAT);

	if (temp)
		delete temp;

	
}

/* n is in bytes. dest must begin on pixel boundary. If n is not a whole number of pixels, rounding is
 * performed downwards.
 */
static inline void pixel_set(unsigned char *dest, int n,void * pattern)
{
#ifdef PSP_16BPP
	#ifdef t2c
		short v=*(t2c *)pattern;
		int a;
		
		for (a=0;a<(n>>1);a++) ((t2c *)dest)[a]=v;
	#else
		unsigned char a,b;
		int i;
		
		a=*(char*)pattern;
		b=((char*)pattern)[1];
		for (i=0;i<=n-2;i+=2){
			dest[i]=a;
			dest[i+1]=b;
		}
	#endif
#else /* 32bpp */
	unsigned char a,b,c,d;
	int i;

	a=*(char*)pattern;
	b=((char*)pattern)[1];
	c=((char*)pattern)[2];
	d=((char*)pattern)[3];
	for (i=0;i<=n-4;i+=4){
		dest[i]=a;
		dest[i+1]=b;
		dest[i+2]=c;
		dest[i+3]=d;
	}
#endif	
}

static void redraw_mouse(void);

static void pspgu_mouse_move(int dx, int dy, int fl)
{
	struct event ev;
	mouse_x += dx;
	mouse_y += dy;
	ev.ev = EV_MOUSE;
	if (mouse_x >= gu_data.virt_width) mouse_x = gu_data.virt_width - 1;
	if (mouse_y >= gu_data.virt_height) mouse_y = gu_data.virt_height - 1;
	if (mouse_x < 0) mouse_x = 0;
	if (mouse_y < 0) mouse_y = 0;
	ev.x = mouse_x;
	ev.y = mouse_y;
	ev.b = B_MOVE;
	if (!current_virtual_device) return;
	if (current_virtual_device->mouse_handler) current_virtual_device->mouse_handler(current_virtual_device, ev.x, ev.y, fl/*ev.b*/);
	redraw_mouse();
}

#define mouse_getscansegment(buf,x,y,w) memcpy(buf,gu_data.virtbuffer+y*gu_data.virt_sl_bytepitch+x*gu_data.virt_pixel_size,w)
#define mouse_drawscansegment(ptr,x,y,w) memcpy(gu_data.virtbuffer+y*gu_data.virt_sl_bytepitch+x*gu_data.virt_pixel_size,ptr,w);

/* Flushes the background_buffer onscreen where it was originally taken from. */
static void place_mouse_background(void)
{
	struct bitmap bmp;

	bmp.x=arrow_width;
	bmp.y=arrow_height;
	bmp.skip=arrow_width*gu_data.virt_pixel_size;
	bmp.data=background_buffer;

	{
		struct graphics_device * current_virtual_device_backup;

		current_virtual_device_backup=current_virtual_device;
		current_virtual_device=mouse_graphics_device;
		pspgu_draw_bitmap(mouse_graphics_device, &bmp, background_x,
			background_y);
		current_virtual_device=current_virtual_device_backup;
	}

}

/* Only when the old and new mouse don't interfere. Using it on interfering mouses would
 * cause a flicker.
 */
static void hide_mouse(void)
{

	global_mouse_hidden=1;
	place_mouse_background();
}

/* Gets background from the screen (clipping provided only right and bottom) to the
 * passed buffer.
 */
static void get_mouse_background(unsigned char *buffer_ptr)
{
	int width,height,skip,x,y;

	skip=arrow_width*gu_data.virt_pixel_size;

	x=mouse_x;
	y=mouse_y;

	width=gu_data.virt_pixel_size*(arrow_width+x>gu_data.virt_width?gu_data.virt_width-x:arrow_width);
	height=arrow_height+y>gu_data.virt_height?gu_data.virt_height-y:arrow_height;

	for (;height;height--){
		mouse_getscansegment(buffer_ptr,x,y,width);
		buffer_ptr+=skip;
		y++;
	}
}

/* Overlays the arrow's image over the mouse_buffer
 * Doesn't draw anything into the screen
 */
static void render_mouse_arrow(void)
{
	int x,y, reg0, reg1;
	unsigned char *mouse_ptr=mouse_buffer;
	unsigned int *arrow_ptr=arrow;

	for (y=arrow_height;y;y--){
		reg0=*arrow_ptr;
		reg1=arrow_ptr[1];
		arrow_ptr+=2;
		for (x=arrow_width;x;)
		{
			int mask=1<<(--x);

			if (reg0&mask)
				memcpy (mouse_ptr, &mouse_black, gu_data.virt_pixel_size);
			else if (reg1&mask)
				memcpy (mouse_ptr, &mouse_white, gu_data.virt_pixel_size);
			mouse_ptr+=gu_data.virt_pixel_size;
		}
	}
	s_bbDirty = truE;
}

static void place_mouse(void)
{
	struct bitmap bmp;

	bmp.x=arrow_width;
	bmp.y=arrow_height;
	bmp.skip=arrow_width*gu_data.virt_pixel_size;
	bmp.data=mouse_buffer;
	{
		struct graphics_device * current_graphics_device_backup;
		current_graphics_device_backup=current_virtual_device;
		current_virtual_device=mouse_graphics_device;
		pspgu_draw_bitmap(mouse_graphics_device, &bmp, mouse_x, mouse_y);
		current_virtual_device=current_graphics_device_backup;
	}
	global_mouse_hidden=0;
}

/* Only when the old and the new mouse positions do not interfere. Using this routine
 * on interfering positions would cause a flicker.
 */
static void show_mouse(void)
{

	get_mouse_background(background_buffer);
	background_x=mouse_x;
	background_y=mouse_y;
	memcpy(mouse_buffer,background_buffer,gu_data.virt_pixel_size*arrow_area);
	render_mouse_arrow();
	place_mouse();
}

/* Doesn't draw anything into the screen
 */
static void put_and_clip_background_buffer_over_mouse_buffer(void)
{
	unsigned char *bbufptr=background_buffer, *mbufptr=mouse_buffer;
	int left=background_x-mouse_x;
	int top=background_y-mouse_y;
	int right,bottom;
	int bmpixelsizeL=gu_data.virt_pixel_size;
	int number_of_bytes;
	int byte_skip;

	right=left+arrow_width;
	bottom=top+arrow_height;

	if (left<0){
		bbufptr-=left*bmpixelsizeL;
		left=0;
	}
	if (right>arrow_width) right=arrow_width;
	if (top<0){
		bbufptr-=top*bmpixelsizeL*arrow_width;
		top=0;
	}
	if (bottom>arrow_height) bottom=arrow_height;
	mbufptr+=bmpixelsizeL*(left+arrow_width*top);
	byte_skip=arrow_width*bmpixelsizeL;
	number_of_bytes=bmpixelsizeL*(right-left);
	for (;top<bottom;top++){
		memcpy(mbufptr,bbufptr,number_of_bytes);
		mbufptr+=byte_skip;
		bbufptr+=byte_skip;
	}
}

/* This draws both the contents of background_buffer and mouse_buffer in a scan
 * way (left-right, top-bottom), so the flicker is reduced.
 */
static inline void place_mouse_composite(void)
{
	int mouse_left=mouse_x;
	int mouse_top=mouse_y;
	int background_left=background_x;
	int background_top=background_y;
	int mouse_right=mouse_left+arrow_width;
	int mouse_bottom=mouse_top+arrow_height;
	int background_right=background_left+arrow_width;
	int background_bottom=background_top+arrow_height;
	int skip=arrow_width*gu_data.virt_pixel_size;
	int background_length,mouse_length;
	unsigned char *mouse_ptr=mouse_buffer,*background_ptr=background_buffer;
	int yend;

	if (mouse_bottom>gu_data.virt_height) mouse_bottom=gu_data.virt_height;
	if (background_bottom>gu_data.virt_height) background_bottom=gu_data.virt_height;

	/* Let's do the top part */
	if (background_top<mouse_top){
		/* Draw the background */
		background_length=background_right>gu_data.virt_width?gu_data.virt_width-background_left
			:arrow_width;
		for (;background_top<mouse_top;background_top++){
			mouse_drawscansegment(background_ptr,background_left
				,background_top,background_length*gu_data.virt_pixel_size);
			background_ptr+=skip;
		}

	}else if (background_top>mouse_top){
		/* Draw the mouse */
		mouse_length=mouse_right>gu_data.virt_width
			?gu_data.virt_width-mouse_left:arrow_width;
		for (;mouse_top<background_top;mouse_top++){
			mouse_drawscansegment(mouse_ptr,mouse_left,mouse_top,mouse_length*gu_data.virt_pixel_size);
			mouse_ptr+=skip;
		}
	}

	/* Let's do the middle part */
	yend=mouse_bottom<background_bottom?mouse_bottom:background_bottom;
	if (background_left<mouse_left){
		/* Draw background, mouse */
		mouse_length=mouse_right>gu_data.virt_width?gu_data.virt_width-mouse_left:arrow_width;
		for (;mouse_top<yend;mouse_top++){
			mouse_drawscansegment(background_ptr,background_left,mouse_top
				,(mouse_left-background_left)*gu_data.virt_pixel_size);
			mouse_drawscansegment(mouse_ptr,mouse_left,mouse_top,mouse_length*gu_data.virt_pixel_size);
			mouse_ptr+=skip;
			background_ptr+=skip;
		}

	}else{
		int l1, l2, l3;

		/* Draw mouse, background */
		mouse_length=mouse_right>gu_data.virt_width?gu_data.virt_width-mouse_left:arrow_width;
		background_length=background_right-mouse_right;
		if (background_length+mouse_right>gu_data.virt_width)
			background_length=gu_data.virt_width-mouse_right;
		l1=mouse_length*gu_data.virt_pixel_size;
		l2=(mouse_right-background_left)*gu_data.virt_pixel_size;
		l3=background_length*gu_data.virt_pixel_size;
		for (;mouse_top<yend;mouse_top++){
			mouse_drawscansegment(mouse_ptr,mouse_left,mouse_top,l1);
			if (background_length>0)
				mouse_drawscansegment(
					background_ptr +l2,
				       	mouse_right,mouse_top ,l3);
			mouse_ptr+=skip;
			background_ptr+=skip;
		}
	}

	if (background_bottom<mouse_bottom){
		/* Count over bottoms! tops will be invalid! */
		/* Draw mouse */
		mouse_length=mouse_right>gu_data.virt_width?gu_data.virt_width-mouse_left
			:arrow_width;
		for (;background_bottom<mouse_bottom;background_bottom++){
			mouse_drawscansegment(mouse_ptr,mouse_left,background_bottom
				,mouse_length*gu_data.virt_pixel_size);
			mouse_ptr+=skip;
		}
	}else{
		/* Draw background */
		background_length=background_right>gu_data.virt_width?gu_data.virt_width-background_left
			:arrow_width;
		for (;mouse_bottom<background_bottom;mouse_bottom++){
			mouse_drawscansegment(background_ptr,background_left,mouse_bottom
				,background_length*gu_data.virt_pixel_size);
			background_ptr+=skip;
		}
	}
}

/* This moves the mouse a sophisticated way when the old and new position of the
 * cursor overlap.
 */
static inline void redraw_mouse_sophisticated(void)
{
	int new_background_x, new_background_y;

	get_mouse_background(mouse_buffer);
	put_and_clip_background_buffer_over_mouse_buffer();
	memcpy(new_background_buffer,mouse_buffer,gu_data.virt_pixel_size*arrow_area);
	new_background_x=mouse_x;
	new_background_y=mouse_y;
	render_mouse_arrow();
	place_mouse_composite();
	memcpy(background_buffer,new_background_buffer,gu_data.virt_pixel_size*arrow_area);
	background_x=new_background_x;
	background_y=new_background_y;
}

static void redraw_mouse(void){

	if (!pspgu_active) return; /* We are not drawing */
	if (mouse_x!=background_x||mouse_y!=background_y){
		if (RECTANGLES_INTERSECT(
			background_x, background_x+arrow_width,
			mouse_x, mouse_x+arrow_width,
			background_y, background_y+arrow_height,
			mouse_y, mouse_y+arrow_height)){
			redraw_mouse_sophisticated();
		}else{
			/* Do a normal hide/show */
			get_mouse_background(mouse_buffer);
			memcpy(new_background_buffer,
				mouse_buffer,arrow_area*gu_data.virt_pixel_size);
			render_mouse_arrow();
			hide_mouse();
			place_mouse();
			memcpy(background_buffer,new_background_buffer
				,arrow_area*gu_data.virt_pixel_size);
			background_x=mouse_x;
			background_y=mouse_y;
		}
	}
}

void pspSetMouse(int x, int y)
{
	mouse_x = x;
	mouse_y = y;

	redraw_mouse();

	s_bbDirty = truE;
}

void pspInputThread(void *)
{
	static int oldButtonMask = 0;
	int deltax = 0, deltay = 0;
	static int danzeff_x = PSP_SCREEN_WIDTH/2-(64*3/2), danzeff_y = PSP_SCREEN_HEIGHT/2-(64*3/2);
	SceCtrlData pad;
	SceCtrlLatch latch;
	unsigned short	fl	= 0;

	if ( g_PSPEnableInput == truE )
	{

		if (g_PSPEnableRendering == falsE)
		{
			g_PSPEnableRendering = truE;
			cls_redraw_all_terminals();
		}

		sceCtrlReadBufferPositive(&pad, 1);
		sceCtrlReadLatch(&latch);

		RAND_add(&pad, sizeof(pad), sizeof(pad)); /** Add more randomness to SSL */

		if (sf_danzeffOn)
		{

			if (latch.uiMake)
			{
				// Button Pressed
				oldButtonMask = latch.uiPress;
			}
			else if (latch.uiBreak) /** Button Released */
			{
				if (oldButtonMask & PSP_CTRL_START)
				{
					/* Enter input */
					sf_danzeffOn = 0;
					danzeff_free();
					s_bbDirty = truE;
				}
				else if (oldButtonMask & PSP_CTRL_SELECT)
				{
					g_PSPEnableRendering = falsE;
					TakeScreenShot();
					wait_for_triangle("");
					g_PSPEnableRendering = truE;
					s_bbDirty = truE;
				}
				oldButtonMask = 0;
			}

			if (pad.Buttons & PSP_CTRL_LEFT)
			{
				danzeff_x-=5;
				danzeff_moveTo(danzeff_x, danzeff_y);
			}
			else if (pad.Buttons & PSP_CTRL_RIGHT)
			{
				danzeff_x+=5;
				danzeff_moveTo(danzeff_x, danzeff_y);
			}
			else
			{
				int key = 0;
				key = danzeff_readInput(pad);
				if (key)
				{
					switch (key)
					{
						case '\n':
							key = KBD_ENTER;
							break;
						case 8:
							key = KBD_BS;
							break;
					}
					current_virtual_device->keyboard_handler(current_virtual_device, key, 0);
				}
			}
			if (sf_danzeffOn && danzeff_dirty())
			{
				s_bbDirty = truE;
			}
		}
		else
		{
			if  (pad.Lx < 128)
			{
				deltax = -(128 - pad.Lx)/g_PSPConfig.mouse_speed_factor;//30;
			}
			else
			{
				deltax = (pad.Lx - 128)/g_PSPConfig.mouse_speed_factor;
			}

			if  (pad.Ly < 128)
			{
				deltay = - (128 - pad.Ly)/g_PSPConfig.mouse_speed_factor;
			}
			else
			{
				deltay = (pad.Ly - 128)/g_PSPConfig.mouse_speed_factor;
			}

			if (pad.Buttons & PSP_CTRL_LTRIGGER)
			{
				s_BbRowOffset += deltay;
				s_BbColOffset += deltax;

				if (s_BbRowOffset < 0) s_BbRowOffset = 0;
				if (s_BbColOffset < 0) s_BbColOffset = 0;
				if (s_BbColOffset > (gu_data.virt_width - gu_data.display_width))  
					s_BbColOffset = gu_data.virt_width - gu_data.display_width;
				if (s_BbRowOffset > (gu_data.virt_height - gu_data.display_height)) 
					s_BbRowOffset = gu_data.virt_height - gu_data.display_height;

				s_bbDirty = truE;
			}
			else
			{
				fl	= B_MOVE;
				if (pad.Buttons & PSP_CTRL_CROSS)
				{
					fl = B_DRAG | B_LEFT;
				}
				else if (pad.Buttons & PSP_CTRL_TRIANGLE)
				{
					fl = B_DRAG | B_RIGHT;
				}
				/* calls handler */
				pspgu_mouse_move(deltax, deltay, fl);
			}


			if (latch.uiMake)
			{
				// Button Pressed
				oldButtonMask = latch.uiPress;
				if (oldButtonMask & PSP_CTRL_CROSS)
				{
					fl	= B_DOWN | B_LEFT;
					if (current_virtual_device) current_virtual_device->mouse_handler(current_virtual_device, mouse_x, mouse_y, fl);
				}
				else if (oldButtonMask & PSP_CTRL_TRIANGLE)
				{
					fl	= B_DOWN | B_RIGHT;
					if (current_virtual_device) current_virtual_device->mouse_handler(current_virtual_device, mouse_x, mouse_y, fl);
				}
			}
			else if (latch.uiBreak) /** Button Released */
			{
				if (IS_BUTTON_PRESSED(oldButtonMask, PSP_CTRL_SELECT | PSP_CTRL_CROSS))
				{
					pspDebugScreenInit();
					wifiChooseConnect();
					s_bbDirty = truE;
				}
				else if (IS_BUTTON_PRESSED(oldButtonMask, PSP_CTRL_SELECT | PSP_CTRL_SQUARE))
				{
					pspDebugScreenInit();
					cleanup_cookies();
					init_cookies();
					wait_for_triangle("Cookies saved to disk");
				}
				else if (IS_BUTTON_PRESSED(oldButtonMask, PSP_CTRL_DOWN))
				{
					//if (current_virtual_device) current_virtual_device->keyboard_handler(current_virtual_device, KBD_DEL, fl);
					if (current_virtual_device) current_virtual_device->keyboard_handler(current_virtual_device, KBD_DOWN, fl);
				}
				else if (IS_BUTTON_PRESSED(oldButtonMask, PSP_CTRL_DOWN | PSP_CTRL_RTRIGGER))
				{
					if (current_virtual_device) current_virtual_device->keyboard_handler(current_virtual_device, KBD_PAGE_DOWN, fl);
				}
				else if (IS_BUTTON_PRESSED(oldButtonMask, PSP_CTRL_UP))
				{
					//if (current_virtual_device) current_virtual_device->keyboard_handler(current_virtual_device, KBD_INS, fl);
					if (current_virtual_device) current_virtual_device->keyboard_handler(current_virtual_device, KBD_UP, fl);
				}
				else if (IS_BUTTON_PRESSED(oldButtonMask, PSP_CTRL_UP | PSP_CTRL_RTRIGGER))
				{
					if (current_virtual_device) current_virtual_device->keyboard_handler(current_virtual_device, KBD_PAGE_UP, fl);
				}
				else if (IS_BUTTON_PRESSED(oldButtonMask, PSP_CTRL_LEFT))
				{
					if (current_virtual_device) current_virtual_device->keyboard_handler(current_virtual_device, KBD_LEFT, fl);
				}
				else if (IS_BUTTON_PRESSED(oldButtonMask, PSP_CTRL_LEFT | PSP_CTRL_RTRIGGER))
				{
					if (current_virtual_device) current_virtual_device->keyboard_handler(current_virtual_device, '[', fl);
				}
				else if (IS_BUTTON_PRESSED(oldButtonMask, PSP_CTRL_RIGHT))
				{
					if (current_virtual_device) current_virtual_device->keyboard_handler(current_virtual_device, KBD_RIGHT, fl);
				}
				else if (IS_BUTTON_PRESSED(oldButtonMask, PSP_CTRL_RIGHT | PSP_CTRL_RTRIGGER))
				{
					if (current_virtual_device) current_virtual_device->keyboard_handler(current_virtual_device, ']', fl);
				}
				else if (IS_BUTTON_PRESSED(oldButtonMask, PSP_CTRL_CROSS))
				{
					fl	= B_UP | B_LEFT;
					if (current_virtual_device) current_virtual_device->mouse_handler(current_virtual_device, mouse_x, mouse_y, fl);
				}
				else if (IS_BUTTON_PRESSED(oldButtonMask, PSP_CTRL_SQUARE))
				{
					if (current_virtual_device) current_virtual_device->keyboard_handler(current_virtual_device, KBD_ESC, fl);
				}
				else if (IS_BUTTON_PRESSED(oldButtonMask, PSP_CTRL_TRIANGLE))
				{
					fl	= B_UP | B_RIGHT;
					if (current_virtual_device) current_virtual_device->mouse_handler(current_virtual_device, mouse_x, mouse_y, fl);
				}
				else if (IS_BUTTON_PRESSED(oldButtonMask, PSP_CTRL_CIRCLE))
				{
					if (current_virtual_device) current_virtual_device->keyboard_handler(current_virtual_device, KBD_ENTER, fl);
				}
				else if (IS_BUTTON_PRESSED(oldButtonMask, PSP_CTRL_START))
				{
					if (!danzeff_isinitialized())
					{
						danzeff_load();
					}
					//current_virtual_device->keyboard_handler(current_virtual_device, 'g', fl);
					if (danzeff_isinitialized())
					{
						danzeff_moveTo(danzeff_x, danzeff_y);
						//danzeff_render();
						sf_danzeffOn = 1;
						s_bbDirty = truE;
					}
					else
					{
						wait_for_triangle("Error loading danzeff OSK");
					}
				}
				else if (IS_BUTTON_PRESSED(oldButtonMask, PSP_CTRL_SELECT))
				{
					g_PSPEnableRendering = falsE;
					TakeScreenShot();
					wait_for_triangle("");
					g_PSPEnableRendering = truE;
					s_bbDirty = truE;
					//cls_redraw_all_terminals();
				}
				else if (IS_BUTTON_PRESSED(oldButtonMask, PSP_CTRL_START | PSP_CTRL_TRIANGLE))
				{
					static int mode = 2;
					switch(mode)
					{
					case 0:
						gu_data.virt_width  = 320;
						gu_data.virt_height = 182;
						break;
						
					case 1:
						gu_data.virt_width  = 480;
						gu_data.virt_height = 272;
						break;

					case 2:
						gu_data.virt_width  = 640;
						gu_data.virt_height = 362;
						break;

					case 3:
						gu_data.virt_width  = 720;
						gu_data.virt_height = 408;
						break;
					}

					mode = (mode+1)%4;
					
					gu_data.virt_sl_pixelpitch  = gu_data.virt_width;
					gu_data.virt_sl_bytepitch   = gu_data.virt_sl_pixelpitch * gu_data.virt_pixel_size;

					current_virtual_device->size.x2 = gu_data.virt_width;
					current_virtual_device->size.y2 = gu_data.virt_height;
					//pspgu_update_driver_param(newwidth, newhight);
					current_virtual_device->resize_handler(current_virtual_device);
				}

				if (s_Zoom == falsE)
				{
					if (oldButtonMask & PSP_CTRL_LTRIGGER)
					{
						if (oldButtonMask & PSP_CTRL_CROSS)
						{
							s_Zoom = truE;
						}
						s_bbDirty = truE;
					}
				}
				else
				{
					if (oldButtonMask & PSP_CTRL_LTRIGGER)
					{
						if (oldButtonMask & PSP_CTRL_CROSS)
						{
							s_Zoom = falsE;
						}
						s_bbDirty = truE;
					}
				}
				oldButtonMask = 0;
			}
		}
	}
	sceKernelDelayThread(1); /* yield */

	// Restart Input Timer
	install_timer(10, pspInputThread, NULL);
}

typedef struct {
	float s, t;
	unsigned int c;
	float x, y, z;
} VERT;

void render_thread(void *)
{
	SceCtrlData pad;
	int bb_to_fb_factor = 1;
	int bb_row_offset = 0, bb_col_offset = 0;
	unsigned int tex_color = 0xFFFFFFFF;
	float z = 10.0;

	for(;;)
	{
		if (g_PSPEnableRendering && s_bbDirty)
		{
			sceCtrlPeekBufferPositive(&pad, 1);

			if ((pad.Buttons & PSP_CTRL_LTRIGGER) || s_Zoom)
			{
				bb_to_fb_factor = 1;
				bb_row_offset = s_BbRowOffset;
				bb_col_offset = s_BbColOffset;
			}
			else
			{
				/** Display the whole backbuffer (shrinking if necessary..) */
				bb_to_fb_factor = g_PSPConfig.screen_zoom_factor;
				bb_row_offset = 0;
				bb_col_offset = 0;
			}

			{
				VERT* v = (VERT*)sceGuGetMemory(sizeof(VERT) * 2 * 1);
				gScreen->StartList();
				sceGuEnable( GU_TEXTURE_2D );
				sceGuTexMode(gu_data.virt_pixel_mode, 0,0,0);

				//q1
				sceGuTexImage(0,  //mipmaplevel
					512, 512,  //width, height
					gu_data.virt_sl_pixelpitch, //buffer width
					gu_data.virtbuffer);
				//topleft
				v[0].s = 0;//tex x
				v[0].t = 0;//tex y
				v[0].c = tex_color;
				v[0].x = 0; //scr x
				v[0].y = 0; //scr y
				v[0].z = 0.0f;
				//bottom right
				v[1].s = (gu_data.virt_width/2);
				v[1].t = (gu_data.virt_height/2);
				v[1].c = tex_color;
				v[1].x = 480/2;
				v[1].y = 272/2;
				v[1].z = z;
				sceGumDrawArray(GU_SPRITES, 
					GU_TEXTURE_32BITF | PSPVIRT_GU_TEXTURE_FORMAT | GU_VERTEX_32BITF | GU_TRANSFORM_2D,
					1 * 2, 0, v);
				gScreen->EndList();

				//q2
				v = (VERT *)sceGuGetMemory(sizeof(VERT) * 2 * 1);
				gScreen->StartList();
				sceGuTexImage(0,  //mipmaplevel
					512, 512,  //width, height
					gu_data.virt_sl_pixelpitch, //buffer width
					(virt_pixel_type *)gu_data.virtbuffer + (gu_data.virt_width/2));
				//topleft
				v[0].s = 0;//tex x
				v[0].t = 0;//tex y
				v[0].c = tex_color;
				v[0].x = 480/2; //scr x
				v[0].y = 0; //scr y
				v[0].z = 0.0f;
				//bottom right
				v[1].s = (gu_data.virt_width/2);
				v[1].t = (gu_data.virt_height/2);
				v[1].c = tex_color;
				v[1].x = 480;
				v[1].y = 272/2;
				v[1].z = z;
				sceGumDrawArray(GU_SPRITES, 
					GU_TEXTURE_32BITF | PSPVIRT_GU_TEXTURE_FORMAT | GU_VERTEX_32BITF | GU_TRANSFORM_2D,
					1 * 2, 0, v);
				gScreen->EndList();
				
				//q3
				v = (VERT *)sceGuGetMemory(sizeof(VERT) * 2 * 1);
				gScreen->StartList();
				sceGuTexImage(0,  //mipmaplevel
					512, 512,  //width, height
					gu_data.virt_sl_pixelpitch, //buffer width
					(virt_pixel_type *)gu_data.virtbuffer + (gu_data.virt_sl_pixelpitch * (gu_data.virt_height/2)));
				//topleft
				v[0].s = 0;//tex x
				v[0].t = 0;//tex y
				v[0].c = tex_color;
				v[0].x = 0; //scr x
				v[0].y = 272/2; //scr y
				v[0].z = 0.0f;
				//bottom right
				v[1].s = (gu_data.virt_width/2);
				v[1].t = (gu_data.virt_height/2);
				v[1].c = 0xFFFFFFFF;
				v[1].x = 480/2;
				v[1].y = 272;
				v[1].z = z;
				sceGumDrawArray(GU_SPRITES, 
					GU_TEXTURE_32BITF | PSPVIRT_GU_TEXTURE_FORMAT | GU_VERTEX_32BITF | GU_TRANSFORM_2D,
					1 * 2, 0, v);
				gScreen->EndList();

				//q4
				v = (VERT *)sceGuGetMemory(sizeof(VERT) * 2 * 1);
				gScreen->StartList();
				sceGuTexImage(0,  //mipmaplevel
					512, 512,  //width, height
					gu_data.virt_sl_pixelpitch, //buffer width
					(virt_pixel_type *)gu_data.virtbuffer + (gu_data.virt_sl_pixelpitch * (gu_data.virt_height/2)) + (gu_data.virt_width/2));
				//topleft
				v[0].s = 0;//tex x
				v[0].t = 0;//tex y
				v[0].c = 0xFFFFFFFF;
				v[0].x = 480/2; //scr x
				v[0].y = 272/2; //scr y
				v[0].z = 0.0f;
				//bottom right
				v[1].s = (gu_data.virt_width/2);
				v[1].t = (gu_data.virt_height/2);
				v[1].c = 0xFFFFFFFF;
				v[1].x = 480;
				v[1].y = 272;
				v[1].z = z;
				sceGumDrawArray(GU_SPRITES, 
					GU_TEXTURE_32BITF | PSPVIRT_GU_TEXTURE_FORMAT | GU_VERTEX_32BITF | GU_TRANSFORM_2D,
					1 * 2, 0, v);
				//EndList();

			}

			if (sf_danzeffOn)
			{
				//StartList();
				danzeff_render();
			}
			gScreen->EndList();

			/* flipping */
			sceDisplayWaitVblankStart();
			gScreen->SwapBuffers();

			s_bbDirty = falsE;
		}

		sceKernelDelayThread(1); /* yield */
	}

}

extern "C" void psp_reset_graphic_mode()
{
	/* (Re)set graphic mode */
	static int is_first_time = 1;
	unsigned int CacheMask = 0x44000000; /* uncached access */

	if (gu_data.guinitialized)
		sceGuTerm();

	gu_data.framesize = 
		gu_data.display_height * gu_data.display_sl_pixelpitch * gu_data.display_pixel_size;
	gu_data.virtframesize = 
		gu_data.virt_height * gu_data.virt_sl_pixelpitch * gu_data.virt_pixel_size;

	if (is_first_time)
	{
		/* GU Set up code: */
		psp_init();
		gu_data.virtbuffer    = (unsigned char*)(CacheMask | (unsigned int)valloc(gu_data.virtframesize));
		is_first_time = 0;
	}

	gScreen->StartList();
	sceGuEnable(GU_BLEND);
	sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
	sceGuEnable(GU_TEXTURE_2D);
	sceGuTexMode(gu_data.virt_pixel_mode, 0,0,0);
	sceGuTexImage(0,  //mipmaplevel
				 512, 512,  //width, height
				 gu_data.virt_sl_pixelpitch, //buffer width
				 gu_data.virtbuffer);
	sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
	sceGuTexEnvColor(0x0);
	sceGuTexOffset(0.0f, 0.0f);
	sceGuTexScale(1.0f / 480.0f, 1.0f / 272.0f);
	sceGuTexWrap(GU_CLAMP, GU_CLAMP);
	sceGuTexFilter( GU_LINEAR, GU_LINEAR );
	gScreen->EndList();
	gu_data.guinitialized = 1;
}

static unsigned char *pspgu_init_driver(unsigned char *param, unsigned char *ignore)
{
	struct stat st;
	pspgu_old_vd = NULL;
	ignore=ignore;
	pspgu_driver_param=NULL;
	if(param != NULL)
		pspgu_driver_param=stracpy(param);

	gu_data.virt_width  = 480;
	gu_data.virt_height = 272;
	gu_data.virt_sl_pixelpitch  = 512;
	gu_data.virt_pixel_mode = PSPVIRT_GU_PIXEL_FORMAT;
	gu_data.virt_pixel_size = PSPVIRT_PIXELSIZE;
	gu_data.virt_sl_bytepitch   = gu_data.virt_sl_pixelpitch * gu_data.virt_pixel_size;

	gu_data.display_width  = 480;
	gu_data.display_height = 272;
	gu_data.display_sl_pixelpitch  = 512;
	gu_data.display_pixel_mode = PSPSCR_GU_PIXEL_FORMAT;
	gu_data.display_pixel_size = PSPSCR_PIXELSIZE;
	gu_data.display_sl_bytepitch   = gu_data.display_sl_pixelpitch * gu_data.display_pixel_size;

	gu_data.guinitialized = 0;

	border_left = border_right = border_top = border_bottom = 0;

	pspgu_console = st.st_rdev & 0xff;

	pspgu_driver.x=gu_data.virt_width;
	pspgu_driver.y=gu_data.virt_height;
	pspgu_colors=1<<(gu_data.virt_pixel_size*8);

	if (init_virtual_devices(&pspgu_driver, NUMBER_OF_DEVICES))
	{
		if(pspgu_driver_param) { mem_free(pspgu_driver_param); pspgu_driver_param=NULL; }
		return stracpy((unsigned char*)"Allocation of virtual devices failed.\n");
	}

	/* Initialize GU */
	psp_reset_graphic_mode();



#if (PSPVIRT_PIXEL_FORMAT_NUMBER==4444)/* 387 = ABGR 4444 */
	pspgu_driver.depth = 387;
#endif

#if (PSPVIRT_PIXEL_FORMAT_NUMBER==5650)/* 131 = BGR 565 */
	pspgu_driver.depth = 131;
#endif

#if (PSPVIRT_PIXEL_FORMAT_NUMBER==8888)/* 32bpp (ABGR 8888) */
	pspgu_driver.depth=gu_data.virt_pixel_size&7;
	pspgu_driver.depth|=(24/*pspgu_bits_pp*/&31)<<3;
	if (htons (0x1234) == 0x1234) pspgu_driver.depth |= 0x100;
#endif

	pspgu_driver.get_color=get_color_fn(pspgu_driver.depth);

	/* Pass info to Danzeff */
	danzeff_set_screen(PSPSCR_PIXELSIZE, PSPSCR_GU_PIXEL_FORMAT, PSPSCR_GU_TEXTURE_FORMAT);

	/* mouse */
	mouse_buffer=(unsigned char*)mem_alloc(gu_data.virt_pixel_size*arrow_area);
	background_buffer=(unsigned char*)mem_alloc(gu_data.virt_pixel_size*arrow_area);
	new_background_buffer=(unsigned char*)mem_alloc(gu_data.virt_pixel_size*arrow_area);
	background_x=mouse_x=gu_data.virt_width>>1;
	background_y=mouse_y=gu_data.virt_height>>1;
	mouse_black=pspgu_driver.get_color(0);
	mouse_white=pspgu_driver.get_color(0xffffff);
	mouse_graphics_device=pspgu_driver.init_device();
	virtual_devices[0] = NULL;
	global_mouse_hidden=1;

	/*if (border_left | border_top | border_right | border_bottom) */
	show_mouse();
	s_bbDirty = truE;

	/* Start Render Thread */
	{
		pthread_t pthid;
		pthread_attr_t pthattr;
		struct sched_param shdparam;
		pthread_attr_init(&pthattr);
		shdparam.sched_policy = SCHED_OTHER;
		shdparam.sched_priority = 45; /* It has to be 45 so the PSPRadio plugin version works correctly */
		pthread_attr_setschedparam(&pthattr, &shdparam);
		pthread_create(&pthid, &pthattr, render_thread, NULL);
	}

	/* Start Input Timer */
	install_timer(10, pspInputThread, NULL);

	return NULL;
}

static void pspgu_shutdown_driver(void)
{
	mem_free(mouse_buffer);
	mem_free(background_buffer);
	mem_free(new_background_buffer);
	pspgu_driver.shutdown_device(mouse_graphics_device);

	shutdown_virtual_devices();
	if(pspgu_driver_param) mem_free(pspgu_driver_param);
}


static unsigned char *pspgu_get_driver_param(void)
{
    return pspgu_driver_param;
}


/* Return value:        0 alloced on heap
 *                      1 alloced in vidram
 *                      2 alloced in X server shm
 */
static int pspgu_get_empty_bitmap(struct bitmap *dest)
{
	if (dest->x && (unsigned)dest->x * (unsigned)dest->y / (unsigned)dest->x != (unsigned)dest->y) overalloc();
	if ((unsigned)dest->x * (unsigned)dest->y > (unsigned)MAXINT / gu_data.virt_pixel_size) overalloc();
	dest->data=mem_alloc(dest->x*dest->y*gu_data.virt_pixel_size);
	dest->skip=dest->x*gu_data.virt_pixel_size;
	dest->flags=0;
	return 0;
}

static void pspgu_register_bitmap(struct bitmap *bmp)
{
	s_bbDirty = truE;
}

static void pspgu_unregister_bitmap(struct bitmap *bmp)
{
	mem_free(bmp->data);
}

static void *pspgu_prepare_strip(struct bitmap *bmp, int top, int lines)
{
	return ((char *)bmp->data)+bmp->skip*top;
}

static void pspgu_commit_strip(struct bitmap *bmp, int top, int lines)
{
	s_bbDirty = truE;
	return;
}

void pspgu_draw_bitmap(struct graphics_device *dev,struct bitmap* hndl, int x, int y)
{
	unsigned char *scr_start;

	CLIP_PREFACE

	scr_start=(unsigned char*)(gu_data.virtbuffer+y*gu_data.virt_sl_bytepitch+x*gu_data.virt_pixel_size);
	for(;ys;ys--){
		memcpy(scr_start,data,xs*gu_data.virt_pixel_size);
		data+=hndl->skip;
		scr_start+=gu_data.virt_sl_bytepitch;
	}
	END_GR
	s_bbDirty = truE;
}

static void pspgu_draw_bitmaps(struct graphics_device *dev, struct bitmap **hndls, int n, int x, int y)
{
	TEST_INACTIVITY

	if (x>=gu_data.virt_width||y>gu_data.virt_height) return;
	while(x+(*hndls)->x<=0&&n){
		x+=(*hndls)->x;
		n--;
		hndls++;
	}
	while(n&&x<=gu_data.virt_width){
		pspgu_draw_bitmap(dev, *hndls, x, y);
		x+=(*hndls)->x;
		n--;
		hndls++;
	}
}

static void pspgu_fill_area(struct graphics_device *dev, int left, int top, int right, int bottom, long color)
{
	unsigned char *dest;
	int y;

	FILL_CLIP_PREFACE

	dest=gu_data.virtbuffer+top*gu_data.virt_sl_bytepitch+left*gu_data.virt_pixel_size;
	for (y=bottom-top;y;y--){
		pixel_set(dest,(right-left)*gu_data.virt_pixel_size,&color);
		dest+=gu_data.virt_sl_bytepitch;
	}
	END_GR
	s_bbDirty = truE;
}

static void pspgu_draw_hline(struct graphics_device *dev, int left, int y, int right, long color)
{
	unsigned char *dest;
	HLINE_CLIP_PREFACE

	dest=gu_data.virtbuffer+y*gu_data.virt_sl_bytepitch+left*gu_data.virt_pixel_size;
	pixel_set(dest,(right-left)*gu_data.virt_pixel_size,&color);
	END_GR
	s_bbDirty = truE;
}

static void pspgu_draw_vline(struct graphics_device *dev, int x, int top, int bottom, long color)
{
	unsigned char *dest;
	int y;
	VLINE_CLIP_PREFACE

	dest=gu_data.virtbuffer+top*gu_data.virt_sl_bytepitch+x*gu_data.virt_pixel_size;
	for (y=(bottom-top);y;y--){
		memcpy(dest,&color,gu_data.virt_pixel_size);
		dest+=gu_data.virt_sl_bytepitch;
	}
	END_GR
	s_bbDirty = truE;
}

static int pspgu_hscroll(struct graphics_device *dev, struct rect_set **ignore, int sc)
{
	unsigned char *dest, *src;
	int y;
	int len;
	HSCROLL_CLIP_PREFACE

	ignore=NULL;
	if (sc>0){
		len=(dev->clip.x2-dev->clip.x1-sc)*gu_data.virt_pixel_size;
		src=gu_data.virtbuffer+gu_data.virt_sl_bytepitch*dev->clip.y1+dev->clip.x1*gu_data.virt_pixel_size;
		dest=src+sc*gu_data.virt_pixel_size;
		for (y=dev->clip.y2-dev->clip.y1;y;y--){
			memmove(dest,src,len);
			dest+=gu_data.virt_sl_bytepitch;
			src+=gu_data.virt_sl_bytepitch;
		}
	}else{
		len=(dev->clip.x2-dev->clip.x1+sc)*gu_data.virt_pixel_size;
		dest=gu_data.virtbuffer+gu_data.virt_sl_bytepitch*dev->clip.y1+dev->clip.x1*gu_data.virt_pixel_size;
		src=dest-sc*gu_data.virt_pixel_size;
		for (y=dev->clip.y2-dev->clip.y1;y;y--){
			memmove(dest,src,len);
			dest+=gu_data.virt_sl_bytepitch;
			src+=gu_data.virt_sl_bytepitch;
		}
	}
	END_GR
	s_bbDirty = truE;
	return 1;
}

static int pspgu_vscroll(struct graphics_device *dev, struct rect_set **ignore, int sc)
{
	unsigned char *dest, *src;
	int y;
	int len;

	VSCROLL_CLIP_PREFACE

	ignore=NULL;
	len=(dev->clip.x2-dev->clip.x1)*gu_data.virt_pixel_size;
	if (sc>0){
		/* Down */
		dest=gu_data.virtbuffer+(dev->clip.y2-1)*gu_data.virt_sl_bytepitch+dev->clip.x1*gu_data.virt_pixel_size;
		src=dest-gu_data.virt_sl_bytepitch*sc;
		for (y=dev->clip.y2-dev->clip.y1-sc;y;y--){
			memcpy(dest,src,len);
			dest-=gu_data.virt_sl_bytepitch;
			src-=gu_data.virt_sl_bytepitch;
		}
	}else{
		/* Up */
		dest=gu_data.virtbuffer+dev->clip.y1*gu_data.virt_sl_bytepitch+dev->clip.x1*gu_data.virt_pixel_size;
		src=dest-gu_data.virt_sl_bytepitch*sc;
		for (y=dev->clip.y2-dev->clip.y1+sc;y;y--){
			memcpy(dest,src,len);
			dest+=gu_data.virt_sl_bytepitch;
			src+=gu_data.virt_sl_bytepitch;
		}
	}
	END_GR
	s_bbDirty = truE;
	return 1;
}

static void pspgu_set_clip_area(struct graphics_device *dev, struct rect *r)
{
	memcpy(&dev->clip, r, sizeof(struct rect));
	if (dev->clip.x1>=dev->clip.x2||dev->clip.y2<=dev->clip.y1||dev->clip.y2<=0||dev->clip.x2<=0||dev->clip.x1>=gu_data.virt_width
			||dev->clip.y1>=gu_data.virt_height){
		/* Empty region */
		dev->clip.x1=dev->clip.x2=dev->clip.y1=dev->clip.y2=0;
	}else{
		if (dev->clip.x1<0) dev->clip.x1=0;
		if (dev->clip.x2>gu_data.virt_width) dev->clip.x2=gu_data.virt_width;
		if (dev->clip.y1<0) dev->clip.y1=0;
		if (dev->clip.y2>gu_data.virt_height) dev->clip.y2=gu_data.virt_height;
	}
}

static int pspgu_block(struct graphics_device *dev)
{
	if (pspgu_old_vd) return 1;
	pspgu_old_vd = current_virtual_device;
	current_virtual_device=NULL;
	return 0;
}

static void pspgu_unblock(struct graphics_device *dev)
{
	current_virtual_device = pspgu_old_vd;
	pspgu_old_vd = NULL;
	//if (border_left | border_top | border_right | border_bottom) 
	//	memset(pspgu_mem,0,pspgu_mem_size);
	if (current_virtual_device) current_virtual_device->redraw_handler(current_virtual_device
			,&current_virtual_device->size);
}


struct graphics_driver pspgu_driver={
	(unsigned char*)"pspgu",
	pspgu_init_driver,
	init_virtual_device,
	shutdown_virtual_device,
	pspgu_shutdown_driver,
	pspgu_get_driver_param,
	pspgu_get_empty_bitmap,
	/*pspgu_get_filled_bitmap,*/
	pspgu_register_bitmap,
	pspgu_prepare_strip,
	pspgu_commit_strip,
	pspgu_unregister_bitmap,
	pspgu_draw_bitmap,
	pspgu_draw_bitmaps,
	NULL,	/* pspgu_get_color */
	pspgu_fill_area,
	pspgu_draw_hline,
	pspgu_draw_vline,
	pspgu_hscroll,
	pspgu_vscroll,
	pspgu_set_clip_area,
	pspgu_block,
	pspgu_unblock,
	NULL,	/* set_title */
	NULL, /* exec */
	0,				/* depth (filled in pspgu_init_driver function) */
	0, 0,				/* size (in X is empty) */
	GD_DONT_USE_SCROLL|GD_NEED_CODEPAGE,		/* flags */
	0,				/* codepage */
	NULL,				/* shell */
};


#endif /* GRDRV_PSPGU */
