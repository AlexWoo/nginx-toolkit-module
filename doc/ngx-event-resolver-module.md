# Module nginx-event-resolver-module
---
## Instructions

Common resovler in event modules, just like http resolver, stream resolver in nginx, but can be used by http modules, stream modules and other modules

## Directives

	Syntax  : resolver address ... [valid=time] [ipv6=on|off];
	Default : -
	Context : events

Configures name servers used to resolve names of upstream servers into addresses, for example:

> resolver 127.0.0.1 [::1]:5353;

An address can be specified as a domain name or IP address, and an optional port (1.3.1, 1.2.2). If port is not specified, the port 53 is used. Name servers are queried in a round-robin fashion.

> Before version 1.1.7, only a single name server could be configured. Specifying name servers using IPv6 addresses is supported starting from versions 1.3.1 and 1.2.2.

By default, nginx will look up both IPv4 and IPv6 addresses while resolving. If looking up of IPv6 addresses is not desired, the ipv6=off parameter can be specified.

> Resolving of names into IPv6 addresses is supported starting from version 1.5.8.

By default, nginx caches answers using the TTL value of a response. An optional valid parameter allows overriding it:

> resolver 127.0.0.1 [::1]:5353 valid=30s;

Before version 1.1.9, tuning of caching time was not possible, and nginx always cached answers for the duration of 5 minutes.
To prevent DNS spoofing, it is recommended configuring DNS servers in a properly secured trusted local network.

	Syntax  : resolver_timeout time;
	Default : resolver_timeout 60s;
	Context : events

Sets a timeout for name resolution, for example:

> resolver_timeout 5s;

Example:

	events {
		resolver                  192.168.84.254 valid=20s;
		resolver_timeout          10s;
	}

## API

**header file**

For using this API, You should include the header file as below:

	#include "ngx_event_resolver.h"

**resolver a domain**

	void ngx_event_resolver_start_resolver(ngx_str_t *domain,
        ngx_event_resolver_handler_pt h, void *data);

- return value:

	None

- paras:

	- domain: domain for resolving
	- h     : callback handler
	- data  : data for callback

h's protype is:

	typedef void (* ngx_event_resolver_handler_pt)(void *data,
        ngx_resolver_addr_t *addrs, ngx_uint_t naddrs);

- return value:

	None

- paras:

	- data  : user private data set in ngx_event_resolver_start_resolver
	- addrs : addrs resolv by DNS
	- naddrs: number of addrs resolv by DNS


## Build

cd to NGINX source directory & run this:

	./configure --add-module=/path/to/nginx-timer-module/
	make && make install

## Example

See t/ngx\_event\_resolver\_test\_module.c as reference

Build:

	./configure --with-debug --add-module=/path/to/nginx-timer-module/ --add-module=/path/to/nginx-timer-module/t
	make && make install

Configure:

	events {
		resolver                  192.168.84.254 114.114.114.114 valid=20s;
		resolver_timeout          10s;
	}
	
	http {
	
		...
	
		server {
	
			...
			
			location /event_resolver_test/ {
				event_resolver_test;
			}
		}
	}

Install bind server

	/path/to/nginx-timer-module/t/dns_install.sh
	
modify /var/named/test.com.zone dns ip address to fit your enviroment

Test:

- add domain for resolving

		curl -v "http://127.0.0.1/event_resolver_test/domain?domain=www.test.com"
