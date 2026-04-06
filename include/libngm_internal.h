#ifndef NAGAME_INTERNAL_H
#define NAGAME_INTERNAL_H

#include <libngm.h>
#include <xf86drmMode.h>
#include "libngm_log.h"

extern void _ngm_free_display_info(ngm_display_output *info);
extern void _ngm_free_framebuffer(int fd, ngm_framebuffer *fb);


extern drmModeConnectorPtr _ngm_get_connected_connector (int fd, drmModeResPtr res);
extern ngm_display_output *_ngm_get_display_output(int fd, drmModeResPtr res);

extern uint8_t _ngm_valid_vec2_in_buffer(ngm_framebuffer *fb, ngm_vec2 *p);

#endif
