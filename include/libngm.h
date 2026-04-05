#ifndef NAGAME_H
#define NAGAME_H

#include <drm/drm_mode.h>
#include <sys/types.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <fcntl.h>

#define NGM_DUMB_FB_COLOR_DEPTH_BITS 24


typedef struct {
    drmModeCrtcPtr      crtc;
    drmModeConnectorPtr connector;
    drmModeModeInfo     mode;
    uint32_t            crtc_id;
    uint32_t            connector_id;
} ngm_display_output;

typedef struct {
    uint32_t handle; //kernel identifier
    uint32_t pitch;  //bytes per row
    uint64_t size;   //total size
    uint32_t fb_id;  //KMS framebuffer id
    void     *map;   //pixels pointer after mmap
    uint32_t width;
    uint32_t height;
} ngm_framebuffer;

typedef struct {
    int fd;
    ngm_display_output *display_info;
    ngm_framebuffer *fb;
} ngm_root;


extern ngm_root *ngm_get_root(int fd);
extern int ngm_get_drm_fd(const char *path);
extern ngm_display_output *_ngm_get_display_output(int fd, drmModeResPtr res);
extern void ngm_print_drm_informations(int fd, drmModeResPtr res);
extern ngm_framebuffer *ngm_get_dumb_buffer(int fd, ngm_display_output *info);
extern void _ngm_free_display_info(ngm_display_output *info);
extern void ngm_free_root(ngm_root *ngm);

#endif
