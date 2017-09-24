/*
 * Copyright (C) AlexWoo(Wu Jie) wj19840501@gmail.com
 */


#ifndef _NGX_RBUF_H_INCLUDED_
#define _NGX_RBUF_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


/*
 * paras:
 *      size: buffer size for allocate
 *      alloc_rbuf: whether alloc rbuf
 * return:
 *      nginx chain
 */
ngx_chain_t *ngx_get_chainbuf(size_t size, ngx_flag_t alloc_rbuf);

/*
 * paras:
 *      cl: nginx chain return by ngx_rtmp_shared_get_chainbuf
 */
void ngx_put_chainbuf(ngx_chain_t *cl);

/*
 * paras:
 *      r: http request to query status of rbuf
 */
ngx_chain_t *ngx_rbuf_state(ngx_http_request_t *r);

#endif
