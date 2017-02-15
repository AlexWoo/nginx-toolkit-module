# Module nginx-timer-module
---
## Instructions

Independent timer for nginx, submodule:

- [nginx-dynamic-resolv-module](https://github.com/AlexWoo/nginx-timer-module/blob/master/dynamic_resolv.md) used nginx-timer-module to resolv domains registered in system every timeinterval

## Directives

	Syntax  : worker_timers number;
	Default : worker_timers 0;
	Context : events

Sets the maximum number of timers that can be used worker process.

	events {
		...
		worker_timers       1024;
	}

## API

**header file**

For using this API, You should include the header file as below:

	#include "ngx_event_timer_module.h"

**add timer**

	ngx_int_t ngx_event_timer_add_timer(ngx_msec_t tv,
	        ngx_timer_handler_pt h, void *data);

- return value:

	return timerid for successd, NGX_ERROR for failed

- paras:

	tv   : timer interval to trigger handler
	h    : timer handler
	data : data of h para

Register a timer handler, timer interval is tv, measured by millsecond. When timer triggered, h will be called, using data as function parameters.

h's protype is:

	typedef void (* ngx_timer_handler_pt)(void *data);

**del timer**

	void ngx_event_timer_del_timer(ngx_uint_t timerid);

- return value:

	void

- paras:

	timerid: return by ngx_event_timer_add_timer

Deregister timer handler.

## Build

cd to NGINX source directory & run this:

	./configure --add-module=/path/to/nginx-timer-module/t/ --add-module=/path/to/nginx-timer-module/
	make && make install

## Example

See t/ngx_http_timer_test_module.c as reference

Build:

	./configure --add-module=/path/to/nginx-timer-module/t/ --add-module=/path/to/nginx-timer-module/
	make && make install

Configure:

	location / {
		timer_test;
	}

Test:

	curl -v http://127.0.0.1/

Test module will start a timer when process init. It will log every 5 seconds:

	2016/12/10 18:48:37 [error] 20295#0: http timer test times 2, timerid 1

when test times > 3, use **"curl -v http://127.0.0.1/"**, the timer will deregister
