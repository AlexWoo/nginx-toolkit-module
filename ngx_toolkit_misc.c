/*
 * Copyright (C) AlexWoo(Wu Jie) wj19840501@gmail.com
 */


#include "ngx_toolkit_misc.h"
#include <ngx_md5.h>


#define FILEBUFSIZE     8192

ngx_int_t
ngx_md5_file(ngx_fd_t fd, u_char md5key[NGX_MD5KEY_LEN])
{
    ngx_md5_t                   ctx;
    u_char                      buf[FILEBUFSIZE];
    u_char                      md5[16];
    ssize_t                     n;
    ngx_uint_t                  i;
    u_char                     *p;

    ngx_md5_init(&ctx);

    for (;;) {
        n = ngx_read_fd(fd, buf, FILEBUFSIZE);

        if (n == 0) { /* read eof of file */
            break;
        }

        if (n == NGX_FILE_ERROR) {
            return NGX_ERROR;
        }

        ngx_md5_update(&ctx, buf, n);
    }

    ngx_md5_final(md5, &ctx);

    p = md5key;
    for (i = 0; i < 16; ++i) {
        p = ngx_sprintf(p, "%02xi", md5[i]);
    }

    return NGX_OK;
}
