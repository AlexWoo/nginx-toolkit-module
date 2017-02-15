# Module nginx-dynamic-resolv-module
---
## Instructions

Resolv the domains user registed every interval time

## Directives

	Syntax  : resolver_timeout time;
	Default : resolver_timeout 30s;
	Context : events

Set the timeout time for DNS query.

	Syntax  : dynamic_resolver dns_server time;
	Default : None;
	Context : events

Set time interval for DNS query frequency, 0 for shutdown this function

	Syntax  : resolver_hash_bucket_size num;
	Default : resolver_hash_bucket_size 32;
	Context : events

Set hash table bucket size for storing domains user registed

Example:

	events {
		...
		dynamic_resolver       60s;
		resolver_timeout			30s;
		resolver_hash_bucket_size 64
	}

## API

**header file**

For using this API, You should include the header file as below:

	#include "ngx_event_timer_module.h"

**registe domain**

	void ngx_dynamic_resolv_add_domain(ngx_str_t *domain);

- paras:

	domain: domain for resolv

**deregiste domain**

	void ngx_dynamic_resolv_del_domain(ngx_str_t *domain);

- paras:

	domain: domain registed int dynamic resolv module

**get address for domain**

	socklen_t ngx_dynamic_resolv_get_domain(ngx_str_t *domain,
        struct sockaddr *sa, int family);

- return value:

	sucessed, return sockaddr length; failed, return 0

- paras:

	- domain: domain for quering
	- sa    : return sockaddr
	- family: addr type wants like AF\_INET AF\_INET6

## Build

cd to NGINX source directory & run this:

	./configure --add-module=/path/to/nginx-timer-module/
	make && make install

## Example

See t/ngx\_http\_dr\_test\_module.c as reference

Build:

	./configure --with-debug --add-module=/path/to/nginx-timer-module/ --add-module=/path/to/nginx-timer-module/t
	make && make install

Configure:

	events {
		dynamic_resolver       8.8.8.8 60s;
		...
	}
	
	http {
	
		...
	
		server {
	
			...
			
			location /dynamic_test {
				dynamic_test;
			}
		}
	}

Test:

- add domain for resolving

		curl -v 'http://127.0.0.1/dynamic_test?op=add&domain=www.baidu.com'

- del domain for resolving

		curl -v 'http://127.0.0.1/dynamic_test?op=del&domain=www.baidu.com'

- query domain for resolving

		curl -v 'http://127.0.0.1/dynamic_test?op=query&domain=www.baidu.com&type=ipv4'

	
