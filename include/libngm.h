#ifndef NAGAME_H
#define NAGAME_H

#include <drm/drm_mode.h>
#include <sys/types.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <fcntl.h>

#define NGM_NOT_IMPLEMENTED(str)                                                              \
  do {                                                                         \
    if (str)                                                                   \
      fprintf(stderr, "not implemented: " #str "\n");                                      \
    exit(1);                                                                   \
  } while (0)

#define NGM_UNUSED(x) ((void)x)

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
extern ngm_framebuffer *ngm_get_dumb_buffer(int fd, ngm_display_output *info);

extern void ngm_show_drm(int fd, drmModeResPtr res);
extern void ngm_show_CRTC(ngm_root *ngm);
extern void ngm_show_framebuffer(ngm_root *ngm);

extern void ngm_set_pixel(ngm_framebuffer *fb, uint32_t x, uint32_t y, uint32_t color);
extern void ngm_set_line(ngm_framebuffer *fb, uint32_t sx, uint32_t sy, uint32_t ax, uint32_t ay, uint32_t color);

extern void _ngm_free_display_info(ngm_display_output *info);
extern void ngm_free_root(ngm_root *ngm);


void _ngm_log(uint8_t level, const char *file, int line, const char *func, const char *fmt, ...);
extern void ngm_log_init();
extern void ngm_log_set_file(const char *path);
extern void ngm_log_set_level(uint8_t log_level);
extern uint8_t ngm_get_log_level();

#define NGM_LOG_DEFAULT_PATH "./libngm.log"

#define NGM_LOG_NONE  0x0
#define NGM_LOG_INFO  0x1
#define NGM_LOG_DEBUG 0x2
#define NGM_LOG_WARN  0x3
#define NGM_LOG_ERROR 0x4

#define NGM_INFO(...) \
    do { \
        if (ngm_get_log_level() >= NGM_LOG_INFO) \
        _ngm_log(NGM_LOG_INFO, __FILE__, __LINE__, __func__, __VA_ARGS__); \
    } while(0) \

#define NGM_DEBUG(...) \
    do { \
        if (ngm_get_log_level() >= NGM_LOG_DEBUG) \
        _ngm_log(NGM_LOG_DEBUG, __FILE__, __LINE__, __func__, __VA_ARGS__); \
    } while(0) \

#define NGM_WARN(...) \
    do { \
        if (ngm_get_log_level() >= NGM_LOG_WARN) \
        _ngm_log(NGM_LOG_WARN, __FILE__, __LINE__, __func__, __VA_ARGS__); \
    } while(0) \

#define NGM_ERROR(...) \
    do { \
        if (ngm_get_log_level() >= NGM_LOG_ERROR) \
        _ngm_log(NGM_LOG_ERROR, __FILE__, __LINE__, __func__, __VA_ARGS__); \
    } while(0) \


#endif
