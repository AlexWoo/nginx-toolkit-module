
/*
 * Copyright (C) AlexWoo(Wu Jie) wj19840501@gmail.com
 */


#ifndef _NGX_DYNAMIC_RESOLV_MODULE_H_INCLUDED_
#define _NGX_DYNAMIC_RESOLV_MODULE_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


/*
 * add new domain in dynamic resolv
 * domain: domain for DNS query
 */
void ngx_dynamic_resolv_add_domain(ngx_str_t *domain);

/*
 * del domain from dynamic resolv
 * domain: domain for DNS query
 */
void ngx_dynamic_resolv_del_domain(ngx_str_t *domain);

/*
 * get addr from dynamic resolv by domain
 * domain: domain for DNS query
 * sa: return sockaddr
 * family: addr type wants like AF_INET AF_INET6
 * return value: sockaddr len, 0 for not found
 */
socklen_t ngx_dynamic_resolv_get_domain(ngx_str_t *domain,
        struct sockaddr *sa, int family);

/*
 * print resolver_hash
 * NGX_DEBUG only
 */
void ngx_dynamic_resolv_print();

#endif

