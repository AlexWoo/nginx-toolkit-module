/*
 * Copyright (C) AlexWoo(Wu Jie) wj19840501@gmail.com
 */


#ifndef _NGX_TOOLKIT_MISC_H_INCLUDED_
#define _NGX_TOOLKIT_MISC_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


#define NGX_MD5KEY_LEN  32

ngx_int_t ngx_md5_file(ngx_fd_t, u_char md5key[NGX_MD5KEY_LEN]);

#ifdef NGX_DEBUG

#define NGX_START_TIMING                                            \
    struct timeval      start, end;                                 \
    ngx_gettimeofday(&start);

#define NGX_STOP_TIMING(log, msg)                                   \
    ngx_gettimeofday(&end);                                         \
    ngx_log_error(NGX_LOG_INFO, log, 0, msg " spend %ui us",        \
        (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec));

#else

#define NGX_START_TIMING
#define NGX_STOP_TIMING(log, msg)

#endif

#endif
