#ifndef TASKMASTER_H
#define TASKMASTER_H

#include <ft_list.h>

typedef enum
{
    NEW,
    STOPPED,
    STARTING,
    RUNNING,
    STOPPING,
    EXITED,
    FATAL,
    BACKOFF,
    SIGNALED,
    UNKNOWN
} task_state;

typedef enum
{
    true,
    false
} bool;

typedef enum
{
    ALWAYS,
    UNEXPECTED,
    SUCCESS,
    FAILURE,
    NEVER
} AR_modes;

typedef struct
{
    list_item_t l;
    char log[1024];
} log_t;

typedef struct
{
    list_item_t  l;
    char*   name;
    char*   cmd;
    char**   args;
    char*   dir;
    char**  env;
    int         autostart;
    AR_modes ar;
    int     startretries;
    int     starttime;
    int     stoptime;
    int*    exitcodes;
    int     stopsignal;
    int     stoptimeout;
    int     stdout;
    int     stderr;
    task_state  state;

    /* intern info */
    int     pid;
    int     exit_status;
    int     stop_signal;
    log_t   logs;
} task_t;

#define MAX_LOGS 10

int supervisor(task_t* tasks);

#endif /* TASKMASTER_H */