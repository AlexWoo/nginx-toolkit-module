/*
 * Copyright (C) AlexWoo(Wu Jie) wj19840501@gmail.com
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>
#include "ngx_event_timer_module.h"


static ngx_int_t ngx_dynamic_resolv_process_init(ngx_cycle_t *cycle);

static void *ngx_dynamic_resolv_create_conf(ngx_cycle_t *cycle);
static char *ngx_dynamic_resolv_init_conf(ngx_cycle_t *cycle, void *conf);

static char *ngx_dynamic_resolv_resolver(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);


#define MAX_DOMAIN_LEN      128
#define MAX_ADDR            8


typedef struct ngx_dynamic_resolv_ctx_s ngx_dynamic_resolv_ctx_t;

typedef struct {
    struct sockaddr             sockaddr;
    socklen_t                   socklen;
    u_short                     priority;
    u_short                     weight;
} ngx_dynamic_resolv_addr_t;

struct ngx_dynamic_resolv_ctx_s {
    ngx_str_t                   domain;
    u_char                      domain_cstr[MAX_DOMAIN_LEN];

    ngx_uint_t                  naddrs;
    ngx_dynamic_resolv_addr_t   addrs[MAX_ADDR];

    ngx_resolver_ctx_t         *ctx;
    ngx_dynamic_resolv_ctx_t   *next;
};

typedef struct {
    ngx_msec_t                  resolver_timeout;
    ngx_msec_t                  dynamic_resolver;
    ngx_resolver_t             *resolver;

    ngx_uint_t                  resolver_hash_bucket_size;
    ngx_dynamic_resolv_ctx_t  **resolver_hash;

    ngx_dynamic_resolv_ctx_t   *free;
} ngx_dynamic_resolv_conf_t;


static ngx_str_t dynamic_resolv_name = ngx_string("dynamic_resolv");


static ngx_command_t ngx_dynamic_resolv_commands[] = {

    { ngx_string("resolver_timeout"),
      NGX_EVENT_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_msec_slot,
      0,
      offsetof(ngx_dynamic_resolv_conf_t, resolver_timeout),
      NULL },

    { ngx_string("dynamic_resolver"),
      NGX_EVENT_CONF|NGX_CONF_TAKE2,
      ngx_dynamic_resolv_resolver,
      0,
      0,
      NULL },

    { ngx_string("resolver_hash_bucket_size"),
      NGX_EVENT_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_num_slot,
      0,
      offsetof(ngx_dynamic_resolv_conf_t, resolver_hash_bucket_size),
      NULL },

      ngx_null_command
};


ngx_event_module_t  ngx_dynamic_resolv_module_ctx = {
    &dynamic_resolv_name,
    ngx_dynamic_resolv_create_conf,         /* create configuration */
    ngx_dynamic_resolv_init_conf,           /* init configuration */

    { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};


/* this module use ngx_cycle->log */
ngx_module_t  ngx_dynamic_resolv_module = {
    NGX_MODULE_V1,
    &ngx_dynamic_resolv_module_ctx,         /* module context */
    ngx_dynamic_resolv_commands,            /* module directives */
    NGX_EVENT_MODULE,                       /* module type */
    NULL,                                   /* init master */
    NULL,                                   /* init module */
    ngx_dynamic_resolv_process_init,        /* init process */
    NULL,                                   /* init thread */
    NULL,                                   /* exit thread */
    NULL,                                   /* exit process */
    NULL,                                   /* exit master */
    NGX_MODULE_V1_PADDING
};


static void *
ngx_dynamic_resolv_create_conf(ngx_cycle_t *cycle)
{
    ngx_dynamic_resolv_conf_t      *conf;

    conf = ngx_pcalloc(cycle->pool, sizeof(ngx_dynamic_resolv_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    conf->dynamic_resolver = NGX_CONF_UNSET_MSEC;
    conf->resolver_timeout = NGX_CONF_UNSET_MSEC;
    conf->resolver_hash_bucket_size = NGX_CONF_UNSET_UINT;

    return conf;
}

static char *
ngx_dynamic_resolv_init_conf(ngx_cycle_t *cycle, void *conf)
{
    ngx_dynamic_resolv_conf_t      *drcf = conf;

    ngx_conf_init_msec_value(drcf->dynamic_resolver, 0);
    ngx_conf_init_msec_value(drcf->resolver_timeout, 30000);
    ngx_conf_init_uint_value(drcf->resolver_hash_bucket_size, 32);

    if (drcf->dynamic_resolver > 0 && drcf->resolver_hash_bucket_size > 0) {
        drcf->resolver_hash = ngx_pcalloc(cycle->pool,
                sizeof(ngx_dynamic_resolv_ctx_t *) *
                drcf->resolver_hash_bucket_size);
    }

    return NGX_CONF_OK;
}

static char *
ngx_dynamic_resolv_resolver(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_dynamic_resolv_conf_t      *drcf = conf;

    ngx_str_t                      *value;

    if (drcf->resolver) {
        return "is duplicate";
    }

    value = cf->args->elts;

    drcf->resolver = ngx_resolver_create(cf, &value[1], cf->args->nelts - 2);
    if (drcf->resolver == NULL) {
        return NGX_CONF_ERROR;
    }

    drcf->dynamic_resolver = ngx_parse_time(&value[2], 0);
    if (drcf->dynamic_resolver == (ngx_msec_t) NGX_ERROR) {
        return "invalid value";
    }

    return NGX_CONF_OK;
}


static void
ngx_dynamic_resolv_resolver_handler(ngx_resolver_ctx_t *ctx)
{
    ngx_dynamic_resolv_ctx_t       *dynamic_resolv_ctx;
    ngx_uint_t                      i;

    dynamic_resolv_ctx = ctx->data;

    if (dynamic_resolv_ctx == NULL) {
        ngx_log_error(NGX_LOG_INFO, ngx_cycle->log, 0,
                      "dynamic resolv, %V has been deleted", &ctx->name);
        goto failed;
    }

    if (ctx->state) {
        ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0,
                      "dynamic resolv, %V could not be resolved (%i: %s)",
                      &ctx->name, ctx->state,
                      ngx_resolver_strerror(ctx->state));
        goto failed;
    }

    dynamic_resolv_ctx->naddrs = ngx_min(ctx->naddrs, MAX_ADDR);
    for (i = 0; i < ctx->naddrs && i < MAX_ADDR; ++i) {
        ngx_memcpy(&dynamic_resolv_ctx->addrs[i].sockaddr,
                ctx->addrs[i].sockaddr, ctx->addrs[i].socklen);
        dynamic_resolv_ctx->addrs[i].socklen = ctx->addrs[i].socklen;
        dynamic_resolv_ctx->addrs[i].priority = ctx->addrs[i].priority;
        dynamic_resolv_ctx->addrs[i].weight = ctx->addrs[i].weight;
    }

failed:
    if (dynamic_resolv_ctx) {
        dynamic_resolv_ctx->ctx = NULL;
    }
    ngx_resolve_name_done(ctx);
}

static void
ngx_dynamic_resolv_resolver_start(ngx_dynamic_resolv_ctx_t *dynamic_resolv_ctx)
{
    ngx_dynamic_resolv_conf_t      *drcf;
    ngx_resolver_ctx_t             *ctx, temp;

    drcf = ngx_event_get_conf(ngx_cycle->conf_ctx, ngx_dynamic_resolv_module);

    temp.name = dynamic_resolv_ctx->domain;

    ctx = ngx_resolve_start(drcf->resolver, &temp);
    if (ctx == NULL) {
        goto failed;
    }

    if (ctx == NGX_NO_RESOLVER) {
        ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0, "dynamic resolv, "
            "no resolver defined to resolv %V", &dynamic_resolv_ctx->domain);
        goto failed;
    }

    ctx->name = dynamic_resolv_ctx->domain;
    ctx->handler = ngx_dynamic_resolv_resolver_handler;
    ctx->data = dynamic_resolv_ctx;
    ctx->timeout = drcf->resolver_timeout;
    dynamic_resolv_ctx->ctx = ctx;

    if (ngx_resolve_name(ctx) != NGX_OK) { /* send DNS query here */
        ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0, "dynamic resolv, "
            "resolv %V failed", &dynamic_resolv_ctx->domain);
        goto failed;
    }

    return;

failed:
    if (ctx == NULL || ctx == NGX_NO_RESOLVER) {
        dynamic_resolv_ctx->ctx = NULL;
        ngx_resolve_name_done(ctx);
    }
}

static void
ngx_dynamic_resolv_start_resolver(void *data)
{
    ngx_dynamic_resolv_conf_t      *drcf;
    ngx_dynamic_resolv_ctx_t       *dynamic_resolv_ctx;
    ngx_uint_t                      i;

    drcf = ngx_event_get_conf(ngx_cycle->conf_ctx, ngx_dynamic_resolv_module);

    for (i = 0; i < drcf->resolver_hash_bucket_size; ++i) {
        dynamic_resolv_ctx = drcf->resolver_hash[i];
        while (dynamic_resolv_ctx) {
            ngx_dynamic_resolv_resolver_start(dynamic_resolv_ctx);
            dynamic_resolv_ctx = dynamic_resolv_ctx->next;
        }
    }

    ngx_event_timer_add_timer(drcf->dynamic_resolver,
            ngx_dynamic_resolv_start_resolver, NULL);
}

static ngx_int_t
ngx_dynamic_resolv_process_init(ngx_cycle_t *cycle)
{
    ngx_dynamic_resolv_conf_t      *drcf;

    drcf = ngx_event_get_conf(ngx_cycle->conf_ctx, ngx_dynamic_resolv_module);
    if (drcf->dynamic_resolver == 0) {
        return NGX_OK;
    }

    ngx_event_timer_add_timer(drcf->dynamic_resolver,
            ngx_dynamic_resolv_start_resolver, NULL);

    return NGX_OK;
}


static ngx_dynamic_resolv_ctx_t *
ngx_dynamic_resolv_get_ctx(ngx_dynamic_resolv_conf_t *drcf)
{
    ngx_dynamic_resolv_ctx_t       *ctx;

    ctx = drcf->free;

    if (ctx == NULL) {
        ctx = ngx_pcalloc(ngx_cycle->pool, sizeof(ngx_dynamic_resolv_ctx_t));

        if (ctx == NULL) {
            ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0,
                    "dynamic resolv, alloc memory dynamic resolv ctx failed");
            return NULL;
        }
    } else {
        drcf->free = drcf->free->next;
        ngx_memzero(ctx, sizeof(ngx_dynamic_resolv_ctx_t));
    }

    return ctx;
}

static void
ngx_dynamic_resolv_put_ctx(ngx_dynamic_resolv_ctx_t *ctx,
        ngx_dynamic_resolv_conf_t *drcf)
{
    ctx->next = drcf->free;
    drcf->free = ctx;
}

void
ngx_dynamic_resolv_add_domain(ngx_str_t *domain)
{
    ngx_dynamic_resolv_conf_t      *drcf;
    ngx_dynamic_resolv_ctx_t       *ctx;
    ngx_uint_t                      idx;

    drcf = ngx_event_get_conf(ngx_cycle->conf_ctx, ngx_dynamic_resolv_module);
    if (drcf->dynamic_resolver == 0) {
        return;
    }

    idx = ngx_hash_key_lc(domain->data, domain->len);
    idx %= drcf->resolver_hash_bucket_size;

    ngx_log_error(NGX_LOG_INFO, ngx_cycle->log, 0, "dynamic resolv, "
                  "prepare add %V in %d slot", domain, idx);

    /* domain already exist */
    ctx = drcf->resolver_hash[idx];
    while (ctx) {
        if (ctx->domain.len == domain->len &&
            ngx_memcmp(ctx->domain.data, domain->data, domain->len) == 0)
        {
            ngx_log_error(NGX_LOG_INFO, ngx_cycle->log, 0,
                          "dynamic resolv, %V already exist", domain);
            return;
        }
        ctx = ctx->next;
    }

    ctx = ngx_dynamic_resolv_get_ctx(drcf);
    if (ctx == NULL) {
        ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0,
                      "dynamic resolv, get resolv ctx failed");
        return;
    }

    *ngx_copy(ctx->domain_cstr, domain->data, domain->len) = 0;
    ctx->domain.data = ctx->domain_cstr;
    ctx->domain.len = domain->len;

    /* link dynamic_resolv_ctx to hash table */
    ctx->next = drcf->resolver_hash[idx];
    drcf->resolver_hash[idx] = ctx;

    /* query DNS right now */
    if (ngx_process != NGX_PROCESS_WORKER) {
        return;
    }

    ngx_dynamic_resolv_resolver_start(ctx);
}

void
ngx_dynamic_resolv_del_domain(ngx_str_t *domain)
{
    ngx_dynamic_resolv_conf_t      *drcf;
    ngx_dynamic_resolv_ctx_t      **pctx, *ctx;
    ngx_uint_t                      idx;

    drcf = ngx_event_get_conf(ngx_cycle->conf_ctx, ngx_dynamic_resolv_module);
    if (drcf->dynamic_resolver == 0) {
        return;
    }

    idx = ngx_hash_key_lc(domain->data, domain->len);
    idx %= drcf->resolver_hash_bucket_size;

    ngx_log_error(NGX_LOG_INFO, ngx_cycle->log, 0, "dynamic resolv, "
                  "prepare del %V in %d slot", domain, idx);

    for (pctx = &drcf->resolver_hash[idx]; *pctx; pctx = &(*pctx)->next) {
        if ((*pctx)->domain.len == domain->len &&
            ngx_memcmp((*pctx)->domain.data, domain->data, domain->len) == 0)
        {
            ctx = *pctx;
            *pctx = (*pctx)->next;
            if (ctx->ctx) {
                ctx->ctx->data = NULL;
            }
            ngx_dynamic_resolv_put_ctx(ctx, drcf);
            return;
        }
    }

    ngx_log_error(NGX_LOG_INFO, ngx_cycle->log, 0, "dynamic resolv, "
                  "%V is not in dynamic resolv hash table", domain);
}

socklen_t
ngx_dynamic_resolv_get_domain(ngx_str_t *domain,
        struct sockaddr *sa, int family)
{
    ngx_dynamic_resolv_conf_t      *drcf;
    ngx_dynamic_resolv_ctx_t       *ctx;
    ngx_uint_t                      idx, i;

    drcf = ngx_event_get_conf(ngx_cycle->conf_ctx, ngx_dynamic_resolv_module);
    if (drcf->dynamic_resolver == 0) {
        return 0;
    }

    idx = ngx_hash_key_lc(domain->data, domain->len);
    idx %= drcf->resolver_hash_bucket_size;

    ctx = drcf->resolver_hash[idx];
    while (ctx) {
        if (ctx->domain.len == domain->len &&
            ngx_memcmp(ctx->domain.data, domain->data, domain->len) == 0)
        {
            break;
        }

        ctx = ctx->next;
    }

    if (ctx == NULL) { /* not found */
        ngx_log_error(NGX_LOG_INFO, ngx_cycle->log, 0, "dynamic resolv, "
                      "%V is not in dynamic resolv hash table", domain);
        return 0;
    }

    for (i = 0; i < ctx->naddrs; ++i) {
        if (ctx->addrs[i].sockaddr.sa_family == family) {
            ngx_memcpy(sa, &ctx->addrs[i].sockaddr, ctx->addrs[i].socklen);
            return ctx->addrs[i].socklen;
        }
    }

    ngx_log_error(NGX_LOG_INFO, ngx_cycle->log, 0, "dynamic resolv, "
                  "%V[%d] is not in dynamic resolv hash table", domain, family);
    return 0;
}

void ngx_dynamic_resolv_print()
{
#if (NGX_DEBUG)
    ngx_dynamic_resolv_conf_t      *drcf;
    ngx_dynamic_resolv_ctx_t       *ctx;
    ngx_uint_t                      i, j;
    u_char                          text[NGX_SOCKADDR_STRLEN];

    drcf = ngx_event_get_conf(ngx_cycle->conf_ctx, ngx_dynamic_resolv_module);
    if (drcf->dynamic_resolver == 0) {
        return;
    }

    ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0, "[DYNAMIC RESOLV] print");

    for (i = 0; i < drcf->resolver_hash_bucket_size; ++i) {
        if (drcf->resolver_hash[i]) {
            ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0,
                          "[DYNAMIC RESOLV]   idx:%ui", i);
            ctx = drcf->resolver_hash[i];
            while (ctx) {
                ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0,
                    "[DYNAMIC RESOLV]     %p, domain:%V, naddrs:%ui, next:%p",
                    ctx, &ctx->domain, ctx->naddrs, ctx->next);
                for (j = 0; j < ctx->naddrs; ++j) {
                    ngx_memzero(text, NGX_SOCKADDR_STRLEN);
                    ngx_sock_ntop(&ctx->addrs[j].sockaddr,
                        ctx->addrs[j].socklen, text, NGX_SOCKADDR_STRLEN, 0);
                    ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0,
                        "[DYNAMIC RESOLV]       %s, pri:%d, wei:%d",
                        text, ctx->addrs[j].priority, ctx->addrs[j].weight);
                }
                ctx = ctx->next;
            }
        }
    }
#endif
}

