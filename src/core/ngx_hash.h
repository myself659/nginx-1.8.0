
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_HASH_H_INCLUDED_
#define _NGX_HASH_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


typedef struct {
    void             *value;  /* key 对应的值 */
    u_short           len;
    u_char            name[1];
} ngx_hash_elt_t;


typedef struct {
    ngx_hash_elt_t  **buckets;  /* bucket 指针数组 */
    ngx_uint_t        size;     /* 数组大小 */
} ngx_hash_t;


typedef struct {
    ngx_hash_t        hash;
    void             *value;
} ngx_hash_wildcard_t;


typedef struct {
    ngx_str_t         key;		 /* hashkey */
    ngx_uint_t        key_hash;  /* hashkey 值 */
    void             *value;
} ngx_hash_key_t;


typedef ngx_uint_t (*ngx_hash_key_pt) (u_char *data, size_t len);


typedef struct {
    ngx_hash_t            hash;
    ngx_hash_wildcard_t  *wc_head;
    ngx_hash_wildcard_t  *wc_tail;
} ngx_hash_combined_t;


typedef struct {
	
    ngx_hash_t       *hash;
    /* 指向从字符串生成hash 值的hash 函数。nginx 的源代码中提供了默认的实现函数 ngx_hash_key_lc */
    ngx_hash_key_pt   key;
	/* hash 表中的桶的个数 */
    ngx_uint_t        max_size;
    /* 每个桶的最大限制大小，单位是字节 */
    ngx_uint_t        bucket_size;
	/* hash表名称 */
    char             *name;
    /* 该hash 表分配内存使用的pool */
    ngx_pool_t       *pool;
    ngx_pool_t       *temp_pool;
} ngx_hash_init_t;


#define NGX_HASH_SMALL            1
#define NGX_HASH_LARGE            2

#define NGX_HASH_LARGE_ASIZE      16384
#define NGX_HASH_LARGE_HSIZE      10007

#define NGX_HASH_WILDCARD_KEY     1
#define NGX_HASH_READONLY_KEY     2


typedef struct {
	/* hash size */
    ngx_uint_t        hsize; 

    ngx_pool_t       *pool;
    ngx_pool_t       *temp_pool;
    
	/* 存放所有非通配符key 的数组 */
    ngx_array_t       keys;			
     /* 这是个二维数组，第一个维度代表的是bucket 的编号，那么keys_hash[i] 中存放的
是所有的key 算出来的hash 值对hsize 取模以后的值为i 的key */
    ngx_array_t      *keys_hash;   
	/* 放前向通配符key 被处理完成以后的值。比如：“*.abc.com”被处理完成以后，变
成“com.abc.”被存放在此数组中 */
    ngx_array_t       dns_wc_head;
    /* 该值在调用的过程中用来保存和检测是否有冲突的前向通配符的key 值，也
就是是否有重复*/
    ngx_array_t      *dns_wc_head_hash;
	/* 存放后向通配符key 被处理完成以后的值。比如：“mail.xxx.*”被处理完成以后，变
成“mail.xxx.”被存放在此数组中 */
    ngx_array_t       dns_wc_tail;
    /* 该值在调用的过程中用来保存和检测是否有冲突的后向通配符的key 值，也就
是是否有重复 */
    ngx_array_t      *dns_wc_tail_hash;
} ngx_hash_keys_arrays_t;


typedef struct {
    ngx_uint_t        hash;
    ngx_str_t         key;
    ngx_str_t         value;
    u_char           *lowcase_key;
} ngx_table_elt_t;


void *ngx_hash_find(ngx_hash_t *hash, ngx_uint_t key, u_char *name, size_t len);
void *ngx_hash_find_wc_head(ngx_hash_wildcard_t *hwc, u_char *name, size_t len);
void *ngx_hash_find_wc_tail(ngx_hash_wildcard_t *hwc, u_char *name, size_t len);
void *ngx_hash_find_combined(ngx_hash_combined_t *hash, ngx_uint_t key,
    u_char *name, size_t len);

ngx_int_t ngx_hash_init(ngx_hash_init_t *hinit, ngx_hash_key_t *names,
    ngx_uint_t nelts);
ngx_int_t ngx_hash_wildcard_init(ngx_hash_init_t *hinit, ngx_hash_key_t *names,
    ngx_uint_t nelts);

#define ngx_hash(key, c)   ((ngx_uint_t) key * 31 + c)
ngx_uint_t ngx_hash_key(u_char *data, size_t len);
ngx_uint_t ngx_hash_key_lc(u_char *data, size_t len);
ngx_uint_t ngx_hash_strlow(u_char *dst, u_char *src, size_t n);


ngx_int_t ngx_hash_keys_array_init(ngx_hash_keys_arrays_t *ha, ngx_uint_t type);
ngx_int_t ngx_hash_add_key(ngx_hash_keys_arrays_t *ha, ngx_str_t *key,
    void *value, ngx_uint_t flags);


#endif /* _NGX_HASH_H_INCLUDED_ */
