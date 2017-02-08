
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_CONNECTION_H_INCLUDED_
#define _NGX_CONNECTION_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


typedef struct ngx_listening_s  ngx_listening_t;

struct ngx_listening_s {
    ngx_socket_t        fd;  /* 侦听fd */

    struct sockaddr    *sockaddr; /* 侦听地址 */
    socklen_t           socklen;    /* size of sockaddr */
    size_t              addr_text_max_len;
    ngx_str_t           addr_text;  /* 字符串形式的侦听地址 */

    int                 type;  /* 套接字类型 */
	/* TCP实现监听时的backlog队列，它表示允许正在通过三次握手建立tcp连接但还没有任何进程开始处理的连接最大个数 */
    int                 backlog;  
    int                 rcvbuf;  /* 接收缓存大小 */
    int                 sndbuf;  /* 发送缓存大小 */
#if (NGX_HAVE_KEEPALIVE_TUNABLE)
    int                 keepidle;  /* */
    int                 keepintvl; /* keepalive空闲间隔 */
    int                 keepcnt;  /* keepalive计数 */
#endif

    /* handler of accepted connection */
    ngx_connection_handler_pt   handler;  /* 当新的tcp连接成功建立后的处理方法  如ngx_http_init_connection  */

    void               *servers;  /* array of ngx_http_in_addr_t, for example */

    ngx_log_t           log;  /* 日志 */
    ngx_log_t          *logp;  /* 日志指针  */

    size_t              pool_size;
    /* should be here because of the AcceptEx() preread */
    size_t              post_accept_buffer_size;
    /* should be here because of the deferred accept */
    ngx_msec_t          post_accept_timeout;

    ngx_listening_t    *previous;
    ngx_connection_t   *connection; /* 对应侦听建立起来的连接 */

    ngx_uint_t          worker;

    unsigned            open:1;
    unsigned            remain:1;
    unsigned            ignore:1;

    unsigned            bound:1;       /* already bound */
    unsigned            inherited:1;   /* inherited from previous process */
    unsigned            nonblocking_accept:1;
    unsigned            listen:1;
    unsigned            nonblocking:1;
    unsigned            shared:1;    /* shared between threads or processes */
    unsigned            addr_ntop:1;

#if (NGX_HAVE_INET6 && defined IPV6_V6ONLY)
    unsigned            ipv6only:1;
#endif
#if (NGX_HAVE_REUSEPORT)
    unsigned            reuseport:1;	/* 置1表示支持端口reuse */
    unsigned            add_reuseport:1;
#endif
    unsigned            keepalive:2;

#if (NGX_HAVE_DEFERRED_ACCEPT)
    unsigned            deferred_accept:1;
    unsigned            delete_deferred:1;
    unsigned            add_deferred:1;
#ifdef SO_ACCEPTFILTER
    char               *accept_filter;
#endif
#endif
#if (NGX_HAVE_SETFIB)
    int                 setfib;
#endif

#if (NGX_HAVE_TCP_FASTOPEN)
    int                 fastopen;
#endif

};


typedef enum {
     NGX_ERROR_ALERT = 0,
     NGX_ERROR_ERR,
     NGX_ERROR_INFO,
     NGX_ERROR_IGNORE_ECONNRESET,
     NGX_ERROR_IGNORE_EINVAL
} ngx_connection_log_error_e;


typedef enum {
     NGX_TCP_NODELAY_UNSET = 0,
     NGX_TCP_NODELAY_SET,
     NGX_TCP_NODELAY_DISABLED
} ngx_connection_tcp_nodelay_e;


typedef enum {
     NGX_TCP_NOPUSH_UNSET = 0,
     NGX_TCP_NOPUSH_SET,
     NGX_TCP_NOPUSH_DISABLED
} ngx_connection_tcp_nopush_e;


#define NGX_LOWLEVEL_BUFFERED  0x0f
#define NGX_SSL_BUFFERED       0x01
#define NGX_SPDY_BUFFERED      0x02

/* http连接 */
struct ngx_connection_s { 
    void               *data;      /* 连接对应处理业务信息，例如ngx_stream_session_t ngx_http_connection_t  ngx_http_request_t */
    ngx_event_t        *read;     /* 读事件信息 */
    ngx_event_t        *write;    /* 写事件信息 */

    ngx_socket_t        fd;  /* 连接对应fd */

    ngx_recv_pt         recv;  /* 单缓冲区接收函数指针  如 ngx_unix_recv */
    ngx_send_pt         send;  /* 单缓冲区发送函数指针  如  ngx_unix_send */
    ngx_recv_chain_pt   recv_chain;  /* 多缓冲区接收函数指针  如 ngx_readv_chain */
    ngx_send_chain_pt   send_chain;  /* 多缓冲区发送函数指针  如 ngx_linux_sendfile_chain */

    ngx_listening_t    *listening;  /* 连接对应侦听信息 */

    off_t               sent; /* 连接上发送的字节数 */

    ngx_log_t          *log;

    ngx_pool_t         *pool; /* 该连接申请内存的内存池*/

    struct sockaddr    *sockaddr;  /* 连接客户端的sockaddr */
    socklen_t           socklen;
    ngx_str_t           addr_text; /* 客户端字符串ip地址 */

    ngx_str_t           proxy_protocol_addr;

#if (NGX_SSL)
    ngx_ssl_connection_t  *ssl;
#endif

    struct sockaddr    *local_sockaddr;
    socklen_t           local_socklen;

    ngx_buf_t          *buffer; /* 用户接受、缓存客户端发来的字符流，buffer是由连接内存池分配的，大小自由决定 */
	 /* 用来将当前连接以双向链表元素的形式添加到ngx_cycle_t核心结构体的reuseable_connection_queue双向链表中，表示可以重用的连接 */  
    ngx_queue_t         queue;
	 /*连接使用次数。ngx_connection_t结构体每次建立一条来自客户端的连接，或者主动向后端服务器发起连接时，number都会加1 */  
    ngx_atomic_uint_t   number;  

    ngx_uint_t          requests; 	/* 处理请求个数 */

    unsigned            buffered:8;  /* 缓存中的业务类型 */

    unsigned            log_error:3;     /* ngx_connection_log_error_e */

    unsigned            unexpected_eof:1; /* 为1表示不期待字符流结束 */
    unsigned            timedout:1; 	/* 为1表示连接已经超时 */
    unsigned            error:1; 		/* 表示连接出错 */
    unsigned            destroyed:1; 	/* 表示连接已销毁 */

    unsigned            idle:1; 	/* 为1表示连接处于空闲状态，如keepalive两次请求中间的状态 */
    unsigned            reusable:1; /* 为1表示连接可重用，与上面的queue字段对应使用 */
    unsigned            close:1; 	/* 表示连接关闭 */
	/* 为1表示正在将文件中的数据发往连接的另一端  todo  */
    unsigned            sendfile:1; 
    
    /*为1表示只有连接套接字对应的发送缓冲区必须满足最低设置的大小阀值时，事件驱动模块才会分发该事件。
    这与ngx_handle_write_event方法中的lowat参数是对应的 */ 
    unsigned            sndlowat:1;
    unsigned            tcp_nodelay:2;   /* ngx_connection_tcp_nodelay_e */
    unsigned            tcp_nopush:2;    /* ngx_connection_tcp_nopush_e */

    unsigned            need_last_buf:1; /* spyd使用 */

#if (NGX_HAVE_IOCP)
    unsigned            accept_context_updated:1;
#endif

#if (NGX_HAVE_AIO_SENDFILE)
    unsigned            busy_count:2;
#endif

#if (NGX_THREADS)
    ngx_thread_task_t  *sendfile_task;
#endif
};


#define ngx_set_connection_log(c, l)                                         \
                                                                             \
    c->log->file = l->file;                                                  \
    c->log->next = l->next;                                                  \
    c->log->writer = l->writer;                                              \
    c->log->wdata = l->wdata;                                                \
    if (!(c->log->log_level & NGX_LOG_DEBUG_CONNECTION)) {                   \
        c->log->log_level = l->log_level;                                    \
    }


ngx_listening_t *ngx_create_listening(ngx_conf_t *cf, void *sockaddr,
    socklen_t socklen);
ngx_int_t ngx_clone_listening(ngx_conf_t *cf, ngx_listening_t *ls);
ngx_int_t ngx_set_inherited_sockets(ngx_cycle_t *cycle);
ngx_int_t ngx_open_listening_sockets(ngx_cycle_t *cycle);
void ngx_configure_listening_sockets(ngx_cycle_t *cycle);
void ngx_close_listening_sockets(ngx_cycle_t *cycle);
void ngx_close_connection(ngx_connection_t *c);
ngx_int_t ngx_connection_local_sockaddr(ngx_connection_t *c, ngx_str_t *s,
    ngx_uint_t port);
ngx_int_t ngx_connection_error(ngx_connection_t *c, ngx_err_t err, char *text);

ngx_connection_t *ngx_get_connection(ngx_socket_t s, ngx_log_t *log);
void ngx_free_connection(ngx_connection_t *c);

void ngx_reusable_connection(ngx_connection_t *c, ngx_uint_t reusable);

#endif /* _NGX_CONNECTION_H_INCLUDED_ */
