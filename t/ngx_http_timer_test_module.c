#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include "../ngx_event_timer_module.h"


static ngx_int_t ngx_http_timer_test_init_worker(ngx_cycle_t *cycle);

static char *ngx_http_timer_test(ngx_conf_t *cf, ngx_command_t *cmd,
                                 void *conf);


static ngx_uint_t ngx_http_timer_call_times = 0;
static ngx_uint_t ngx_http_timer_timerid;

static ngx_command_t  ngx_http_timer_test_commands[] = {

    { ngx_string("timer_test"),
      NGX_HTTP_LOC_CONF|NGX_CONF_NOARGS,
      ngx_http_timer_test,
      0,
      0,
      NULL },

      ngx_null_command
};


static ngx_http_module_t  ngx_http_timer_test_module_ctx = {
    NULL,                               /* preconfiguration */
    NULL,                               /* postconfiguration */

    NULL,                               /* create main configuration */
    NULL,                               /* init main configuration */

    NULL,                               /* create server configuration */
    NULL,                               /* merge server configuration */

    NULL,                               /* create location configuration */
    NULL                                /* merge location configuration */
};


ngx_module_t  ngx_http_timer_test_module = {
    NGX_MODULE_V1,
    &ngx_http_timer_test_module_ctx,    /* module context */
    ngx_http_timer_test_commands,       /* module directives */
    NGX_HTTP_MODULE,                    /* module type */
    NULL,                               /* init master */
    NULL,                               /* init module */
    ngx_http_timer_test_init_worker,    /* init process */
    NULL,                               /* init thread */
    NULL,                               /* exit thread */
    NULL,                               /* exit process */
    NULL,                               /* exit master */
    NGX_MODULE_V1_PADDING
};


static ngx_int_t
ngx_http_timer_test_handler(ngx_http_request_t *r)
{
    ngx_chain_t                     cl;
    ngx_buf_t                      *b;
    size_t                          len;
    ngx_int_t                       rc;

    ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0,
                  "http timer test handler");

    if (ngx_http_timer_call_times > 3) {
        ngx_event_timer_del_timer(ngx_http_timer_timerid);
    }

    len = sizeof("timer trigger 2147483647 times\n") - 1;

    r->headers_out.status = NGX_HTTP_OK;

    rc = ngx_http_send_header(r);
    if (rc == NGX_ERROR || rc > NGX_OK || r->header_only) {
        return rc;
    }

    b = ngx_create_temp_buf(r->pool, len);

    if (b == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    b->last = ngx_snprintf(b->last, len, "timer trigger %i times\n",
                           ngx_http_timer_call_times);
    b->last_buf = 1;
    b->last_in_chain = 1;

    cl.buf = b;
    cl.next = NULL;

    return ngx_http_output_filter(r, &cl);
}


static void
ngx_http_timer_test_timer_handler(void *data)
{
    ++ngx_http_timer_call_times;

    ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0,
                  "http timer test times %i, timerid %i",
                  ngx_http_timer_call_times, ngx_http_timer_timerid);

    ngx_http_timer_timerid =
        ngx_event_timer_add_timer(5000, ngx_http_timer_test_timer_handler, NULL);
}


static char *
ngx_http_timer_test(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_core_loc_conf_t  *clcf;

    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = ngx_http_timer_test_handler;

    return NGX_CONF_OK;
}


static ngx_int_t
ngx_http_timer_test_init_worker(ngx_cycle_t *cycle)
{
    ngx_http_timer_timerid =
        ngx_event_timer_add_timer(5000, ngx_http_timer_test_timer_handler, NULL);

    ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0,
                  "ngx http timer test init worker");

    return NGX_OK;
}

