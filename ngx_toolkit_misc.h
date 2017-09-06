/*
 * Copyright (C) AlexWoo(Wu Jie) wj19840501@gmail.com
 */


#ifndef _NGX_TOOLKIT_MISC_H_INCLUDED_
#define _NGX_TOOLKIT_MISC_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


#define NGX_MD5KEY_LEN  32

ngx_int_t ngx_md5_file(ngx_fd_t, u_char md5key[NGX_MD5KEY_LEN]);


#endif
