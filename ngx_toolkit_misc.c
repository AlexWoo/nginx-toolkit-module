/*
 * Copyright (C) AlexWoo(Wu Jie) wj19840501@gmail.com
 */


#include "ngx_toolkit_misc.h"
#include <ngx_md5.h>


typedef struct {
    ngx_str_t                   scheme;
    in_port_t                   port;
} ngx_scheme_port_t;


ngx_scheme_port_t ngx_sheme_port[] = {
    { ngx_string("http"),   80      },
    { ngx_string("https"),  443     },
    { ngx_string("rtmp"),   1935    },
    { ngx_null_string,      0       }
};


#define FILEBUFSIZE     8192


ngx_int_t
ngx_parse_request_url(ngx_request_url_t *request_url, ngx_str_t *url)
{
    u_char                     *last, *host_last;
    u_char                     *scheme;
    u_char                     *user, *host, *port;
    u_char                     *path, *args, *fragment;

    /* NULL url */
    if (url->len == 0) {
        return NGX_ERROR;
    }

    last = url->data + url->len;

    scheme = url->data;

    user = (u_char *) ngx_strnstr(scheme, "://", last - scheme);

    if (user == NULL) {
        return NGX_ERROR;
    }

    request_url->scheme.data = scheme;
    request_url->scheme.len = user - scheme;

    user += 3;
    if (user >= last) {
        return NGX_ERROR;
    }

    host_last = ngx_strlchr(user, last, '/');
    if (host_last == NULL) {
        host_last = last;
    }

    host = ngx_strlchr(user, host_last, '@');
    if (host == NULL) { /* no user */
        host = user;
    } else {
        if (user == host) { /* user is "" */
            return NGX_ERROR;
        }

        request_url->user.data = user;
        request_url->user.len = host - user;
        ++host;
        if (host == host_last) { /* no host */
            return NGX_ERROR;
        }
    }

    port = ngx_strlchr(host, host_last, ':');
    if (port == NULL) { /* no port */
        request_url->host.data = host;
        request_url->host.len = host_last - host;

        request_url->host_with_port = request_url->host;
    } else {
        request_url->host.data = host;
        request_url->host.len = port - host;
        ++port;
        if (port == host_last) { /* port error */
            return NGX_ERROR;
        }

        request_url->port.data = port;
        request_url->port.len = host_last - port;

        request_url->host_with_port.data = host;
        request_url->host_with_port.len = host_last - host;
    }

    path = ++host_last;
    if (path >= last) { /* no path */
        goto done;
    }

    args = ngx_strlchr(path, last, '?');
    if (args == NULL) { /* no args */
        request_url->path.data = path;
        request_url->path.len = last - path;

        request_url->uri_with_args = request_url->path;

        goto done;
    } else {
        request_url->path.data = path;
        request_url->path.len = args - path;
        ++args;
        if (args == last) { /* args error */
            return NGX_ERROR;
        }
    }

    fragment = ngx_strlchr(args, last, '#');
    if (fragment == NULL) { /* no fragment */
        request_url->args.data = args;
        request_url->args.len = last - args;
    } else {
        request_url->args.data = args;
        request_url->args.len = fragment - args;
        ++fragment;
        if (fragment == last) { /* fragment error */
            return NGX_ERROR;
        }

        request_url->fragment.data = fragment;
        request_url->fragment.len = last - fragment;
    }

    request_url->uri_with_args.data = path;
    request_url->uri_with_args.len = last - path;

done:
    return NGX_OK;
}

in_port_t
ngx_request_port(ngx_str_t *scheme, ngx_str_t *port)
{
    ngx_int_t                   p;
    ngx_scheme_port_t          *sp;

    if (port->len) {
        p = ngx_atoi(port->data, port->len);
        if (p < 1 || p > 65535) { /* invalid port */
            return 0;
        }

        return p;
    }

    for (sp = ngx_sheme_port; sp->port != 0; ++sp) {
        if (sp->scheme.len == scheme->len
            && ngx_memcmp(sp->scheme.data, scheme->data, scheme->len) == 0)
        {
            return sp->port;
        }
    }

    return 0;
}

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

ngx_int_t
ngx_copy_str(ngx_pool_t *pool, ngx_str_t *dst, ngx_str_t *src)
{
    if (src->len == 0) {
        return NGX_OK;
    }

    dst->len = src->len;
    dst->data = ngx_palloc(pool, src->len);
    if (dst->data == NULL) {
        return NGX_ERROR;
    }

    ngx_memcpy(dst->data, src->data, src->len);

    return NGX_OK;
}
