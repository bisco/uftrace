/*
 * ftrace.c => rename uftrace.c
 * $Id: ftrace.c,v 1.10 2007/06/15 13:24:54 hamano Exp $
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

void usage()
{
    printf("uftrace version %s\n", VERSION);
    printf("usage: uftrace [-s] [-o file] [-t] [-T] [-l] [-f regex_pattern] [-b int] [-d int] command [arg ...]\n");
    printf("option\n");
    printf("  -s   : output to syslog\n");
    printf("  -o   : output to file   (FileName: file.pid)\n");
    printf("  -t   : with '-o' option (FileName: file.pid.thread_id)\n");
    printf("  -T   : thread_id added to the output format of pid.\n");
    printf("  -l   : file name and line no. added to the output\n");
    printf("  -f   : select non-matching output \n");
    printf("  -e   : output info when exit function \n");
    printf("  -b   : don't trace when current stack depth is smaller start_depth\n");
    printf("  -d   : don't trace when current stack depth is larger than max_depth\n");
    printf("  -a   : not output args\n");
    printf("\n");
}

int main(int argc,char *argv[])
{
    int i,j;
    char so[PATH_MAX];

    sprintf(so, "%s/libuftrace.so", FTRACE_LIB);
    setenv("LD_PRELOAD", so, 1);

    if(argc == 1){
        usage();
        exit(0);
    }
    for(i=1;i<argc;i++){
        if(argv[i][0] == '-'){
            switch(argv[i][1]){
            case 's':
                unsetenv("FTRACE_PATH");
                setenv("FTRACE_OUTPUT", "syslog", 1);
                break;
            case 'o':
                setenv("FTRACE_PATH",   argv[++i], 1);
                setenv("FTRACE_OUTPUT", "file",    1);
                break;
            case 't':
                setenv("FTRACE_SPLIT_THREAD_S","1",1);
                break;
            case 'T':
                setenv("FTRACE_SPLIT_THREAD_L","1",1);
                break;
            case 'l':
                setenv("FTRACE_PRINT_FILENAME","1",1);
                break;
            case 'f':
                setenv("FTRACE_FILTER_IGNORE","1",1);
                setenv("FTRACE_FILTER_IGNORE_PATTERN",   argv[++i], 1);
                break;
            case 'd':
                setenv("FTRACE_FILTER_MAX_DEPTH","1",1);
                setenv("FTRACE_MAX_DEPTH",   argv[++i], 1);
                break;
            case 'b':
                setenv("FTRACE_FILTER_START_DEPTH","1",1);
                setenv("FTRACE_START_DEPTH",   argv[++i], 1);
                break;
            case 'e':
                setenv("FTRACE_EXIT_OUTPUT","1",1);
                break;
            case 'a':
                setenv("FTRACE_NOARG","1",1);
                break;
            case 'h':
                usage();
                return(0);
            }
        }else{
            execvp(argv[i], argv + i);
            fprintf(stderr,"cant exec %s\n", argv[i]);
            exit(1);
        }
    }
    usage();
    return(1);
}
