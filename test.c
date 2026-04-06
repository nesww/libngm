#include "include/libngm.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static volatile int running = 1;

void handle_sig(int sig) {
    (void)sig;
    running = 0;
    printf("SIGINT: exiting\n");
}

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

    ngm_vec2 start = {0,200};
    ngm_vec2 dest = {100,300};
    ngm_set_line(ngm->fb, &start, &dest, 0xFF0000);


    if (drmModeSetCrtc(fd, ngm->display_info->crtc_id, ngm->fb->fb_id, 0, 0, &ngm->display_info->connector_id, 1, &ngm->display_info->mode)) {
        perror("drmModeSetCrtc");
        ngm_free_root(ngm);
        close(fd);
        return 1;
    }

    signal(SIGINT, handle_sig);
    while (running) pause();

    //clean
    ngm_free_root(ngm);
    close(fd);
    return 0;
}
