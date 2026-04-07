#include <libngm.h>
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

    ngm_root *root = ngm_get_root(fd);
    if (!root) {
        printf("error: ngm root could not be created\n");
        exit(EXIT_FAILURE);
    }

    ngm_show_CRTC(root);
    ngm_show_framebuffer(root);


    ngm_set_crtc_from_root(root);

    ngm_vec2 start = {0,200};
    ngm_vec2 dest = {100,300};
    ngm_set_line(root->fb, &start, &dest, 0xFF0000);

    for (int y = dest.y; y < dest.y + 100; ++y) {
        for (int x = dest.x; x < dest.x + 200; ++x) {
            ngm_set_pixel_xy(root->fb, x, y, (x*0xFF + y*0xFF)%0xFFFFFF);
            usleep(NGM_USLEEP_60FPS/10);
        }
    }

    signal(SIGINT, handle_sig);
    while (running) pause();

    //clean
    ngm_free_root(root);
    close(fd);
    return 0;
}
