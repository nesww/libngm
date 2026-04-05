#include "include/libngm.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
    int fd = ngm_get_drm_fd("/dev/dri/card1");
    if (fd < 0) {
        return 1;
    }

    ngm_root *ngm = ngm_get_root(fd);
    if (!ngm) {
        printf("error: ngm root could not be created\n");
        exit(EXIT_FAILURE);
    }
    printf("First CRTC found: %d, %s (%d x %d @ %d)\n",
        ngm->display_info->crtc_id,
        ngm->display_info->mode.name,
        ngm->display_info->mode.hdisplay,
        ngm->display_info->mode.vdisplay,
        ngm->display_info->mode.vrefresh
    );

    printf("Framebuffer informations: %dx%d pitch=%d size=%ld fb_id=%d\n", ngm->fb->width, ngm->fb->height, ngm->fb->pitch, ngm->fb->size, ngm->fb->fb_id);

    uint32_t *pixels = (uint32_t*)ngm->fb->map;
    for (int i = 0; i < ngm->fb->width * ngm->fb->height; ++i) {
        if (i%3 == 0)
            pixels[i] = 0xFF00FF;
        else
            pixels[i] = 0x00FF00;
    }

    if (drmModeSetCrtc(fd, ngm->display_info->crtc_id, ngm->fb->fb_id, 0, 0, &ngm->display_info->connector_id, 1, &ngm->display_info->mode)) {
        perror("drmModeSetCrtc");
        ngm_free_root(ngm);
        close(fd);
        return 1;
    }

    while(1) {
        pause();
    }

    //clean
    ngm_free_root(ngm);
    close(fd);
    return 0;
}
