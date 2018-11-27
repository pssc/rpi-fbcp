
#include <stdio.h>
#include <syslog.h>
#include <stdlib.h>

#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>

#include <bcm_host.h>

#define FPS 15

int process() {
    DISPMANX_DISPLAY_HANDLE_T display;
    DISPMANX_MODEINFO_T display_info;
    DISPMANX_RESOURCE_HANDLE_T screen_resource;
    VC_IMAGE_TRANSFORM_T transform;
    uint32_t image_prt;
    VC_RECT_T rect1;
    int ret;
    unsigned int count = 0;
    int fbfd = 0;
    int dpitch = 0;
    int delay = FPS;
    char *fbp = NULL;
    char *fps = NULL;
    char *fbdev = "/dev/fb1";
    char *fbdev0 = "/dev/fb0";

    struct fb_var_screeninfo vinfo;
    struct fb_var_screeninfo vinfo0;
    struct fb_fix_screeninfo finfo;
    struct fb_fix_screeninfo finfo0;


    bcm_host_init();

    fbp = getenv("FBDEV");
    if (fbp) {
       fbdev=fbp;
       fbp=(char *)-1;
    }

    fps = getenv("FPS");
    if (fps) {
	delay = atoi(fps);
	delay = (delay == 0) ? FPS : delay;
    }
    delay = 1000000 / delay;

    display = vc_dispmanx_display_open(0);
    if (!display) {
        syslog(LOG_ERR, "Unable to open primary display");
        return -1;
    }
    ret = vc_dispmanx_display_get_info(display, &display_info);
    if (ret) {
        syslog(LOG_ERR, "Unable to get primary display information");
        return -1;
    }
    syslog(LOG_INFO, "Primary display vc is %d x %d rotation %d", display_info.width, display_info.height, (display_info.transform & 3) * 90);

    fbfd = open(fbdev0, O_RDWR);
    if (fbfd < 0) {
        syslog(LOG_ERR, "Unable to open primary display");
        return -1;
    }
    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo0)) {
        syslog(LOG_ERR, "Unable to get primary fb display information");
        return -1;
    }
    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo0)) {
        syslog(LOG_ERR, "Unable to get pirmary fb display information");
        return -1;
    }
    syslog(LOG_INFO, "Primary display fb is %d x %d %dbps\n", vinfo0.xres, vinfo0.yres, vinfo0.bits_per_pixel);
    close(fbfd);

    fbfd = open(fbdev, O_RDWR);
    if (fbfd < 0) {
        syslog(LOG_ERR, "Unable to open secondary display %s",fbdev);
        return -1;
    }
    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo)) {
        syslog(LOG_ERR, "Unable to get secondary display information");
        return -1;
    }
    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo)) {
        syslog(LOG_ERR, "Unable to get secondary display information");
        return -1;
    }
    syslog(LOG_INFO, "Second display %s is %d x %d %dbps\n",fbdev , vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);

    dpitch = vinfo.xres * vinfo.bits_per_pixel / 8;
    screen_resource = vc_dispmanx_resource_create(VC_IMAGE_RGB565, vinfo.xres, vinfo.yres, &image_prt);
    if (!screen_resource) {
        syslog(LOG_ERR, "Unable to create screen buffer");
        close(fbfd);
        vc_dispmanx_display_close(display);
        return -1;
    }

    fbp = (char*) mmap(0, finfo.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    if (fbp <= 0) {
        syslog(LOG_ERR, "Unable to create memory mapping");
        close(fbfd);
        ret = vc_dispmanx_resource_delete(screen_resource);
        vc_dispmanx_display_close(display);
        return -1;
    } else {
        syslog(LOG_INFO, "Starting Snapshotting for %s FPS with delay %d", fps, delay);
    }

    vc_dispmanx_rect_set(&rect1, 0, 0, vinfo.xres, vinfo.yres);

    ret = 0;
    while (ret == 0) {
        ret = vc_dispmanx_snapshot(display, screen_resource, 0);
        if (ret != 0) {
		count +=1;
		if ( count % 60 == 0 ) {
			syslog(LOG_ERR, "Unable to snapshot %d. (%u)", ret, count);
		}
		// for when tvservice -o has been called - check state some how?
		vc_dispmanx_resource_delete(screen_resource);
		display = vc_dispmanx_display_open(0);
		screen_resource = vc_dispmanx_resource_create(VC_IMAGE_RGB565, vinfo.xres, vinfo.yres, &image_prt);
		sleep(1);
		ret = 0;
        } else {
	     if (count > 0 ) {
			syslog(LOG_INFO, "Snapshotting after %u attempts.",count);
		count = 0;
	     }
	     ret = vc_dispmanx_resource_read_data(screen_resource, &rect1, fbp, dpitch);
             if (ret != 0) {
                 syslog(LOG_ERR, "Unable to read data from vc %d.", ret);
             }
             usleep(delay);
        }
    }
    syslog(LOG_INFO, "Terminating...");

    munmap(fbp, finfo.smem_len);
    close(fbfd);
    ret = vc_dispmanx_resource_delete(screen_resource);
    vc_dispmanx_display_close(display);
}

int main(int argc, char **argv) {
    setlogmask(LOG_UPTO(LOG_DEBUG));
    openlog("fbcp", LOG_NDELAY | LOG_PID, LOG_USER);

    return process();
}

