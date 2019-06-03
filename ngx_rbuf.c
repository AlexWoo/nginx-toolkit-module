/*
 * Copyright (C) AlexWoo(Wu Jie) wj19840501@gmail.com
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include "ngx_map.h"


static ngx_pool_t              *ngx_rbuf_pool;

static ngx_map_t                ngx_rbuf_map;
static ngx_uint_t               ngx_rbuf_nalloc_node;

static ngx_uint_t               ngx_rbuf_nalloc_chain;
static ngx_uint_t               ngx_rbuf_nfree_chain;

static ngx_map_t                ngx_rbuf_using;


typedef struct {
    ngx_map_node_t              node;
    ngx_chain_t                *free;
} ngx_chainbuf_node_t;

typedef struct {
    ngx_chain_t                 cl;
    ngx_buf_t                   buf;

#if (NGX_DEBUG)

    ngx_map_node_t              node;
    char                       *file;
    int                         line;

#endif

    size_t                      size;
    u_char                      alloc[];
} ngx_chainbuf_t;


static ngx_int_t
ngx_rbuf_init()
{
    ngx_rbuf_pool = ngx_create_pool(4096, ngx_cycle->log);
    if (ngx_rbuf_pool == NULL) {
        return NGX_ERROR;
    }

    ngx_map_init(&ngx_rbuf_map, ngx_map_hash_uint, ngx_cmp_uint);
    ngx_rbuf_nalloc_node = 0;

    ngx_rbuf_nalloc_chain = 0;
    ngx_rbuf_nfree_chain = 0;

    ngx_map_init(&ngx_rbuf_using, ngx_map_hash_uint, ngx_cmp_uint);

    return NGX_OK;
}


ngx_chain_t *
ngx_get_chainbuf_debug(size_t size, ngx_flag_t alloc_rbuf, char *file, int line)
{
    ngx_chainbuf_node_t        *cn;
    ngx_map_node_t             *node;
    ngx_chainbuf_t             *cb;
    ngx_chain_t                *cl;

    if (ngx_rbuf_pool == NULL) {
        ngx_rbuf_init();
    }

    node = ngx_map_find(&ngx_rbuf_map, size);
    if (node == NULL) { /* new size */
        cn = ngx_pcalloc(ngx_rbuf_pool, sizeof(ngx_chainbuf_node_t));
        if (cn == NULL) {
            return NULL;
        }

        node = &cn->node;
        node->raw_key = size;
        ngx_map_insert(&ngx_rbuf_map, node, 0);

        ++ngx_rbuf_nalloc_node;
    } else {
        cn = (ngx_chainbuf_node_t *) node;
    }

    cl = cn->free;
    if (cl) {
        cn->free = cl->next;
        cl->next = NULL;
        cb = (ngx_chainbuf_t *) cl;

        --ngx_rbuf_nfree_chain;
    } else {
        cb = ngx_pcalloc(ngx_rbuf_pool, sizeof(ngx_chainbuf_t) + size);
        if (cb == NULL) {
            return NULL;
        }

        cl = &cb->cl;
        cl->buf = &cb->buf;
        cb->size = size;

        ++ngx_rbuf_nalloc_chain;
    }

    cb->buf.last = cb->buf.pos = cb->buf.start = cb->alloc;
    cb->buf.end = cb->alloc + size;
    cb->buf.memory = 1;

#if (NGX_DEBUG)

    // record chainbuf in using map
    cb->file = file;
    cb->line = line;
    cb->node.raw_key = (intptr_t) cl;
    ngx_map_insert(&ngx_rbuf_using, &cb->node, 0);

#endif

    return cl;
}

void
ngx_put_chainbuf_debug(ngx_chain_t *cl, char *file, int line)
{
    ngx_chainbuf_node_t        *cn;
    ngx_map_node_t             *node;
    ngx_chainbuf_t             *cb;


    if (ngx_rbuf_pool == NULL) {
        return;
    }

    if (cl == NULL) {
        return;
    }

    cb = (ngx_chainbuf_t *) cl;
    node = ngx_map_find(&ngx_rbuf_map, cb->size);
    if (node == NULL) {
        return;
    }

    cn = (ngx_chainbuf_node_t *) node;
    cl->next = cn->free;
    cn->free = cl;

    ++ngx_rbuf_nfree_chain;

#if (NGX_DEBUG)

    // delete chainbuf from using map
    if (ngx_map_find(&ngx_rbuf_using, (intptr_t) cl) == NULL) {
        ngx_log_error(NGX_LOG_EMERG, ngx_cycle->log, 0,
                "destroy chainbuf twice: %s:%d", file, line);
        return;
    }
    ngx_map_delete(&ngx_rbuf_using, (intptr_t) cl);

#endif
}

ngx_chain_t *
ngx_rbuf_state(ngx_http_request_t *r, unsigned detail)
{
    ngx_chain_t                *cl;
    ngx_buf_t                  *b;
    size_t                      len;
#if (NGX_DEBUG)
    size_t                      len1;
    ngx_uint_t                  n;
    ngx_chainbuf_t             *cb;
    ngx_map_node_t             *node;
#endif

    len = sizeof("##########ngx rbuf state##########\n") - 1
        + sizeof("ngx_rbuf nalloc node: \n") - 1 + NGX_OFF_T_LEN
        + sizeof("ngx_rbuf nalloc chain: \n") - 1 + NGX_OFF_T_LEN
        + sizeof("ngx_rbuf nfree chain: \n") - 1 + NGX_OFF_T_LEN;

#if (NGX_DEBUG)

    if (detail) {
        n = ngx_rbuf_nalloc_chain - ngx_rbuf_nfree_chain;
        /* "    file:line\n" */
        len1 = 4 + 256 + 1 + NGX_OFF_T_LEN + 1;
        len += len1 * n;
    }

#endif

    cl = ngx_alloc_chain_link(r->pool);
    if (cl == NULL) {
        return NULL;
    }
    cl->next = NULL;

    b = ngx_create_temp_buf(r->pool, len);
    if (b == NULL) {
        return NULL;
    }
    cl->buf = b;

    b->last = ngx_snprintf(b->last, len,
            "##########ngx rbuf state##########\nngx_rbuf nalloc node: %ui\n"
            "ngx_rbuf nalloc chain: %ui\nngx_rbuf nfree chain: %ui\n",
            ngx_rbuf_nalloc_node, ngx_rbuf_nalloc_chain, ngx_rbuf_nfree_chain);

#if (NGX_DEBUG)

    if (detail) {
        for (node = ngx_map_begin(&ngx_rbuf_using); node;
                node = ngx_map_next(node))
        {
            cb = (ngx_chainbuf_t *) ((char *) node
                    - offsetof(ngx_chainbuf_t, node));
            b->last = ngx_snprintf(b->last, len1, "    %s:%d\n",
                    cb->file, cb->line);
        }
    }

#endif

    return cl;
}
