#include <drm/drm.h>
#include <drm/drm_mode.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include "../include/libngm.h"
#include <stdlib.h>
#include <xf86drmMode.h>
#include "../include/libngm_internal.h"

ngm_root *
ngm_get_root(int fd) {
    ngm_root *ngm = malloc(sizeof(ngm_root));
    if (!ngm) {
        perror("malloc");
        NGM_WARN("could not allocate memory for the ngm root");
        return NULL;
    }
    drmModeResPtr resources = drmModeGetResources(fd);

    ngm_display_output *info = _ngm_get_display_output(fd, resources);
    if (!info) {
        perror("_ngm_get_display_output");
        NGM_WARN("could not get the display output");
        goto clean_ngm;
    }
    ngm_framebuffer *fb = ngm_get_dumb_buffer(fd, info);
    if (!fb) {
        perror("ngm_get_dumb_buffer");
        NGM_WARN("could not get the dumb buffer");
        _ngm_free_display_info(info);
        goto clean_ngm;
    }

    ngm->display_info = info;
    ngm->fb           = fb;
    ngm->fd           = fd;

    drmModeFreeResources(resources);
    NGM_INFO("ngm_root was created with a framebuffer_id: %d", ngm->fb->fb_id);
    return ngm;

    clean_ngm:
        drmModeFreeResources(resources);
        free(ngm);
        return NULL;
}

int
ngm_get_drm_fd(const char *path) {
    int fd = open(path, O_RDWR | O_CLOEXEC);
    if (fd < 0) {
        perror("ngm_get_drm_fd");
        NGM_WARN("failed to open file descriptor for given path: %s\n", path);
        return -1;
    }
    return fd;
}

void
ngm_print_drm_informations(int fd, drmModeResPtr res) {
    if (res) {
        printf("Connectors: %p (%d)\n", (void*)res->connectors, res->count_connectors);
        printf("Encoders: %p (%d)\n", (void*)res->encoders, res->count_encoders);
        printf("CRTCs: %p (%d)\n", (void*)res->crtcs, res->count_crtcs);

        for (int i = 0; i < res->count_connectors; ++i) {
            drmModeConnectorPtr con = drmModeGetConnector(fd, res->connectors[i]);
            if (con) {

                if (con->count_modes > 0) {
                    printf("(id=%d) = Connector info: %ux%u@%u\n",
                        con->connector_id,
                        con->modes->hdisplay,
                        con->modes->vdisplay,
                        con->modes->vrefresh
                    );

                    for (int j = 0; j < con->count_encoders; ++j) {
                        drmModeEncoderPtr enc = drmModeGetEncoder(fd, con->encoders[j]);
                        if (enc) {
                            for (int k = 0; k < res->count_crtcs; ++k) {
                                if ((enc->possible_crtcs >> k) & 1) {
                                    printf("(id=%d) = encoder %d can use CRTC[%d] = id %u\n", con->connector_id, enc->encoder_id, k, res->crtcs[k]);
                                }
                            }
                        drmModeFreeEncoder(enc);
                        }
                    }
                } else {
                    printf("Connector %d has no modes\n", con->connector_id);
                }
                drmModeFreeConnector(con);
            }
        }
    }
}

ngm_framebuffer *
ngm_get_dumb_buffer(int fd, ngm_display_output *info) {
    if (!info) {
        return NULL;
    }
    ngm_framebuffer * fb = malloc(sizeof(ngm_framebuffer));
    if (!fb) {
        perror("malloc");
        NGM_ERROR("could not allocate memory for the framebuffer");
        return NULL;
    }

    fb->width = info->mode.hdisplay;
    fb->height = info->mode.vdisplay;

    struct drm_mode_create_dumb create_dumb = {
        .width = fb->width,
        .height = fb->height,
        .bpp = 32,
    };

    int alloc = ioctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &create_dumb);
    if (alloc != 0) {
        perror("CREATE_DUMB");
        NGM_ERROR("failed to create the dumb buffer map");
        goto clean;
    }
    struct drm_mode_map_dumb map_dumb = {
        .handle = create_dumb.handle,
        .pad = 0
    };

    int map_res = ioctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &map_dumb);
    if (map_res != 0) {
        perror("MAP_DUMB");
        NGM_ERROR("failed to map the dumb buffer");
        goto clean_addfb;
    }

    uint32_t fb_id = 0;
    if (drmModeAddFB(fd,info->mode.hdisplay,info->mode.vdisplay,NGM_DUMB_FB_COLOR_DEPTH_BITS,create_dumb.bpp,create_dumb.pitch,create_dumb.handle,&fb_id)) {
        perror("drmModeAddFB");
        NGM_ERROR("could not add the dumb buffer to DRM mode");
        goto clean_addfb;
    }

    void *map = mmap(NULL, create_dumb.size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, map_dumb.offset);
    if (map == MAP_FAILED) {
        perror("mmap");
        NGM_ERROR("could not mmap the map for the dumb buffer");
        goto clean_mmap;
    }
    fb->fb_id  = fb_id;
    fb->handle = create_dumb.handle;
    fb->pitch  = create_dumb.pitch;
    fb->size   = create_dumb.size;
    fb->map    = map;

    NGM_INFO("dumb framebuffer was created, with an id: %d", fb->fb_id);
    NGM_DEBUG("framebuffer width: %d, framebuffer width: %d", fb->width, fb->height);
    NGM_DEBUG("framebuffer map address: %p , pitch: %d", fb->map, fb->pitch);

    return fb;

    clean_mmap:
        drmModeRmFB(fd, fb_id);
    clean_addfb:
        struct drm_mode_destroy_dumb destroy = {.handle = create_dumb.handle };
        ioctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
    clean:
        free(fb);
        return NULL;
}

drmModeConnectorPtr
_ngm_get_connected_connector(int fd, drmModeResPtr res) {
    drmModeConnectorPtr out = NULL;
    for (int i = 0; i < res->count_connectors; ++i) {
        drmModeConnectorPtr con = drmModeGetConnector(fd, res->connectors[i]);
        if (con && con->connection ==  DRM_MODE_CONNECTED) {
            out = con;
            break;
        }
         if (con) drmModeFreeConnector(con);
    }
    return out;
}

ngm_display_output *
_ngm_get_display_output(int fd, drmModeResPtr res) {
    drmModeCrtcPtr crtc = NULL;
    drmModeConnectorPtr con = _ngm_get_connected_connector(fd, res);
    uint8_t found = 0x0;
    if (con) {
        for (int i = 0; i < con->count_encoders; ++i) {
            if (!found) {
                drmModeEncoderPtr enc = drmModeGetEncoder(fd, con->encoders[i]);
                if (enc) {
                    for (int j = 0; j < res->count_crtcs; ++j) {
                        if ((enc->possible_crtcs >> j) & 1) {
                            crtc = drmModeGetCrtc(fd, res->crtcs[j]);
                            found = 0x1;
                            break;
                        }
                    }
                    drmModeFreeEncoder(enc);
                }
            }
        }
    }
    if (found) {
        ngm_display_output* ngm_do = malloc(sizeof(ngm_display_output));
        if (!ngm_do) {
            perror("malloc");
            NGM_WARN("failed to malloc for ngm_display_output");
            return NULL;
        }
        ngm_do->crtc = crtc;
        ngm_do->crtc_id = crtc->crtc_id;
        ngm_do->connector = con;
        ngm_do->connector_id = con->connector_id;
        ngm_do->mode = con->modes[0];
        return ngm_do;
    }
    drmModeFreeConnector(con);
    return NULL;
}

void
ngm_print_driver_info(int fd) {
    drmVersionPtr ver = drmGetVersion(fd);
    if (ver) {
        printf("Driver: %s\n", ver->name);
        printf("Version: %d.%d.%d\n", ver->version_major, ver->version_minor, ver->version_patchlevel);
        drmFreeVersion(ver);
    }

}

void
ngm_show_CRTC(ngm_root *root) {
    printf("First CRTC found: %d, %s (%d x %d @ %d)\n",
        root->display_info->crtc_id,
        root->display_info->mode.name,
        root->display_info->mode.hdisplay,
        root->display_info->mode.vdisplay,
        root->display_info->mode.vrefresh
    );
}

void
ngm_show_framebuffer(ngm_root *root) {
    printf("Framebuffer informations: %dx%d pitch=%d size=%ld fb_id=%d\n",
        root->fb->width,
        root->fb->height,
        root->fb->pitch,
        root->fb->size,
        root->fb->fb_id);
}

uint8_t
_ngm_valid_vec2_in_buffer(ngm_framebuffer *fb, ngm_vec2 *p) {
    if (p->x < 0 || p->x >= fb->width) {
        NGM_WARN("invalid x position: tried to draw a pixel at %d when framebuffer has 0 to %d width", p->x, fb->width);
        return 0;
    }
    if (p->y < 0 || p->y >= fb->height) {
        NGM_WARN("invalid y position: tried to draw a pixel at %d when framebuffer has 0 to %d height", p->y, fb->height);
        return 0;
    }
    return 1;
}

void
ngm_set_pixel(ngm_framebuffer *fb, ngm_vec2 *p, uint32_t color) {
    if (!fb) {
        NGM_WARN("no framebuffer given, won't set pixel");
        return;
    }

    if (!_ngm_valid_vec2_in_buffer(fb, p)) return;

    NGM_FB_PX(fb, p->x, p->y) = color;
}

void
ngm_set_pixel_xy(ngm_framebuffer *fb, int32_t x, int32_t y, uint32_t color) {
    ngm_vec2 p = {x,y};
    ngm_set_pixel(fb, &p, color);
}

void
ngm_set_line(ngm_framebuffer *fb, ngm_vec2* start, ngm_vec2 *destination, uint32_t color) {
    if (!fb) {
        NGM_WARN("no framebuffer given, won't set line");
        return;
    }
    if (!_ngm_valid_vec2_in_buffer(fb, start)) return;
    if (!_ngm_valid_vec2_in_buffer(fb, destination)) return;

    int32_t dx = destination->x - start->x;
    int32_t dy = destination->y - start->y;

    int32_t inc_x = dx < 0 ? -1 : 1;
    int32_t inc_y = dy < 0 ? -1 : 1;
    uint32_t e = 0;

    ngm_vec2 cursor = {start->x, start->y};

    if (abs(dx) >= abs(dy)) {
        for(;cursor.x != destination->x; cursor.x += inc_x) {
            ngm_vec2 p = {cursor.x, cursor.y};
            ngm_set_pixel(fb, &p, color);
            e+= abs(dy);
            if(e>=abs(dx)) {
                e-=abs(dx);
                cursor.y += inc_y;
            }
        }
    } else {
        for(;cursor.y != destination->y; cursor.y += inc_y) {
            ngm_vec2 p = {cursor.x, cursor.y};
            ngm_set_pixel(fb, &p, color);
            e+= abs(dx);
            if(e>=abs(dy)) {
                e-=abs(dy);
                cursor.x += inc_x;
            }
        }
    }
}

void
ngm_set_line_xy(ngm_framebuffer *fb, int32_t sx, int32_t sy, int32_t dx, int32_t dy,  uint32_t color) {
    ngm_vec2 start = {sx,sy};
    ngm_vec2 dest  = {dx,dy};
    ngm_set_line(fb, &start, &dest, color);
}

void
_ngm_free_display_info(ngm_display_output *info) {
    if (!info) return;

    drmModeFreeConnector(info->connector);
    drmModeFreeCrtc(info->crtc);
    free(info);
}

void
_ngm_free_framebuffer(int fd, ngm_framebuffer *fb) {
    if (!fb) return;

    munmap(fb->map, fb->size);
    drmModeRmFB(fd, fb->fb_id);
    struct drm_mode_destroy_dumb destroy = {
        .handle = fb->handle,
    };
    ioctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
    free(fb);
}

void
ngm_free_root(ngm_root *root) {
    _ngm_free_display_info(root->display_info);
    _ngm_free_framebuffer(root->fd, root->fb);
}

static FILE *_ngm_log_file = NULL;
static uint8_t _ngm_log_level = NGM_LOG_NONE;

void
ngm_log_init() {
    ngm_log_set_file(NGM_LOG_DEFAULT_PATH);
}

uint8_t
ngm_get_log_level() {
    return _ngm_log_level;
}

void
ngm_log_set_level(uint8_t log_level) {
    _ngm_log_level = log_level;
}

void ngm_log_set_file(const char *path) {
    if (_ngm_log_file) fclose(_ngm_log_file);
    _ngm_log_file = fopen(path, "a");
}

void
_ngm_log(uint8_t level, const char *file, int line, const char *func, const char *fmt, ...) {
    const char *names[] = {"none", "error", "warn", "info", "debug"};

    if (level < 1 || level > 4) return;

    va_list ap;
    va_start(ap, fmt);

    FILE *out = _ngm_log_file ? _ngm_log_file : stderr;

    fprintf(out, "libngm: %s %s: %d (%s) ", names[level], file, line, func);
    vfprintf(out, fmt, ap);
    fprintf(out, "\n");

    va_end(ap);

    if (_ngm_log_file) fflush(_ngm_log_file);
}

uint8_t ngm_set_crtc_from_root(ngm_root *root) {
    if (
        drmModeSetCrtc(
            root->fd,
            root->display_info->crtc_id,
            root->fb->fb_id,
            0, 0,
            &root->display_info->connector_id, 1,
            &root->display_info->mode
        )
    ) {
        perror("drmModeSetCrtc");
        NGM_WARN("failed to set crtc for the root:");
        return FALSE;
    }
    return TRUE;
}
