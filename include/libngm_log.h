#ifndef NAGAME_LOG_H
#define NAGAME_LOG_H

#include <stdint.h>
#include "libngm.h"

extern void _ngm_log(uint8_t level, const char *file, int line, const char *func, const char *fmt, ...);
extern void ngm_log_init();
extern void ngm_log_set_file(const char *path);
extern void ngm_log_set_level(uint8_t log_level);
extern uint8_t ngm_get_log_level();

#define NGM_LOG_DEFAULT_PATH "./libngm.log"
#define NGM_ERR_EXIT 1


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
        exit(NGM_ERR_EXIT); \
    } while(0) \


#endif
