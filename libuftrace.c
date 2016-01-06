/*
 * libftrace.c => rename to libuftrace.c
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
#include <glib/ghash.h>
#include <bfd.h>
#include <regex.h>
#include <errno.h>

#include <libuftrace.h>
#include "prototype.h"
#include <mcheck.h>


GHashTable *functions;
FILE *fp;
int opt_syslog = 0;
int opt_time = 1;
int opt_pid = 1;
int opt_threadid = 0;
int opt_type = 0;
int opt_filename = 0;
int opt_exit = 0;

static bfd *pbfd = NULL;
static asymbol **symbols = NULL;
static int nsymbol = 0;

int opt_ignore = 0;
regex_t ignore_preg;

int opt_start_depth = 0;
int opt_max_depth = 0;
int start_depth = 0;
int max_depth = 0;
int cur_depth = 0;

int opt_noarg = 0;


void ftrace_append_time(GString *line){
    struct timeval  tv;
    struct timezone tz;
    struct tm t;

    gettimeofday(&tv,&tz);
    localtime_r(&(tv.tv_sec), &t);
    g_string_append_printf(line, "%02d:%02d:%02d.%06ld ",
                      t.tm_hour, t.tm_min, t.tm_sec, tv.tv_usec);
}

void ftrace_append_pid(GString *line, int ent_or_exit){
    struct timeval  tv;
    struct timezone tz;
    struct tm t;

    gettimeofday(&tv,&tz);
    localtime_r(&(tv.tv_sec), &t);

    int depth = cur_depth;
    if(ent_or_exit == FUNC_ENTER) depth--;

    if(opt_threadid){
        g_string_append_printf(line, "[%05d.0x%lx] ", getpid(),  (unsigned long)pthread_self());
    }else{
        g_string_append_printf(line, "[%05d:%2d] ", getpid(), depth);
    }
}

int ignore_filter_match(const char* pattern) {
    return !(REG_NOMATCH == regexec(&ignore_preg, pattern, 0, NULL, 0));
}

int ftrace_append_filename(GString *line, void* address){
    const char *file_name;
    const char *function_name;
    int lineno;
    
    asection *section = bfd_get_section_by_name(pbfd, ".debug_info");
    int found = bfd_find_nearest_line(pbfd, section, symbols, (long)address, 
                                  &file_name, &function_name, &lineno);
    if(found && file_name != NULL && function_name != NULL) {
        if(opt_ignore && ignore_filter_match(function_name)) return 0;
        g_string_append_printf(line, "(%s:%5d) ", basename(file_name), lineno);
    }
    return 1;
}

#if defined(__i386) || defined(__i386__) || \
defined(__powerpc) || defined(__powerpc__)

void *ftrace_append_arg(GString *line, arg_t *arg, void *frame){

    switch(arg->type){
    case TYPE_POINTER:
        if(opt_type) g_string_append_printf(line, "(%s)", arg->typename);
        g_string_append_printf(line, "%p", frame);
        frame+=4;
        break;
    case TYPE_INT:
        if(opt_type) g_string_append_printf(line, "(%s)", arg->typename);
        g_string_append_printf(line, "%d", *(int*)frame);
        frame+=4;
        break;
    case TYPE_UINT:
        if(opt_type) g_string_append_printf(line, "(%p)", arg->typename);
        g_string_append_printf(line, "%u", *(unsigned int*)frame);
        frame+=4;
        break;
    case TYPE_CHAR:
        if(opt_type) g_string_append_printf(line, "(%s)", arg->typename);
        g_string_append_printf(line, "%d", *(char*)frame);
        frame+=4;
        break;
    case TYPE_UCHAR:
        if(opt_type) g_string_append_printf(line, "(%s)", arg->typename);
        g_string_append_printf(line, "%u", *(unsigned char*)frame);
        frame+=4;
        break;
    case TYPE_SHORT:
        if(opt_type) g_string_append_printf(line, "(%s)", arg->typename);
        g_string_append_printf(line, "%d", *(short*)frame);
        frame+=4;
        break;
    case TYPE_USHORT:
        if(opt_type) g_string_append_printf(line, "(%s)", arg->typename);
        g_string_append_printf(line, "%u", *(unsigned short*)frame);
        frame+=4;
        break;
    case TYPE_LONG:
        if(opt_type) g_string_append_printf(line, "(%s)", arg->typename);
        g_string_append_printf(line, "%ld", *(long*)frame);
        frame+=4;
        break;
    case TYPE_ULONG:
        if(opt_type) g_string_append_printf(line, "(%s)", arg->typename);
        g_string_append_printf(line, "%ld", *(unsigned long*)frame);
        frame+=4;
        break;
    case TYPE_LONGLONG:
        if(opt_type) g_string_append_printf(line, "(%s)", arg->typename);
        g_string_append_printf(line, "%lld",  *(long long*)frame);
        frame+=8;
        break;
    case TYPE_ULONGLONG:
        if(opt_type) g_string_append_printf(line, "(%s)", arg->typename);
        g_string_append_printf(line, "%lld",  *(unsigned long long*)frame);
        frame+=8;
        break;
    default:
        g_string_append_printf(line, "unknown, ...");
        return NULL;
    }
    return frame;
}

#elif defined(__x86_64) || defined(__x86_64__)

void *ftrace_append_arg(GString *line, arg_t *arg, void *frame){

    switch(arg->type){
    case TYPE_POINTER:
        if(opt_type) g_string_append_printf(line, "(%s)", arg->typename);
        g_string_append_printf(line, "%p", frame);
        frame-=4;
        break;
    case TYPE_INT:
        if(opt_type) g_string_append_printf(line, "(%s)", arg->typename);
        g_string_append_printf(line, "%d", *(int*)frame);
        frame-=4;
        break;
    case TYPE_UINT:
        if(opt_type) g_string_append_printf(line, "(%p)", arg->typename);
        g_string_append_printf(line, "%u", *(unsigned int*)frame);
        frame-=4;
        break;
    case TYPE_CHAR:
        if(opt_type) g_string_append_printf(line, "(%s)", arg->typename);
        g_string_append_printf(line, "%d", *(char*)frame);
        frame-=4;
        break;
    case TYPE_UCHAR:
        if(opt_type) g_string_append_printf(line, "(%s)", arg->typename);
        g_string_append_printf(line, "%u", *(unsigned char*)frame);
        frame-=4;
        break;
    case TYPE_SHORT:
        if(opt_type) g_string_append_printf(line, "(%s)", arg->typename);
        g_string_append_printf(line, "%d", *(short*)frame);
        frame-=4;
        break;
    case TYPE_USHORT:
        if(opt_type) g_string_append_printf(line, "(%s)", arg->typename);
        g_string_append_printf(line, "%u", *(unsigned short*)frame);
        frame-=4;
        break;
    case TYPE_LONG:
        if(opt_type) g_string_append_printf(line, "(%s)", arg->typename);
        g_string_append_printf(line, "%ld", *(long*)frame);
        frame-=8;
        break;
    case TYPE_ULONG:
        if(opt_type) g_string_append_printf(line, "(%s)", arg->typename);
        g_string_append_printf(line, "%ld", *(unsigned long*)frame);
        frame-=8;
        break;
    case TYPE_LONGLONG:
        if(opt_type) g_string_append_printf(line, "(%s)", arg->typename);
        g_string_append_printf(line, "%lld",  *(long long*)frame);
        frame-=8;
        break;
    case TYPE_ULONGLONG:
        if(opt_type) g_string_append_printf(line, "(%s)", arg->typename);
        g_string_append_printf(line, "%lld",  *(unsigned long long*)frame);
        frame-=8;
        break;
    default:
        g_string_append_printf(line, "unknown, ...");
        return NULL;
    }

    return frame;
}

#else

void *ftrace_append_arg(GString *line, arg_t *arg, void *frame){
    g_string_append_printf(line, "unknown, ...");
    return NULL;
}

#endif

int ftrace_append_function(GString *line, void *addr){
    function_t *func;
    char *name;
    void *frame;
    GSList *args;
    arg_t *arg;


#if defined(__i386) || defined(__i386__) || \
defined(__powerpc) || defined(__powerpc__)
    frame = __builtin_frame_address(3) + 8;
#elif defined(__x86_64) || defined(__x86_64__)
    frame = __builtin_frame_address(3) - 4;
#else
    frame = __builtin_frame_address(3) + 8;
#endif

    func = (function_t*)g_hash_table_lookup(functions, &addr);
    if(!func){
        g_string_append_printf(line, "UNKNOWN<0x%lx>()", (unsigned long)addr);
        return;
    }

    if(opt_ignore && ignore_filter_match(func->name)) return 0;
    if(opt_noarg) {
        g_string_append_printf(line, "%s", func->name);
        return 1;
    }

    g_string_append_printf(line, "%s(", func->name);
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
            g_string_append_printf(line, ", ");
        }
    }
    g_string_append_printf(line, ")");
    return 1;
}

void init_symbols(){
    int size;
    pbfd = bfd_openr("/proc/self/exe", NULL);
    bfd_check_format(pbfd, bfd_object);
    size = bfd_get_symtab_upper_bound(pbfd);
    symbols = (asymbol **)malloc(size);
    nsymbol = bfd_canonicalize_symtab(pbfd, symbols);
}

void init_ignore(const char* pattern){
    int err = regcomp(&ignore_preg, pattern, REG_EXTENDED|REG_ICASE|REG_NOSUB|REG_NEWLINE);
    if(err) {
        perror("regex compile");
        exit(-1);
    }
}

int init_depth(const char* depth) {
    int ret = (int)strtol(depth, NULL, 0);
    if(ret == EINVAL || ret == ERANGE) {
        perror("init_depth");
        exit(-1);
    }
    return ret;
}

void __attribute__((constructor))ftrace_init()
{
    const char *dfpath = "./uftrace_output";
    const char *target = getenv("FTRACE_OUTPUT");
    const char *ftpath = getenv("FTRACE_PATH");
    const char *stflag = getenv("FTRACE_SPLIT_THREAD_S");
    const char *dtflag = getenv("FTRACE_SPLIT_THREAD_L");
    const char *dfnameflag = getenv("FTRACE_PRINT_FILENAME");
    const char *ignoreflag = getenv("FTRACE_FILTER_IGNORE");
    const char *ignorepat  = getenv("FTRACE_FILTER_IGNORE_PATTERN");
    const char *exitflag  = getenv("FTRACE_EXIT_OUTPUT");
    const char *start_depth_flag  = getenv("FTRACE_FILTER_START_DEPTH");
    const char *start_depth_env  = getenv("FTRACE_START_DEPTH");
    const char *max_depth_flag  = getenv("FTRACE_FILTER_MAX_DEPTH");
    const char *max_depth_env  = getenv("FTRACE_MAX_DEPTH");
    const char *noargflag  = getenv("FTRACE_NOARG");
    char fname[PATH_MAX];

    if(target){
        if(!strcmp(target,"syslog")){
            opt_syslog=1;
            openlog("ftrace", LOG_CONS | LOG_PERROR, LOG_USER);
        }else{
            opt_syslog=0;
            if(!strcmp(target, "file")) {
                if(!ftpath){
                    ftpath = dfpath;
                }
                if(stflag){
                    sprintf(fname,"%s.%d.%u", ftpath, getpid(), pthread_self());
                }else{
                    sprintf(fname,"%s.%d", ftpath, getpid());
                }
                fp = fopen(fname,"a");
                if(fp == NULL){
                    fp = stderr;
                }
            } else {
                fp = stderr;
            }
        }
    }else{
        opt_syslog=0;
        fp = stderr;
    }

    if(dtflag) {
        opt_threadid = 1;
    }

    if(dfnameflag) {
        opt_filename = 1;
        init_symbols();
    }

    if(ignoreflag) {
        opt_ignore = 1;
        init_ignore(ignorepat);
    }

    if(exitflag) {
        opt_exit = 1;
    }

    if(start_depth_flag) {
        opt_start_depth = 1;
        start_depth = init_depth(start_depth_env);
    }

    if(max_depth_flag) {
        opt_max_depth = 1;
        max_depth = init_depth(max_depth_env);
    }

    if(noargflag) {
        opt_noarg = 1;
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

void cyg_profile_func_generic(void *this, void *callsite, int ent_or_exit)
{
    GString *line = g_string_new(NULL);
    int print_ok = 1;

    if(opt_time){
        ftrace_append_time(line);
    }
    if(opt_pid){
        ftrace_append_pid(line, ent_or_exit);
    }
    if(opt_filename) {
        print_ok = ftrace_append_filename(line, this);
    }
    if(opt_exit) {
        if(ent_or_exit == FUNC_ENTER) {
            g_string_append_printf(line, "%s", "enter ");
        } else {
            g_string_append_printf(line, "%s", "exit  ");
        }
    }

    print_ok = ftrace_append_function(line, this);

    if(print_ok && opt_syslog){
        syslog(LOG_INFO, "%s", line->str);
    }else if(print_ok && !(opt_syslog)){
        g_string_append_c(line, '\n');
        fprintf(fp, "%s", line->str);
        fflush(fp);
    }
}


void __cyg_profile_func_enter(void *this, void *callsite)
{
    cur_depth++;
    if(opt_start_depth && (cur_depth <= start_depth)) return;
    if(opt_max_depth && ((cur_depth-1) > max_depth)) return;
    cyg_profile_func_generic(this, callsite, FUNC_ENTER);
}

void __cyg_profile_func_exit(void *this, void *callsite)
{
    cur_depth--;
    if(opt_start_depth && (cur_depth < start_depth)) return;
    if(opt_max_depth && (cur_depth > max_depth)) return;
    if(!opt_exit) return;
    cyg_profile_func_generic(this, callsite, FUNC_EXIT);
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
        ftrace_append_pid(line, FUNC_ENTER);
    }
    fork_org = dlsym(RTLD_NEXT,"fork");
    pid = (*fork_org)();
    if(opt_time){
        ftrace_append_time(line);
    }
    if(opt_pid){
        ftrace_append_pid(line, FUNC_EXIT);
    }    
    g_string_append_printf(line, "fork() pid=%d", pid);
    g_string_append_c(line, '\n');
    fprintf(fp, "%s", line->str);
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
        ftrace_append_pid(line, FUNC_ENTER);
    }
    result=(*pthread_create_org)(thread, attr, start_routine, arg);
    if(thread){
        g_string_append_printf(line, "%-16s: result=%d TID=%lu",
                          "pthread_create()", result, (unsigned long)*thread);
    }else{
        g_string_append_printf(line, "%-16s: result=%d       ",
                          "pthread_create()", result);
    }
    g_string_append_c(line, '\n');
    fprintf(fp,"%s", line->str);
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
        ftrace_append_pid(line, FUNC_EXIT);
    }
    g_string_append_printf(line, "%-16s: retval=%p", "pthread_exit()", retval);
    g_string_append_c(line, '\n');
    fprintf(fp, "%s", line->str);
    fflush(fp);
    (*pthread_exit_org)(retval);
    exit(0);
}
