
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_BUF_H_INCLUDED_
#define _NGX_BUF_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


typedef void *            ngx_buf_tag_t;

typedef struct ngx_buf_s  ngx_buf_t;

/* ngx buf */
struct ngx_buf_s {
    u_char          *pos;		/* 当buf 所指向的数据在内存里的时候，pos 指向的是这段数据开始的位置 */
    u_char          *last;		/* 当buf 所指向的数据在内存里的时候，last 指向的是这段数据结束的位置 */
    /* 当buf 所指向的数据是在文件里的时候，file_pos 指向的是这段数据的开始位置在文件中的偏移量 */
    off_t            file_pos;	
    /* 当buf 所指向的数据是在文件里的时候，file_last 指向的是这段数据的结束位置在文件中的偏移量*/
    off_t            file_last;  
	/* 当buf 所指向的数据在内存里的时候，这一整块内存包含的内容可能被包含在多个buf 中(比
如在某段数据中间插入了其他的数据，这一块数据就需要被拆分开)。那么这些buf 中的start
和end 都指向这一块内存的开始地址和结束地址。而pos 和last 指向本buf 所实际包含的数
据的开始和结尾*/
    u_char          *start;         /* start of buffer */
    u_char          *end;           /* end of buffer */
    
    ngx_buf_tag_t    tag;			
    ngx_file_t      *file;   		/* 缓存对应文件信息  */
    /*  当这个buf 完整copy 了另外一个buf 的所有字段的时候，那么这两个buf 指向的实际上
是同一块内存，或者是同一个文件的同一部分，此时这两个buf 的shadow 字段都是指向对方
的。那么对于这样的两个buf，在释放的时候，就需要使用者特别小心，具体是由哪里释放，要
提前考虑好，如果造成资源的多次释放，可能会造成程序崩溃 */
    ngx_buf_t       *shadow;	   

	/* 标识位*/
	
    /* the buf's content could be changed */
    /* 为1 时表示该buf 所包含的内容是在一个用户创建的内存块中，并且可以被在filter
处理的过程中进行变更，而不会造成问题 */
    unsigned         temporary:1;

    /*
     * the buf's content is in a memory cache or in a read only memory
     * and must not be changed
     */
    /*  为1 时表示该buf 所包含的内容是在内存中，但是这些内容确不能被进行处理的filter 进
行变更 */
    unsigned         memory:1;

    /* the buf's content is mmap()ed and must not be changed */
    /* 为1 时表示该buf 所包含的内容是在内存中, 是通过mmap 使用内存映射从文件中映射到
内存中的，这些内容确不能被进行处理的filter 进行变更 */
    unsigned         mmap:1;

    unsigned         recycled:1;
    unsigned         in_file:1;
    unsigned         flush:1;
    unsigned         sync:1;
    /* 数据被以多个chain 传递给了过滤器，此字段为1 表明这是最后一个buf */
    unsigned         last_buf:1;
    
    unsigned         last_in_chain:1;

    unsigned         last_shadow:1;
    unsigned         temp_file:1;

    /* STUB */ int   num;  /* */
};

/* 单向缓存链表 */
struct ngx_chain_s {
    ngx_buf_t    *buf;
    ngx_chain_t  *next;
};


typedef struct {
    ngx_int_t    num;
    size_t       size;
} ngx_bufs_t;


typedef struct ngx_output_chain_ctx_s  ngx_output_chain_ctx_t;

typedef ngx_int_t (*ngx_output_chain_filter_pt)(void *ctx, ngx_chain_t *in);

#if (NGX_HAVE_FILE_AIO)
typedef void (*ngx_output_chain_aio_pt)(ngx_output_chain_ctx_t *ctx,
    ngx_file_t *file);
#endif

struct ngx_output_chain_ctx_s {
    ngx_buf_t                   *buf;
    ngx_chain_t                 *in;
    ngx_chain_t                 *free;
    ngx_chain_t                 *busy;

    unsigned                     sendfile:1;
    unsigned                     directio:1;
#if (NGX_HAVE_ALIGNED_DIRECTIO)
    unsigned                     unaligned:1;
#endif
    unsigned                     need_in_memory:1;
    unsigned                     need_in_temp:1;
#if (NGX_HAVE_FILE_AIO || NGX_THREADS)
    unsigned                     aio:1;
#endif

#if (NGX_HAVE_FILE_AIO)
    ngx_output_chain_aio_pt      aio_handler;
#if (NGX_HAVE_AIO_SENDFILE)
    ssize_t                    (*aio_preload)(ngx_buf_t *file);
#endif
#endif

#if (NGX_THREADS)
    ngx_int_t                  (*thread_handler)(ngx_thread_task_t *task,
                                                 ngx_file_t *file);
    ngx_thread_task_t           *thread_task;
#endif

    off_t                        alignment;

    ngx_pool_t                  *pool;
    ngx_int_t                    allocated;
    ngx_bufs_t                   bufs;
    ngx_buf_tag_t                tag;

    ngx_output_chain_filter_pt   output_filter;
    void                        *filter_ctx;
};


typedef struct {
    ngx_chain_t                 *out;
    ngx_chain_t                **last;
    ngx_connection_t            *connection;
    ngx_pool_t                  *pool;
    off_t                        limit;
} ngx_chain_writer_ctx_t;


#define NGX_CHAIN_ERROR     (ngx_chain_t *) NGX_ERROR


#define ngx_buf_in_memory(b)        (b->temporary || b->memory || b->mmap)
#define ngx_buf_in_memory_only(b)   (ngx_buf_in_memory(b) && !b->in_file)

#define ngx_buf_special(b)                                                   \
    ((b->flush || b->last_buf || b->sync)                                    \
     && !ngx_buf_in_memory(b) && !b->in_file)

#define ngx_buf_sync_only(b)                                                 \
    (b->sync                                                                 \
     && !ngx_buf_in_memory(b) && !b->in_file && !b->flush && !b->last_buf)

#define ngx_buf_size(b)                                                      \
    (ngx_buf_in_memory(b) ? (off_t) (b->last - b->pos):                      \
                            (b->file_last - b->file_pos))

ngx_buf_t *ngx_create_temp_buf(ngx_pool_t *pool, size_t size);
ngx_chain_t *ngx_create_chain_of_bufs(ngx_pool_t *pool, ngx_bufs_t *bufs);


#define ngx_alloc_buf(pool)  ngx_palloc(pool, sizeof(ngx_buf_t))
#define ngx_calloc_buf(pool) ngx_pcalloc(pool, sizeof(ngx_buf_t))

ngx_chain_t *ngx_alloc_chain_link(ngx_pool_t *pool);
#define ngx_free_chain(pool, cl)                                             \
    cl->next = pool->chain;                                                  \
    pool->chain = cl



ngx_int_t ngx_output_chain(ngx_output_chain_ctx_t *ctx, ngx_chain_t *in);
ngx_int_t ngx_chain_writer(void *ctx, ngx_chain_t *in);

ngx_int_t ngx_chain_add_copy(ngx_pool_t *pool, ngx_chain_t **chain,
    ngx_chain_t *in);
ngx_chain_t *ngx_chain_get_free_buf(ngx_pool_t *p, ngx_chain_t **free);
void ngx_chain_update_chains(ngx_pool_t *p, ngx_chain_t **free,
    ngx_chain_t **busy, ngx_chain_t **out, ngx_buf_tag_t tag);

off_t ngx_chain_coalesce_file(ngx_chain_t **in, off_t limit);

ngx_chain_t *ngx_chain_update_sent(ngx_chain_t *in, off_t sent);

#endif /* _NGX_BUF_H_INCLUDED_ */
