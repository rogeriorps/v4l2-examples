/*
 * Copyright 2004-2012 Freescale Semiconductor, Inc. All rights reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/*
 * @file mxc_v4l2_overlay.c
 *
 * @brief Mxc Video For Linux 2 driver test application
 *
 */

/*=======================================================================
                                        INCLUDE FILES
=======================================================================*/
/* Standard Include Files */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

/* Verification Test Environment Include Files */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <asm/types.h>
#include <linux/videodev2.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <math.h>
#include <string.h>
#include <malloc.h>
#include <signal.h>

#include <linux/mxcfb.h>

#define ipu_fourcc(a,b,c,d)\
        (((__u32)(a)<<0)|((__u32)(b)<<8)|((__u32)(c)<<16)|((__u32)(d)<<24))

int main(int argc, char **argv)
{
	int g_sensor_width = 640;
	int g_sensor_height = 480;
	int g_sensor_top = 0;
	int g_sensor_left = 0;
	int g_display_width = 240;
	int g_display_height = 320;
	int g_display_top = 0;
	int g_display_left = 0;
	int g_rotate = 0;
	int g_timeout = 3600;
	int g_display_lcd = 0;
	int g_camera_framerate = 30;
	int g_capture_mode = 0;

        struct v4l2_format fmt;
        struct v4l2_framebuffer fb_v4l2;
        struct v4l2_streamparm parm;
        struct v4l2_control ctl;
        struct v4l2_crop crop;
	struct v4l2_frmsizeenum fsize;

	char v4l_device[100] = "/dev/video0";
        char fb_device[100] = "/dev/fb0";
	int fd_v4l = 0;
        int fd_fb = 0;
        struct fb_fix_screeninfo fb_fix;
        struct fb_var_screeninfo fb_var;

        int overlay = 1;

        if ((fd_v4l = open(v4l_device, O_RDWR, 0)) < 0) {
                printf("Unable to open %s\n", v4l_device);
                return -1;
        }

	if ((fd_fb = open(fb_device, O_RDWR )) < 0) {
		printf("Unable to open frame buffer 0\n");
                return -1;
        }

        if (ioctl(fd_fb, FBIOGET_VSCREENINFO, &fb_var) < 0) {
                close(fd_fb);
                return -1;
        }
        if (ioctl(fd_fb, FBIOGET_FSCREENINFO, &fb_fix) < 0) {
                close(fd_fb);
                return -1;
        }

	if (strcmp(fb_fix.id, "DISP3 BG - DI1") == 0)
		g_display_lcd = 1;
	else if (strcmp(fb_fix.id, "DISP3 BG") == 0)
		g_display_lcd = 0;
	else if (strcmp(fb_fix.id, "DISP4 BG") == 0)
		g_display_lcd = 3;
	else if (strcmp(fb_fix.id, "DISP4 BG - DI1") == 0)
		g_display_lcd = 4;

        if (ioctl(fd_v4l, VIDIOC_S_OUTPUT, &g_display_lcd) < 0)
        {
                printf("VIDIOC_S_OUTPUT failed\n");
                return -1;
        }

	parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	parm.parm.capture.timeperframe.numerator = 1;
	parm.parm.capture.timeperframe.denominator = g_camera_framerate;
	parm.parm.capture.capturemode = g_capture_mode;
	if (ioctl(fd_v4l, VIDIOC_S_PARM, &parm) < 0)
	{
	        printf("VIDIOC_S_PARM failed\n");
	        return -1;
	}

	ctl.id = V4L2_CID_PRIVATE_BASE + 2;
	ctl.value = g_rotate;
        if (ioctl(fd_v4l, VIDIOC_S_CTRL, &ctl) < 0)
        {
                printf("set control failed\n");
                return -1;
        }

        crop.type = V4L2_BUF_TYPE_VIDEO_OVERLAY;
        crop.c.left = g_sensor_left;
        crop.c.top = g_sensor_top;
        crop.c.width = g_sensor_width;
        crop.c.height = g_sensor_height;
        if (ioctl(fd_v4l, VIDIOC_S_CROP, &crop) < 0)
        {
                printf("set cropping failed\n");
                return -1;
        }

        fmt.type = V4L2_BUF_TYPE_VIDEO_OVERLAY;
        fmt.fmt.win.w.top=  g_display_top ;
        fmt.fmt.win.w.left= g_display_left;
        fmt.fmt.win.w.width=g_display_width;
        fmt.fmt.win.w.height=g_display_height;
        if (ioctl(fd_v4l, VIDIOC_S_FMT, &fmt) < 0)
        {
                printf("set format failed\n");
                return -1;
        }

        memset(&fb_v4l2, 0, sizeof(fb_v4l2));

	fb_v4l2.fmt.width = fb_var.xres;
	fb_v4l2.fmt.height = fb_var.yres;
        
        if (fb_var.bits_per_pixel == 32) {
                fb_v4l2.fmt.pixelformat = ipu_fourcc('B','G','R','4'); // 32bits - BGR-8-8-8-8
                fb_v4l2.fmt.bytesperline = 4 * fb_v4l2.fmt.width;
        }
        else if (fb_var.bits_per_pixel == 24) {
                fb_v4l2.fmt.pixelformat = ipu_fourcc('B','G','R','3'); // 24bits - BGR-8-8-8
                fb_v4l2.fmt.bytesperline = 3 * fb_v4l2.fmt.width;
        }
        else if (fb_var.bits_per_pixel == 16) {
                fb_v4l2.fmt.pixelformat = ipu_fourcc('R','G','B','P'); // 16bits - RGB-5-6-5
                fb_v4l2.fmt.bytesperline = 2 * fb_v4l2.fmt.width;
        }

	fb_v4l2.flags = V4L2_FBUF_FLAG_PRIMARY;
	fb_v4l2.base = (void *) fb_fix.smem_start +
	fb_fix.line_length*fb_var.yoffset;

	close(fd_fb);

	if (ioctl(fd_v4l, VIDIOC_S_FBUF, &fb_v4l2) < 0) {
                printf("set framebuffer failed\n");
                return -1;
        }

	if (ioctl(fd_v4l, VIDIOC_OVERLAY, &overlay) < 0) {
		printf("VIDIOC_OVERLAY start failed\n");
		goto out1;
	}

        sleep(g_timeout);

out1:
        close(fd_v4l);
}

