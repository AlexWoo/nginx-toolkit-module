# ngx-map
---
## Instructions

A map implement use ngx_rbtree

## API

**header file**

For using this API, You should include the header file as below:

	#include "ngx_map.h"

**structure**

	typedef struct {
	    ngx_rbtree_t                rbtree;
	    ngx_rbtree_node_t           sentinel;
	    ngx_map_hash_pt             hash;
	    ngx_cmp_pt                  cmp;
	} ngx_map_t;
	
	typedef struct {
	    ngx_rbtree_node_t           rn;
	    intptr_t                    raw_key;
	    ngx_map_t                  *map;
	} ngx_map_node_t;

use ngx\_map\_t instance as a map, use ngx\_map\_init to initial. Put ngx\_map\_node\_t in your structure, and set raw_key, then you can insert your node in the map. Use raw_key, you can find node in map, or delete node in map.

**ngx\_map\_init**

	void ngx_map_init(ngx_map_t *map, ngx_map_hash_pt hash, ngx_cmp_pt cmp)

Interface for init a map, hash is hash func handler to calculate hashkey for rawkey, cmp is compare func handler for compare two raw_key, when raw_keys' hashkey is same.

We support base hash and cmp func as below:

	/* ngx_str_t */
	ngx_rbtree_key_t ngx_map_hash_str(intptr_t key)
	int ngx_cmp_str(intptr_t key1, intptr_t key2)

	/* ngx_uint_t */
	ngx_rbtree_key_t ngx_map_hash_uint(intptr_t key)
	int ngx_cmp_uint(intptr_t key1, intptr_t key2)

	/* ngx_int_t */
	ngx_rbtree_key_t ngx_map_hash_int(intptr_t key)
	int ngx_cmp_int(intptr_t key1, intptr_t key2)

User also can use own hash and cmp func with protype below:

	/*
	 * key: key for map node
	 */
	typedef ngx_rbtree_key_t (* ngx_map_hash_pt)(intptr_t key);
	
	/*
	 * if key1 < key2, return -1
	 * if key1 = key2, return 0
	 * if key1 > key2, return 1
	 */
	typedef int (* ngx_cmp_pt)(intptr_t key1, intptr_t key2);

**ngx\_map\_insert**

	void ngx_map_insert(ngx_map_t *map, ngx_map_node_t *node, ngx_flag_t covered)

Interface for insert a node in map. New node will force replace old node in map if raw_key is same when covered set to 1. Otherwise, new node can't insert in map

**ngx\_map\_delete**

	void ngx_map_delete(ngx_map_t *map, intptr_t key)

Interface for delete a node in map, if node's raw_key equal to key in paras. If node is not exist, do nothing.

**ngx\_map\_find**

	ngx_map_node_t *ngx_map_find(ngx_map_t *map, intptr_t key)

Interface for find a node in map, if node's raw_key equal to key in paras. If node is not exist, return NULL.