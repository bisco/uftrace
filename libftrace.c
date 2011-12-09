/*
 * libftrace.c
 * $Id: libftrace.c,v 1.6 2007/06/19 01:33:43 hamano Exp $
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
*/
#define _GNU_SOURCE
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <syslog.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <glib.h>
#include <libftrace.h>
#include "prototype.h"
#include <mcheck.h>

GHashTable *functions;
FILE *fp;
int opt_syslog = 0;
int opt_time = 1;
int opt_pid = 1;
int opt_threadid = 1;
int opt_type = 0;

void ftrace_append_time(GString *line){
    struct timeval  tv;
    struct timezone tz;
    struct tm t;

    gettimeofday(&tv,&tz);
    localtime_r(&(tv.tv_sec), &t);
    g_string_sprintfa(line, "%02d:%02d:%02d.%06d ",
                      t.tm_hour, t.tm_min, t.tm_sec, tv.tv_usec);
}

void ftrace_append_pid(GString *line){
    struct timeval  tv;
    struct timezone tz;
    struct tm t;

    gettimeofday(&tv,&tz);
    localtime_r(&(tv.tv_sec), &t);

    if(opt_threadid){
        g_string_sprintfa(line, "[%05d] ", getpid());
    }else{
        g_string_sprintfa(line, "[%05d.%u] ", getpid(),  pthread_self());
    }
}

#if defined(__i386) || defined(__i386__) || \
defined(__powerpc) || defined(__powerpc__)

void *ftrace_append_arg(GString *line, arg_t *arg, void *frame){

    switch(arg->type){
    case TYPE_POINTER:
        if(opt_type) g_string_sprintfa(line, "(%s)", arg->typename);
        g_string_sprintfa(line, "0x%p", frame);
        frame+=4;
        break;
    case TYPE_INT:
        if(opt_type) g_string_sprintfa(line, "(%s)", arg->typename);
        g_string_sprintfa(line, "%d", *(int*)frame);
        frame+=4;
        break;
    case TYPE_UINT:
        if(opt_type) g_string_sprintfa(line, "(%p)", arg->typename);
        g_string_sprintfa(line, "%u", *(unsigned int*)frame);
        frame+=4;
        break;
    case TYPE_CHAR:
        if(opt_type) g_string_sprintfa(line, "(%s)", arg->typename);
        g_string_sprintfa(line, "%d", *(char*)frame);
        frame+=4;
        break;
    case TYPE_UCHAR:
        if(opt_type) g_string_sprintfa(line, "(%s)", arg->typename);
        g_string_sprintfa(line, "%u", *(unsigned char*)frame);
        frame+=4;
        break;
    case TYPE_SHORT:
        if(opt_type) g_string_sprintfa(line, "(%s)", arg->typename);
        g_string_sprintfa(line, "%d", *(short*)frame);
        frame+=4;
        break;
    case TYPE_USHORT:
        if(opt_type) g_string_sprintfa(line, "(%s)", arg->typename);
        g_string_sprintfa(line, "%u", *(unsigned short*)frame);
        frame+=4;
        break;
    case TYPE_LONG:
        if(opt_type) g_string_sprintfa(line, "(%s)", arg->typename);
        g_string_sprintfa(line, "%ld", *(long*)frame);
        frame+=4;
        break;
    case TYPE_ULONG:
        if(opt_type) g_string_sprintfa(line, "(%s)", arg->typename);
        g_string_sprintfa(line, "%ld", *(unsigned long*)frame);
        frame+=4;
        break;
    case TYPE_LONGLONG:
        if(opt_type) g_string_sprintfa(line, "(%s)", arg->typename);
        g_string_sprintfa(line, "%lld",  *(long long*)frame);
        frame+=8;
        break;
    case TYPE_ULONGLONG:
        if(opt_type) g_string_sprintfa(line, "(%s)", arg->typename);
        g_string_sprintfa(line, "%lld",  *(unsigned long long*)frame);
        frame+=8;
        break;
    default:
        g_string_sprintfa(line, "unknown, ...");
        return NULL;
    }
    return frame;
}

#elif defined(__x86_64) || defined(__x86_64__)

void *ftrace_append_arg(GString *line, arg_t *arg, void *frame){

    switch(arg->type){
    case TYPE_POINTER:
        if(opt_type) g_string_sprintfa(line, "(%s)", arg->typename);
        g_string_sprintfa(line, "0x%p", frame);
        frame-=4;
        break;
    case TYPE_INT:
        if(opt_type) g_string_sprintfa(line, "(%s)", arg->typename);
        g_string_sprintfa(line, "%d", *(int*)frame);
        frame-=4;
        break;
    case TYPE_UINT:
        if(opt_type) g_string_sprintfa(line, "(%p)", arg->typename);
        g_string_sprintfa(line, "%u", *(unsigned int*)frame);
        frame-=4;
        break;
    case TYPE_CHAR:
        if(opt_type) g_string_sprintfa(line, "(%s)", arg->typename);
        g_string_sprintfa(line, "%d", *(char*)frame);
        frame-=4;
        break;
    case TYPE_UCHAR:
        if(opt_type) g_string_sprintfa(line, "(%s)", arg->typename);
        g_string_sprintfa(line, "%u", *(unsigned char*)frame);
        frame-=4;
        break;
    case TYPE_SHORT:
        if(opt_type) g_string_sprintfa(line, "(%s)", arg->typename);
        g_string_sprintfa(line, "%d", *(short*)frame);
        frame-=4;
        break;
    case TYPE_USHORT:
        if(opt_type) g_string_sprintfa(line, "(%s)", arg->typename);
        g_string_sprintfa(line, "%u", *(unsigned short*)frame);
        frame-=4;
        break;
    case TYPE_LONG:
        if(opt_type) g_string_sprintfa(line, "(%s)", arg->typename);
        g_string_sprintfa(line, "%ld", *(long*)frame);
        frame-=8;
        break;
    case TYPE_ULONG:
        if(opt_type) g_string_sprintfa(line, "(%s)", arg->typename);
        g_string_sprintfa(line, "%ld", *(unsigned long*)frame);
        frame-=8;
        break;
    case TYPE_LONGLONG:
        if(opt_type) g_string_sprintfa(line, "(%s)", arg->typename);
        g_string_sprintfa(line, "%lld",  *(long long*)frame);
        frame-=8;
        break;
    case TYPE_ULONGLONG:
        if(opt_type) g_string_sprintfa(line, "(%s)", arg->typename);
        g_string_sprintfa(line, "%lld",  *(unsigned long long*)frame);
        frame-=8;
        break;
    default:
        g_string_sprintfa(line, "unknown, ...");
        return NULL;
    }

    return frame;
}

#else

void *ftrace_append_arg(GString *line, arg_t *arg, void *frame){
    g_string_sprintfa(line, "unknown, ...");
    return NULL;
}

#endif

void ftrace_append_function(GString *line, void *addr){
    function_t *func;
    char *name;
    void *frame;
    GSList *args;
    arg_t *arg;


#if defined(__i386) || defined(__i386__) || \
defined(__powerpc) || defined(__powerpc__)
    frame = __builtin_frame_address(2) + 8;
#elif defined(__x86_64) || defined(__x86_64__)
    frame = __builtin_frame_address(2) - 4;
#else
    frame = __builtin_frame_address(2) + 8;
#endif

    func = (function_t*)g_hash_table_lookup(functions, &addr);
    if(!func){
        g_string_sprintfa(line, "UNKNOWN<0x%x>()", addr);
        return;
    }

    g_string_sprintfa(line, "%s(", func->name);
    args = func->args;
    while(args){
        arg = args->data;
        //fprintf(stderr, "%s %s %d\n", arg->typename, arg->name, arg->size);
#if defined(__x86_64) || defined(__x86_64__)
        if(args == func->args && \
           ( \
             arg->type == TYPE_LONG || \
             arg->type == TYPE_ULONG || \
             arg->type == TYPE_LONGLONG || \
             arg->type == TYPE_ULONGLONG )\
            ){
            frame -= 4;
        }
#endif
        frame = ftrace_append_arg(line, arg, frame);
        if(!frame){
            break;
        }
        args = g_slist_next(args);
        if(args){
            g_string_sprintfa(line, ", ");
        }
    }
    g_string_sprintfa(line, ")");
}

void __attribute__((constructor))ftrace_init()
{
    const char *target = getenv("FTRACE_OUTPUT");
    const char *ftpath = getenv("FTRACE_PATH");
    const char *stflag = getenv("FTRACE_SPLIT_THREAD_S");
    const char *dtflag = getenv("FTRACE_SPLIT_THREAD_L");

    if(target){
        if(!strcmp(target,"syslog")){
            opt_syslog=1;
            openlog("ftrace", LOG_CONS | LOG_PERROR, LOG_USER);
        }else{
            opt_syslog=0;
            fp = stderr;
        }
    }else{
        opt_syslog=0;
        fp = stderr;
    }
    functions = g_hash_table_new((GHashFunc)g_int_hash,
                                 (GCompareFunc)g_int_equal);
    prototype_init(functions, "/proc/self/exe");
    //prototype_print(functions);
}

void __attribute__((destructor))ftrace_finish()
{
    
    prototype_finish(functions);
    g_hash_table_destroy(functions);
    fclose(fp);
}

void __cyg_profile_func_enter(void *this, void *callsite)
{
    GString *line = g_string_new(NULL);

    if(opt_time){
        ftrace_append_time(line);
    }
    if(opt_pid){
        ftrace_append_pid(line);
    }

    ftrace_append_function(line, this);

    if(opt_syslog){
        syslog(LOG_INFO, line->str);
    }else{
        g_string_append_c(line, '\n');
        fprintf(fp, line->str);
        fflush(fp);
    }
}

void __cyg_profile_func_exit(void *this, void *callsite)
{
}

pid_t fork()
{
    pid_t pid;
    pid_t (*fork_org)(void);
    GString *line = g_string_new(NULL);

    if(opt_time){
        ftrace_append_time(line);
    }
    if(opt_pid){
        ftrace_append_pid(line);
    }
    fork_org = dlsym(RTLD_NEXT,"fork");
    pid = (*fork_org)();
    if(opt_time){
        ftrace_append_time(line);
    }
    if(opt_pid){
        ftrace_append_pid(line);
    }    
    g_string_sprintfa(line, "fork() pid=%d", pid);
    g_string_append_c(line, '\n');
    fprintf(fp, line->str);
    fflush(fp);
    return(pid);
}

int pthread_create(pthread_t *thread, pthread_attr_t * attr,
                   void * (*start_routine)(void *), void * arg)
{
    int result;
    GString *line = g_string_new(NULL);
    int (*pthread_create_org)(pthread_t * thread, pthread_attr_t * attr,
                              void * (*start_routine)(void *), void * arg);
    pthread_create_org = dlsym(RTLD_NEXT,"pthread_create");

    if(opt_time){
        ftrace_append_time(line);
    }
    if(opt_pid){
        ftrace_append_pid(line);
    }
    result=(*pthread_create_org)(thread, attr, start_routine, arg);
    if(thread){
        g_string_sprintfa(line, "%-16s: result=%d TID=%u",
                          "pthread_create()", result, *thread);
    }else{
        g_string_sprintfa(line, "%-16s: result=%d       ",
                          "pthread_create()", result, *thread);
    }
    g_string_append_c(line, '\n');
    fprintf(fp, line->str);
    fflush(fp);
    return(result);
}

void pthread_exit(void *retval)
{
    GString *line = g_string_new(NULL);
    void (*pthread_exit_org)(void *retval);
    pthread_exit_org = dlsym(RTLD_NEXT,"pthread_exit");

    if(opt_time){
        ftrace_append_time(line);
    }
    if(opt_pid){
        ftrace_append_pid(line);
    }
    g_string_sprintfa(line, "%-16s: retval=%p", "pthread_exit()", retval);
    g_string_append_c(line, '\n');
    fprintf(fp, line->str);
    fflush(fp);
    (*pthread_exit_org)(retval);
    exit(0);
}
