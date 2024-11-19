#ifndef TASKMASTER_H
#define TASKMASTER_H

#include <ft_list.h>

typedef enum
{
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
    list_item_t l;
    char*       name; /* it's name */
    char*       cmd; /* cmd to launch */
    char**      args; /* args of the process */
    char*       dir; /* where should we chdir */
    char**      env; /* environment */
    bool        autostart; /* autostart or wait CLI order? */
    AR_modes    ar; /* when to autorestart */
    int         startretries; /* if it fails to start how many times should we try? */
    int         starttime; /*????*/
    int         stoptime; /*????*/
    int*        exitcodes; /* valid exit codes of the process, any other must be informed */
    int         stopsignal; /* signal to stop the process */
    int         stoptimeout; /* when to stop it? */
    int         stdout; /* where to write stdout */
    int         stderr; /* wher the stderr should be written */
    task_state  state; /* how it is now? */

    /* intern info */
    int         pid; /* process id of the process */
    int         exit_status; /* exit code of the process */
    int         stop_signal; /* signal that stopped the process*/
    log_t       logs; /* logs that the supervisor it's tracing about process */
} task_t;

#define MAX_LOGS 10

extern pthread_mutex_t g_mutex_list;

int supervisor(task_t* tasks);
task_t* get_active_tasks();
int interactive_console(void* param);

#endif /* TASKMASTER_H */