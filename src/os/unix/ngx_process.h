
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_PROCESS_H_INCLUDED_
#define _NGX_PROCESS_H_INCLUDED_


#include <ngx_setaffinity.h>
#include <ngx_setproctitle.h>


typedef pid_t       ngx_pid_t;

#define NGX_INVALID_PID  -1

typedef void (*ngx_spawn_proc_pt) (ngx_cycle_t *cycle, void *data);

/* master进程对worker进程的信息维护 */
typedef struct {
    ngx_pid_t           pid;
    int                 status;
    ngx_socket_t        channel[2];  /* worker与master进程之间通信socketpairfd  */
	
    ngx_spawn_proc_pt   proc;  /* */
    void               *data;
    char               *name;

    unsigned            respawn:1;
    unsigned            just_spawn:1;
    unsigned            detached:1;
    unsigned            exiting:1;
    unsigned            exited:1;
} ngx_process_t;


typedef struct {
    char         *path;
    char         *name;
    char *const  *argv;
    char *const  *envp;
} ngx_exec_ctx_t;


#define NGX_MAX_PROCESSES         1024

/* 这五种类型的区别  */
 
/* 
NGX_PROCESS_XXX 用来标示创建子进程的属性. 
 
NGX_PROCESS_NORESPAWN: 
    子进程退出时,父进程不会再次创建, 该标记用在创建 "cache loader process". 

NGX_PROCESS_JUST_SPAWN:  
    当 nginx -s reload 时, 如果还有未加载的 proxy_cache_path, 
    则需要再次创建 "cache loader process"加载,并用 NGX_PROCESS_JUST_SPAWN给这个进程做记号,
    防止 "master会向老的worker进程,老的cache manager进程,老的cache loader进程(如果存在)发送NGX_CMD_QUIT或SIGQUIT" 时,
    误以为这个进程是老的cache loader进程. 

NGX_PROCESS_RESPAWN:  
    子进程异常退出时,master会重新创建它, 如当worker或cache manager异常退出时,父进程会重新创建它. 

NGX_PROCESS_JUST_RESPAWN:  
    当 nginx -s reload 时, master会向老的worker进程,老的cache manager进程,
    老的cache loader进程(如果存在)发送 ngx_write_channel(NGX_CMD_QUIT)(如果失败则发送SIGQUIT信号); 
    该标记用来标记进程数组中哪些是新创建的子进程;其他的就是老的子进程. 

NGX_PROCESS_DETACHED: 热代码替换 

更多参考:http://blog.csdn.net/gsnumen/article/details/7979484?reload

 */  


#define NGX_PROCESS_NORESPAWN     -1
#define NGX_PROCESS_JUST_SPAWN    -2
#define NGX_PROCESS_RESPAWN       -3
#define NGX_PROCESS_JUST_RESPAWN  -4
#define NGX_PROCESS_DETACHED      -5


#define ngx_getpid   getpid

#ifndef ngx_log_pid
#define ngx_log_pid  ngx_pid
#endif


ngx_pid_t ngx_spawn_process(ngx_cycle_t *cycle,
    ngx_spawn_proc_pt proc, void *data, char *name, ngx_int_t respawn);
ngx_pid_t ngx_execute(ngx_cycle_t *cycle, ngx_exec_ctx_t *ctx);
ngx_int_t ngx_init_signals(ngx_log_t *log);
void ngx_debug_point(void);


#if (NGX_HAVE_SCHED_YIELD)
#define ngx_sched_yield()  sched_yield()
#else
#define ngx_sched_yield()  usleep(1)
#endif


extern int            ngx_argc;
extern char         **ngx_argv;
extern char         **ngx_os_argv;

extern ngx_pid_t      ngx_pid;
extern ngx_socket_t   ngx_channel;
extern ngx_int_t      ngx_process_slot;
extern ngx_int_t      ngx_last_process;
extern ngx_process_t  ngx_processes[NGX_MAX_PROCESSES];


#endif /* _NGX_PROCESS_H_INCLUDED_ */
