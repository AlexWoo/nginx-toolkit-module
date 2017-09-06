# Module ngx-dynamic-conf-module
---
## Instructions

System will reload conf when nginx dynamic config file change. Developer can use this module to reload file without reload nginx worker.

Now it only support NGX_CORE_MODULE

## Directives

	Syntax  : dynamic_conf dynamic_file time;
	Default : -
	Context : main

Set dynamic config file and interval system checked file changed.

	Syntax  : dynamic_log log_file level;
	Default : -
	Context : main

Set dynamic conf load log file and log level. If not configured, use cycle log as default.

Example:

	dynamic_conf    conf/nginx_dynamic.conf 10;
	dynamic_log     logs/error_dynamic.log  info;

## API

**header file**

For using this API, You should include the header file as below:

	#include "ngx_dynamic_conf.h"

**dynamic module define**

	typedef struct {
	    ngx_str_t               name;
	    void                 *(*create_conf)(ngx_conf_t *cf);
	    char                 *(*init_conf)(ngx_conf_t *cf, void *conf);
	} ngx_dynamic_core_module_t;

dynamic conf module define as below

	ngx_module_t  ngx_test_module = {
	    NGX_MODULE_V1,
	    &ngx_test_module_ctx,                   /* module context */
	    ngx_test_commands,                      /* module directives */
	    NGX_CORE_MODULE,                        /* module type */
	    NULL,                                   /* init master */
	    NULL,                                   /* init module */
	    NULL,                                   /* init process */
	    NULL,                                   /* init thread */
	    NULL,                                   /* exit thread */
	    NULL,                                   /* exit process */
	    NULL,                                   /* exit master */
	    (uintptr_t) &ngx_test_module_dctx,      /* module dynamic context */
	    (uintptr_t) ngx_test_dcommands,         /* module dynamic directives */
	    NGX_MODULE_V1_DYNAMIC_PADDING
	};

**module dynamic context** struct define as above, **module dynamic directives** define use ngx\_command\_t. Use ngx\_dynamic\_core\_test\_module define in t/ngx\_dynamic\_conf\_test\_module.c as reference

**ngx\_dynamic\_conf\_parse**

	ngx_int_t ngx_dynamic_conf_parse(ngx_conf_t *cf, unsigned init)

- return value:

	- return NGX_OK for successd, NGX_ERROR for failed

- paras:

	- cf   : ngx\_conf\_t passed from ngx\_dynamic\_conf\_load_conf
	- init : only ngx\_dynamic\_conf\_load\_conf set 1, otherwise set 0

This interface is supported for other dynamic conf module, such as ngx\_conf\_parse

**ngx\_dynamic\_regex\_compile**

	typedef struct {
		ngx_regex_t            *regex;
		ngx_str_t               name;
	} ngx_dynamic_regex_t;

	ngx_dynamic_regex_t *ngx_dynamic_regex_compile(ngx_conf_t *cf,
			ngx_regex_compile_t *rc);

- return value:

	- return regex context

- paras:

	- cf: ngx\_conf\_t passed in dynamic cmd handler
	- rc: regex options

compile regex

**ngx\_get\_dconf**

	void *ngx_get_dconf(ngx_module_t *m)
	
return NGX\_CORE\_MODULE dynamic config for module

## Build

cd to NGINX source directory & run this:

	./configure --add-module=/path/to/nginx-toolkit-module/
	make && make install

## Example

See t/ngx\_dynamic\_conf\_test\_module.c as reference

Build:

	./configure --with-debug --add-module=/path/to/nginx-toolkit-module/ --add-module=/path/to/nginx-toolkit-module/t
	make && make install

Configure:

	dynamic_conf    conf/nginx_dynamic.conf 10;
	dynamic_log     logs/error_dynamic.log  info;

	http {

		...

		server {

			...

			location /dynamic_conf_test/ {
				dynamic_conf_test;
			}
		}
	}

Test:

- get conf configured in dynamic config file for test module

		curl -v 'http://127.0.0.1/dynamic_conf_test/test'

change config in dynamic config, the test api will return new config value after dynamic conf refresh