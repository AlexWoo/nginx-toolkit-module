#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include "../ngx_dynamic_resolv_module.h"


static char *ngx_http_dr_test(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);


static ngx_command_t  ngx_http_dr_test_commands[] = {

    { ngx_string("dynamic_test"),
      NGX_HTTP_LOC_CONF|NGX_CONF_NOARGS,
      ngx_http_dr_test,
      0,
      0,
      NULL },

      ngx_null_command
};


static ngx_http_module_t  ngx_http_dr_test_module_ctx = {
    NULL,                                   /* preconfiguration */
    NULL,                                   /* postconfiguration */

    NULL,                                   /* create main configuration */
    NULL,                                   /* init main configuration */

    NULL,                                   /* create server configuration */
    NULL,                                   /* merge server configuration */

    NULL,                                   /* create location configuration */
    NULL                                    /* merge location configuration */
};


ngx_module_t  ngx_http_dr_test_module = {
    NGX_MODULE_V1,
    &ngx_http_dr_test_module_ctx, /* module context */
    ngx_http_dr_test_commands,    /* module directives */
    NGX_HTTP_MODULE,                        /* module type */
    NULL,                                   /* init master */
    NULL,                                   /* init module */
    NULL,                                   /* init process */
    NULL,                                   /* init thread */
    NULL,                                   /* exit thread */
    NULL,                                   /* exit process */
    NULL,                                   /* exit master */
    NGX_MODULE_V1_PADDING
};


static ngx_int_t
ngx_http_dr_test_handler(ngx_http_request_t *r)
{
    ngx_chain_t                     cl;
    ngx_buf_t                      *b;
    size_t                          len;
    ngx_int_t                       rc;
    ngx_str_t                       domain, op, type;
    socklen_t                       socklen;
    struct sockaddr                 sa;
    int                             family;
    u_char                          ret[NGX_SOCKADDR_STRLEN];

    ngx_memzero(ret, NGX_SOCKADDR_STRLEN);

    if (ngx_http_arg(r, (u_char *) "op", 2, &op) != NGX_OK) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "http dynamic resolv, no op in http args");
        return NGX_HTTP_BAD_REQUEST;
    }

    if (ngx_http_arg(r, (u_char *) "domain", 6, &domain) != NGX_OK) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "http dynamic resolv, no domain in http args");
        return NGX_HTTP_BAD_REQUEST;
    }

    if (op.len == 3) {

        if (ngx_memcmp(op.data, "add", op.len) == 0) {

            ngx_dynamic_resolv_add_domain(&domain);
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                          "http dynamic resolv, add domain: %V", &domain);
        } else if (ngx_memcmp(op.data, "del", op.len) == 0) {

            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                          "http dynamic resolv, del domain: %V", &domain);
            ngx_dynamic_resolv_del_domain(&domain);
        } else {

            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                          "http dynamic resolv, unkown op: %V", &op);
            return NGX_HTTP_BAD_REQUEST;
        }
        *ngx_snprintf(ret, NGX_SOCKADDR_STRLEN, "OK") = 0;
    } else if (op.len == 5 && ngx_memcmp(op.data, "query", op.len) == 0) {

        if (ngx_http_arg(r, (u_char *) "type", 4, &type) != NGX_OK) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                          "http dynamic resolv, no type in http args");
            return NGX_HTTP_BAD_REQUEST;
        }

        if (type.len == 4 && ngx_memcmp("ipv6", type.data, type.len) == 0) {
            family = AF_INET6;
        } else {
            family = AF_INET;
        }
        socklen = ngx_dynamic_resolv_get_domain(&domain, &sa, family);

        if (socklen == 0) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                          "http dynamic resolv, get domain failed");
            *ngx_snprintf(ret, NGX_SOCKADDR_STRLEN, "None") = 0;
        } else {
            ngx_sock_ntop(&sa, socklen, ret, NGX_SOCKADDR_STRLEN, 0);
        }
    } else {

        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "http dynamic resolv, unkown op: %V", &op);
        return NGX_HTTP_BAD_REQUEST;
    }

    ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0,
                  "http dynamic resolv test handler");
    ngx_dynamic_resolv_print();

    r->headers_out.status = NGX_HTTP_OK;

    rc = ngx_http_send_header(r);
    if (rc == NGX_ERROR || rc > NGX_OK || r->header_only) {
        return rc;
    }

    len = ngx_strlen(ret);
    ++len; /* add \n */
    b = ngx_create_temp_buf(r->pool, len);

    if (b == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    b->last = ngx_snprintf(b->last, len, "%s\n", ret);
    b->last_buf = 1;
    b->last_in_chain = 1;

    cl.buf = b;
    cl.next = NULL;

    return ngx_http_output_filter(r, &cl);
}


static char *
ngx_http_dr_test(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_core_loc_conf_t  *clcf;

    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = ngx_http_dr_test_handler;

    return NGX_CONF_OK;
}

