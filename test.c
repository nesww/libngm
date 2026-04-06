#include "include/libngm.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
    ngm_log_init();
    ngm_log_set_level(NGM_LOG_ERROR);

    int fd = ngm_get_drm_fd("/dev/dri/card1");
    if (fd < 0) {
        return 1;
    }

    ngm_root *ngm = ngm_get_root(fd);
    if (!ngm) {
        printf("error: ngm root could not be created\n");
        exit(EXIT_FAILURE);
    }

    ngm_show_CRTC(ngm);
    ngm_show_framebuffer(ngm);

    for (int y = 200; y < 300; ++y) {
        ngm_set_line(ngm->fb, 0, y, ngm->fb->width/2, y, 0xFF0000);
    }


    ngm_set_line(ngm->fb, 100, 700, 1900, 700, 0x00FF00);


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
