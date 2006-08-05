/* framebuffer.c
 * Linux framebuffer code
 * (c) 2002 Petr 'Brain' Kulhavy
 * This file is a part of the Links program, released under GPL.
 */

#include "cfg.h"

#ifdef GRDRV_PSPGU

#ifdef TEXT
#undef TEXT
#endif

#include "links.h"

#include <pthread.h>
#include <pspctrl.h>
#include <pspgu.h>


#include <signal.h>

#include "arrow.inc"


#include <pspdisplay.h>
#define printf pspDebugScreenPrintf
#define BB_TO_FB_FACTOR 2
#define PSP_MOUSE_ACCEL_CONST 30 /* default 30 */
static volatile int sf_danzeffOn = 0;
static volatile int s_BbRowOffset = 0, s_BbColOffset = 0;
static volatile int s_bbDirty = falsE;
static volatile int s_Zoom = falsE;

int pspgu_console;

struct itrm *pspgu_kbd;

struct graphics_device *pspgu_old_vd;

char *pspgu_mem, *pspgu_vmem, *psp_fb;
int pspgu_mem_size,pspgu_linesize,pspgu_bits_pp,pspgu_pixelsize;
int pspgu_xsize,pspgu_ysize;
int border_left, border_right, border_top, border_bottom;
int pspgu_colors;

void pspgu_draw_bitmap(struct graphics_device *dev,struct bitmap* hndl, int x, int y);

static unsigned char *pspgu_driver_param;
struct graphics_driver pspgu_driver;
volatile int pspgu_active=1;

static volatile int in_gr_operation;

/* mouse */
static int mouse_x, mouse_y;		/* mouse pointer coordinates */
static int mouse_black, mouse_white;
static int background_x, background_y; /* Where was the mouse background taken from */
static unsigned char *mouse_buffer, *background_buffer, *new_background_buffer;
static struct graphics_device *mouse_graphics_device;
static int global_mouse_hidden;


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
        char *data=hndl->data;\
\
 	TEST_INACTIVITY\
        if (x>=dev->clip.x2||x+xs<=dev->clip.x1) return;\
        if (y>=dev->clip.y2||y+ys<=dev->clip.y1) return;\
        if (x+xs>dev->clip.x2) xs=dev->clip.x2-x;\
        if (y+ys>dev->clip.y2) ys=dev->clip.y2-y;\
        if (dev->clip.x1-x>0){\
                xs-=(dev->clip.x1-x);\
                data+=pspgu_pixelsize*(dev->clip.x1-x);\
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


/* n is in bytes. dest must begin on pixel boundary. If n is not a whole number of pixels, rounding is
 * performed downwards.
 */
static inline void pixel_set(unsigned char *dest, int n,void * pattern)
{
#if PSP_PIXEL_FORMAT == PSP_DISPLAY_PIXEL_FORMAT_8888
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
#else
	int a;
	
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
#endif	
}

static void redraw_mouse(void);

static void pspgu_mouse_move(int dx, int dy, int fl)
{
	struct event ev;
	mouse_x += dx;
	mouse_y += dy;
	ev.ev = EV_MOUSE;
	if (mouse_x >= pspgu_xsize) mouse_x = pspgu_xsize - 1;
	if (mouse_y >= pspgu_ysize) mouse_y = pspgu_ysize - 1;
	if (mouse_x < 0) mouse_x = 0;
	if (mouse_y < 0) mouse_y = 0;
	ev.x = mouse_x;
	ev.y = mouse_y;
	ev.b = B_MOVE;
	if (!current_virtual_device) return;
	if (current_virtual_device->mouse_handler) current_virtual_device->mouse_handler(current_virtual_device, ev.x, ev.y, fl/*ev.b*/);
	redraw_mouse();
}

#define mouse_getscansegment(buf,x,y,w) memcpy(buf,pspgu_vmem+y*pspgu_linesize+x*pspgu_pixelsize,w)
#define mouse_drawscansegment(ptr,x,y,w) memcpy(pspgu_vmem+y*pspgu_linesize+x*pspgu_pixelsize,ptr,w);

/* Flushes the background_buffer onscreen where it was originally taken from. */
static void place_mouse_background(void)
{
	struct bitmap bmp;

	bmp.x=arrow_width;
	bmp.y=arrow_height;
	bmp.skip=arrow_width*pspgu_pixelsize;
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

	skip=arrow_width*pspgu_pixelsize;

	x=mouse_x;
	y=mouse_y;

	width=pspgu_pixelsize*(arrow_width+x>pspgu_xsize?pspgu_xsize-x:arrow_width);
	height=arrow_height+y>pspgu_ysize?pspgu_ysize-y:arrow_height;

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
				memcpy (mouse_ptr, &mouse_black, pspgu_pixelsize);
			else if (reg1&mask)
				memcpy (mouse_ptr, &mouse_white, pspgu_pixelsize);
			mouse_ptr+=pspgu_pixelsize;
		}
	}
	s_bbDirty = truE;
}

static void place_mouse(void)
{
	struct bitmap bmp;

	bmp.x=arrow_width;
	bmp.y=arrow_height;
	bmp.skip=arrow_width*pspgu_pixelsize;
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
	memcpy(mouse_buffer,background_buffer,pspgu_pixelsize*arrow_area);
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
	int bmpixelsizeL=pspgu_pixelsize;
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
	int skip=arrow_width*pspgu_pixelsize;
	int background_length,mouse_length;
	unsigned char *mouse_ptr=mouse_buffer,*background_ptr=background_buffer;
	int yend;

	if (mouse_bottom>pspgu_ysize) mouse_bottom=pspgu_ysize;
	if (background_bottom>pspgu_ysize) background_bottom=pspgu_ysize;

	/* Let's do the top part */
	if (background_top<mouse_top){
		/* Draw the background */
		background_length=background_right>pspgu_xsize?pspgu_xsize-background_left
			:arrow_width;
		for (;background_top<mouse_top;background_top++){
			mouse_drawscansegment(background_ptr,background_left
				,background_top,background_length*pspgu_pixelsize);
			background_ptr+=skip;
		}

	}else if (background_top>mouse_top){
		/* Draw the mouse */
		mouse_length=mouse_right>pspgu_xsize
			?pspgu_xsize-mouse_left:arrow_width;
		for (;mouse_top<background_top;mouse_top++){
			mouse_drawscansegment(mouse_ptr,mouse_left,mouse_top,mouse_length*pspgu_pixelsize);
			mouse_ptr+=skip;
		}
	}

	/* Let's do the middle part */
	yend=mouse_bottom<background_bottom?mouse_bottom:background_bottom;
	if (background_left<mouse_left){
		/* Draw background, mouse */
		mouse_length=mouse_right>pspgu_xsize?pspgu_xsize-mouse_left:arrow_width;
		for (;mouse_top<yend;mouse_top++){
			mouse_drawscansegment(background_ptr,background_left,mouse_top
				,(mouse_left-background_left)*pspgu_pixelsize);
			mouse_drawscansegment(mouse_ptr,mouse_left,mouse_top,mouse_length*pspgu_pixelsize);
			mouse_ptr+=skip;
			background_ptr+=skip;
		}

	}else{
		int l1, l2, l3;

		/* Draw mouse, background */
		mouse_length=mouse_right>pspgu_xsize?pspgu_xsize-mouse_left:arrow_width;
		background_length=background_right-mouse_right;
		if (background_length+mouse_right>pspgu_xsize)
			background_length=pspgu_xsize-mouse_right;
		l1=mouse_length*pspgu_pixelsize;
		l2=(mouse_right-background_left)*pspgu_pixelsize;
		l3=background_length*pspgu_pixelsize;
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
		mouse_length=mouse_right>pspgu_xsize?pspgu_xsize-mouse_left
			:arrow_width;
		for (;background_bottom<mouse_bottom;background_bottom++){
			mouse_drawscansegment(mouse_ptr,mouse_left,background_bottom
				,mouse_length*pspgu_pixelsize);
			mouse_ptr+=skip;
		}
	}else{
		/* Draw background */
		background_length=background_right>pspgu_xsize?pspgu_xsize-background_left
			:arrow_width;
		for (;mouse_bottom<background_bottom;mouse_bottom++){
			mouse_drawscansegment(background_ptr,background_left,mouse_bottom
				,background_length*pspgu_pixelsize);
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
	memcpy(new_background_buffer,mouse_buffer,pspgu_pixelsize*arrow_area);
	new_background_x=mouse_x;
	new_background_y=mouse_y;
	render_mouse_arrow();
	place_mouse_composite();
	memcpy(background_buffer,new_background_buffer,pspgu_pixelsize*arrow_area);
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
				mouse_buffer,arrow_area*pspgu_pixelsize);
			render_mouse_arrow();
			hide_mouse();
			place_mouse();
			memcpy(background_buffer,new_background_buffer
				,arrow_area*pspgu_pixelsize);
			background_x=mouse_x;
			background_y=mouse_y;
		}
	}
}

static void pspgu_switch_signal(void *data)
{
#ifndef PSP
	struct vt_stat st;
	struct rect r;
	int signal=(int)(unsigned long)data;

	switch(signal)
	{
		case SIG_REL: /* release */
		pspgu_active=0;
		if (!in_gr_operation)ioctl(TTY,VT_RELDISP,1);
		break;

		case SIG_ACQ: /* acq */
		if (ioctl(TTY,VT_GETSTATE,&st)) return;
		if (st.v_active != pspgu_console) return;
		pspgu_active=1;
		ioctl(TTY,VT_RELDISP,VT_ACKACQ);
		r.x1=0;
		r.y1=0;
		r.x2=pspgu_xsize;
		r.y2=pspgu_ysize;
		if (border_left | border_top | border_right | border_bottom) memset(pspgu_mem,0,pspgu_mem_size);
		if (current_virtual_device) current_virtual_device->redraw_handler(current_virtual_device,&r);
		break;
	}
#endif
}

void pspInputThread()
{
	//for(;;)
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
						//cls_redraw_all_terminals();
					}
					else if (oldButtonMask & PSP_CTRL_SELECT)
					{
						g_PSPEnableRendering = falsE;
						TakeScreenShot();
						wait_for_triangle("");
						g_PSPEnableRendering = truE;
						s_bbDirty = truE;
						//cls_redraw_all_terminals();
					}
					oldButtonMask = 0;
				}

				if (pad.Buttons & PSP_CTRL_LEFT)
				{
					danzeff_x-=5;
					danzeff_moveTo(danzeff_x, danzeff_y);
					//cls_redraw_all_terminals();
				}
				else if (pad.Buttons & PSP_CTRL_RIGHT)
				{
					danzeff_x+=5;
					danzeff_moveTo(danzeff_x, danzeff_y);
					//cls_redraw_all_terminals();
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
					deltax = -(128 - pad.Lx)/PSP_MOUSE_ACCEL_CONST;//30;
				}
				else
				{
					deltax = (pad.Lx - 128)/PSP_MOUSE_ACCEL_CONST;
				}

				if  (pad.Ly < 128)
				{
					deltay = - (128 - pad.Ly)/PSP_MOUSE_ACCEL_CONST;
				}
				else
				{
					deltay = (pad.Ly - 128)/PSP_MOUSE_ACCEL_CONST;
				}

				if (pad.Buttons & PSP_CTRL_LTRIGGER)
				{
					s_BbRowOffset += deltay;
					s_BbColOffset += deltax;

					if (s_BbRowOffset < 0) s_BbRowOffset = 0;
					if (s_BbColOffset < 0) s_BbColOffset = 0;
					if (s_BbColOffset > (BB_TO_FB_FACTOR*PSP_SCREEN_WIDTH - PSP_SCREEN_WIDTH))  
						s_BbColOffset = (BB_TO_FB_FACTOR*PSP_SCREEN_WIDTH - PSP_SCREEN_WIDTH);
					if (s_BbRowOffset > (BB_TO_FB_FACTOR*PSP_SCREEN_HEIGHT - PSP_SCREEN_HEIGHT)) 
						s_BbRowOffset = (BB_TO_FB_FACTOR*PSP_SCREEN_HEIGHT - PSP_SCREEN_HEIGHT);

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
					if (oldButtonMask & PSP_CTRL_SELECT)
					{
						if (oldButtonMask & PSP_CTRL_CROSS)
						{
							pspDebugScreenInit();
							wifiChooseConnect();
							//cls_redraw_all_terminals();
						}
						if (oldButtonMask & PSP_CTRL_SQUARE)
						{
							pspDebugScreenInit();
							cleanup_cookies();
							init_cookies();
							wait_for_triangle("Cookies saved to disk");
						}
					}
					else if (oldButtonMask & PSP_CTRL_DOWN)
					{
						if (oldButtonMask & PSP_CTRL_RTRIGGER)
						{
							if (current_virtual_device) current_virtual_device->keyboard_handler(current_virtual_device, KBD_PAGE_DOWN, fl);
						}
						else
						{
							//if (current_virtual_device) current_virtual_device->keyboard_handler(current_virtual_device, KBD_DEL, fl);
							if (current_virtual_device) current_virtual_device->keyboard_handler(current_virtual_device, KBD_DOWN, fl);
						}
					}
					else if (oldButtonMask & PSP_CTRL_UP)
					{
						if (oldButtonMask & PSP_CTRL_RTRIGGER)
						{
							if (current_virtual_device) current_virtual_device->keyboard_handler(current_virtual_device, KBD_PAGE_UP, fl);
						}
						else
						{
							//if (current_virtual_device) current_virtual_device->keyboard_handler(current_virtual_device, KBD_INS, fl);
							if (current_virtual_device) current_virtual_device->keyboard_handler(current_virtual_device, KBD_UP, fl);
						}
					}
					else if (oldButtonMask & PSP_CTRL_LEFT)
					{
						if (oldButtonMask & PSP_CTRL_RTRIGGER)
						{
							if (current_virtual_device) current_virtual_device->keyboard_handler(current_virtual_device, '[', fl);
						}
						else
						{
							if (current_virtual_device) current_virtual_device->keyboard_handler(current_virtual_device, KBD_LEFT, fl);
						}
					}
					else if (oldButtonMask & PSP_CTRL_RIGHT)
					{
						if (oldButtonMask & PSP_CTRL_RTRIGGER)
						{
							if (current_virtual_device) current_virtual_device->keyboard_handler(current_virtual_device, ']', fl);
						}
						else
						{
							if (current_virtual_device) current_virtual_device->keyboard_handler(current_virtual_device, KBD_RIGHT, fl);
						}
					}
					//else if (oldButtonMask & PSP_CTRL_LTRIGGER)
					//{
					//}
					//else if (oldButtonMask & PSP_CTRL_RTRIGGER)
					//{
					//}
					else if (oldButtonMask & PSP_CTRL_CROSS)
					{
						fl	= B_UP | B_LEFT;
						if (current_virtual_device) current_virtual_device->mouse_handler(current_virtual_device, mouse_x, mouse_y, fl);
					}
					else if (oldButtonMask & PSP_CTRL_SQUARE)
					{
						if (current_virtual_device) current_virtual_device->keyboard_handler(current_virtual_device, KBD_ESC, fl);
					}
					else if (oldButtonMask & PSP_CTRL_TRIANGLE)
					{
						fl	= B_UP | B_RIGHT;
						if (current_virtual_device) current_virtual_device->mouse_handler(current_virtual_device, mouse_x, mouse_y, fl);
					}
					else if (oldButtonMask & PSP_CTRL_CIRCLE)
					{
						if (current_virtual_device) current_virtual_device->keyboard_handler(current_virtual_device, KBD_ENTER, fl);
					}
					else if (oldButtonMask & PSP_CTRL_START)
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
					else if (oldButtonMask & PSP_CTRL_SELECT)
					{
						g_PSPEnableRendering = falsE;
						TakeScreenShot();
						wait_for_triangle("");
						g_PSPEnableRendering = truE;
						s_bbDirty = truE;
						//cls_redraw_all_terminals();
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
		//ceKernelDelayThread(11*1000); /* Wait 1ms */
		sceKernelDelayThread(1); /* yield */

	}
}


void render_thread()
{
#if PSP_PIXEL_FORMAT == PSP_DISPLAY_PIXEL_FORMAT_8888
	int *pBb = pspgu_mem; /* Back buffer */
	int *pFb = psp_fb;    /* Front/Frame buffer */
#else
	short *pBb = pspgu_mem; /* Back buffer */
	short *pFb = psp_fb;    /* Front/Frame buffer */
#endif
	SceCtrlData pad;
	int fbRow, fbCol;
	int bbRow, bbCol;
	int bbLineSize = PSP_SCREEN_WIDTH*BB_TO_FB_FACTOR;
	int fbLineSize = PSP_LINE_SIZE;
	int fbMult, bbMult;
	int bb_to_fb_factor = 1;
	int bb_row_offset = 0, bb_col_offset = 0;

	for(;;)
	{
		sceDisplayWaitVblankStart();

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
				bb_to_fb_factor = BB_TO_FB_FACTOR;
				bb_row_offset = 0;
				bb_col_offset = 0;
			}

			for (fbRow = 0, bbRow = bb_row_offset; fbRow < PSP_SCREEN_HEIGHT; fbRow++, bbRow+=bb_to_fb_factor)
			{
				fbMult = fbRow*fbLineSize;
				bbMult = bbRow*bbLineSize;
				for (fbCol = 0, bbCol = bb_col_offset; fbCol < PSP_SCREEN_WIDTH; fbCol++, bbCol+=bb_to_fb_factor)
				{
				   pFb[fbMult+fbCol] = pBb[bbMult+bbCol];
				}
			}
	
			if (sf_danzeffOn)
			{
				danzeff_render();
			}

			s_bbDirty = falsE;
		}

		sceKernelDelayThread(1); /* yield */
	}

}

void psp_reset_graphic_mode()
{
	/* (Re)set graphic mode */
	sceDisplaySetMode(0, PSP_SCREEN_WIDTH, PSP_SCREEN_HEIGHT);
	sceDisplaySetFrameBuf((void *) psp_fb, PSP_LINE_SIZE, PSP_PIXEL_FORMAT, 1);
}

static unsigned char *pspgu_init_driver(unsigned char *param, unsigned char *ignore)
{
	unsigned char *e;
	struct stat st;
	pspgu_old_vd = NULL;
	ignore=ignore;
	pspgu_driver_param=NULL;
	if(param != NULL)
		pspgu_driver_param=stracpy(param);

	border_left = border_right = border_top = border_bottom = 0;

	pspgu_console = st.st_rdev & 0xff;

	pspgu_xsize=PSP_SCREEN_WIDTH*BB_TO_FB_FACTOR;
	pspgu_ysize=PSP_SCREEN_HEIGHT*BB_TO_FB_FACTOR;

#if PSP_PIXEL_FORMAT == PSP_DISPLAY_PIXEL_FORMAT_8888
	pspgu_bits_pp=32;
	pspgu_pixelsize=4;
#else
	pspgu_bits_pp=16;
	pspgu_pixelsize=2;
#endif
	pspgu_driver.x=pspgu_xsize;
	pspgu_driver.y=pspgu_ysize;
	pspgu_colors=1<<pspgu_bits_pp;

	pspgu_linesize=PSP_SCREEN_WIDTH*pspgu_pixelsize*BB_TO_FB_FACTOR;
	pspgu_mem_size=pspgu_linesize * pspgu_ysize;

	if (init_virtual_devices(&pspgu_driver, NUMBER_OF_DEVICES))
	{
		if(pspgu_driver_param) { mem_free(pspgu_driver_param); pspgu_driver_param=NULL; }
		return stracpy("Allocation of virtual devices failed.\n");
	}

	/* Mikulas: nechodi to na sparcu */
	if (pspgu_mem_size < pspgu_linesize * pspgu_ysize)
	{
		shutdown_virtual_devices();
		if(pspgu_driver_param) { mem_free(pspgu_driver_param); pspgu_driver_param=NULL; }
		return stracpy("Nonlinear mapping of graphics memory not supported.\n");
	}

	/* Place vram in uncached memory */
	psp_fb = (u32 *) (0x40000000 | (u32) sceGeEdramGetAddr());
	psp_reset_graphic_mode();

	pspgu_mem = (char *) malloc(pspgu_mem_size);

	pspgu_vmem = pspgu_mem + border_left * pspgu_pixelsize + border_top * pspgu_linesize;
	///pspgu_driver.depth=pspgu_pixelsize&7;
	//pspgu_driver.depth|=(24/*pspgu_bits_pp*/&31)<<3;
	///pspgu_driver.depth|=(16/*pspgu_bits_pp*/&31)<<3;
	///if (htons (0x1234) == 0x1234) pspgu_driver.depth |= 0x100;

#if PSP_PIXEL_FORMAT == PSP_DISPLAY_PIXEL_FORMAT_8888
	pspgu_driver.depth=pspgu_pixelsize&7;
	pspgu_driver.depth|=(24/*pspgu_bits_pp*/&31)<<3;
	if (htons (0x1234) == 0x1234) pspgu_driver.depth |= 0x100;
#else
	pspgu_driver.depth = 131;
#endif

	pspgu_driver.get_color=get_color_fn(pspgu_driver.depth);

	/* Pass VRAM info to Danzeff */
	danzeff_set_screen(psp_fb, PSP_LINE_SIZE, pspgu_ysize, pspgu_pixelsize);

	/* mouse */
	mouse_buffer=mem_alloc(pspgu_pixelsize*arrow_area);
	background_buffer=mem_alloc(pspgu_pixelsize*arrow_area);
	new_background_buffer=mem_alloc(pspgu_pixelsize*arrow_area);
	background_x=mouse_x=pspgu_xsize>>1;
	background_y=mouse_y=pspgu_ysize>>1;
	mouse_black=pspgu_driver.get_color(0);
	mouse_white=pspgu_driver.get_color(0xffffff);
	mouse_graphics_device=pspgu_driver.init_device();
	virtual_devices[0] = NULL;
	global_mouse_hidden=1;

	if (border_left | border_top | border_right | border_bottom) memset(pspgu_mem,0,pspgu_mem_size);

	show_mouse();
	s_bbDirty = truE;

	if (1)
	{
		pthread_t pthid;
		pthread_attr_t pthattr;
		struct sched_param shdparam;
		pthread_attr_init(&pthattr);
		shdparam.sched_policy = SCHED_OTHER;
		shdparam.sched_priority = 35;
		pthread_attr_setschedparam(&pthattr, &shdparam);
		pthread_create(&pthid, &pthattr, render_thread, NULL);
	}
	else
	{
	//	install_timer(10, pspInputThread, NULL);
	}

	return NULL;
}

static void pspgu_shutdown_driver(void)
{
	mem_free(mouse_buffer);
	mem_free(background_buffer);
	mem_free(new_background_buffer);
	pspgu_driver.shutdown_device(mouse_graphics_device);

	memset(pspgu_mem,0,pspgu_mem_size);
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
	if ((unsigned)dest->x * (unsigned)dest->y > (unsigned)MAXINT / pspgu_pixelsize) overalloc();
	dest->data=mem_alloc(dest->x*dest->y*pspgu_pixelsize);
	dest->skip=dest->x*pspgu_pixelsize;
	dest->flags=0;
	return 0;
}

/* Return value:        0 alloced on heap
 *                      1 alloced in vidram
 *                      2 alloced in X server shm
 */
/*
static int pspgu_get_filled_bitmap(struct bitmap *dest, long color)
{
	int n;

	if (dest->x && (unsigned)dest->x * (unsigned)dest->y / (unsigned)dest->x != (unsigned)dest->y) overalloc();
	if ((unsigned)dest->x * (unsigned)dest->y > MAXINT / pspgu_pixelsize) overalloc();
	n=dest->x*dest->y*pspgu_pixelsize;
	dest->data=mem_alloc(n);
	pixel_set(dest->data,n,&color);
	dest->skip=dest->x*pspgu_pixelsize;
	dest->flags=0;
	return 0;
}
*/

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

	scr_start=pspgu_vmem+y*pspgu_linesize+x*pspgu_pixelsize;
	for(;ys;ys--){
		memcpy(scr_start,data,xs*pspgu_pixelsize);
		data+=hndl->skip;
		scr_start+=pspgu_linesize;
	}
	END_GR
	s_bbDirty = truE;
}

static void pspgu_draw_bitmaps(struct graphics_device *dev, struct bitmap **hndls, int n, int x, int y)
{
	TEST_INACTIVITY

	if (x>=pspgu_xsize||y>pspgu_ysize) return;
	while(x+(*hndls)->x<=0&&n){
		x+=(*hndls)->x;
		n--;
		hndls++;
	}
	while(n&&x<=pspgu_xsize){
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

	dest=pspgu_vmem+top*pspgu_linesize+left*pspgu_pixelsize;
	for (y=bottom-top;y;y--){
		pixel_set(dest,(right-left)*pspgu_pixelsize,&color);
		dest+=pspgu_linesize;
	}
	END_GR
	s_bbDirty = truE;
}

static void pspgu_draw_hline(struct graphics_device *dev, int left, int y, int right, long color)
{
	unsigned char *dest;
	HLINE_CLIP_PREFACE

	dest=pspgu_vmem+y*pspgu_linesize+left*pspgu_pixelsize;
	pixel_set(dest,(right-left)*pspgu_pixelsize,&color);
	END_GR
	s_bbDirty = truE;
}

static void pspgu_draw_vline(struct graphics_device *dev, int x, int top, int bottom, long color)
{
	unsigned char *dest;
	int y;
	VLINE_CLIP_PREFACE

	dest=pspgu_vmem+top*pspgu_linesize+x*pspgu_pixelsize;
	for (y=(bottom-top);y;y--){
		memcpy(dest,&color,pspgu_pixelsize);
		dest+=pspgu_linesize;
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
		len=(dev->clip.x2-dev->clip.x1-sc)*pspgu_pixelsize;
		src=pspgu_vmem+pspgu_linesize*dev->clip.y1+dev->clip.x1*pspgu_pixelsize;
		dest=src+sc*pspgu_pixelsize;
		for (y=dev->clip.y2-dev->clip.y1;y;y--){
			memmove(dest,src,len);
			dest+=pspgu_linesize;
			src+=pspgu_linesize;
		}
	}else{
		len=(dev->clip.x2-dev->clip.x1+sc)*pspgu_pixelsize;
		dest=pspgu_vmem+pspgu_linesize*dev->clip.y1+dev->clip.x1*pspgu_pixelsize;
		src=dest-sc*pspgu_pixelsize;
		for (y=dev->clip.y2-dev->clip.y1;y;y--){
			memmove(dest,src,len);
			dest+=pspgu_linesize;
			src+=pspgu_linesize;
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
	len=(dev->clip.x2-dev->clip.x1)*pspgu_pixelsize;
	if (sc>0){
		/* Down */
		dest=pspgu_vmem+(dev->clip.y2-1)*pspgu_linesize+dev->clip.x1*pspgu_pixelsize;
		src=dest-pspgu_linesize*sc;
		for (y=dev->clip.y2-dev->clip.y1-sc;y;y--){
			memcpy(dest,src,len);
			dest-=pspgu_linesize;
			src-=pspgu_linesize;
		}
	}else{
		/* Up */
		dest=pspgu_vmem+dev->clip.y1*pspgu_linesize+dev->clip.x1*pspgu_pixelsize;
		src=dest-pspgu_linesize*sc;
		for (y=dev->clip.y2-dev->clip.y1+sc;y;y--){
			memcpy(dest,src,len);
			dest+=pspgu_linesize;
			src+=pspgu_linesize;
		}
	}
	END_GR
	s_bbDirty = truE;
	return 1;
}

static void pspgu_set_clip_area(struct graphics_device *dev, struct rect *r)
{
	memcpy(&dev->clip, r, sizeof(struct rect));
	if (dev->clip.x1>=dev->clip.x2||dev->clip.y2<=dev->clip.y1||dev->clip.y2<=0||dev->clip.x2<=0||dev->clip.x1>=pspgu_xsize
			||dev->clip.y1>=pspgu_ysize){
		/* Empty region */
		dev->clip.x1=dev->clip.x2=dev->clip.y1=dev->clip.y2=0;
	}else{
		if (dev->clip.x1<0) dev->clip.x1=0;
		if (dev->clip.x2>pspgu_xsize) dev->clip.x2=pspgu_xsize;
		if (dev->clip.y1<0) dev->clip.y1=0;
		if (dev->clip.y2>pspgu_ysize) dev->clip.y2=pspgu_ysize;
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
	if (border_left | border_top | border_right | border_bottom) memset(pspgu_mem,0,pspgu_mem_size);
	if (current_virtual_device) current_virtual_device->redraw_handler(current_virtual_device
			,&current_virtual_device->size);
}


struct graphics_driver pspgu_driver={
	"pspgu",
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
