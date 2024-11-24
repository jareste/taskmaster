#ifndef TASKMASTER_H
#define TASKMASTER_H

#include <ft_list.h>
#include <pthread.h>
#include <sys/types.h>

typedef enum
{
    true,
    false
} bool;

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
    CMD_NONE,
    CMD_START,
    CMD_STOP,
    CMD_RESTART,
} cmd_request;

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
    char*       name; /* it's name */
    char*       cmd; /* cmd to launch */
    char**      args; /* args of the process */
    char*       dir; /* where should we chdir */
    char**      env; /* environment */
    char*       stdout; /* where to write stdout */
    char*       stderr; /* wher the stderr should be written */
    bool        append_out; /* append outputs or just create new ones? */
    int         dtach; /* run with dtach (this overwrites stdout and stderr) */
    bool        autostart; /* autostart or wait CLI order? */
    AR_modes    ar; /* when to autorestart */
    int         startretries; /* if it fails to start how many times should we try? */
    int         starttime; /*????*/
    int         stoptime; /*????*/
    int*        exitcodes; /* valid exit codes of the process, any other must be informed */
    int         stopsignal; /* signal to stop the process */
    int         stoptimeout; /* when to stop it? */
    int         umask; /* umask of the process */
} parser_t;

typedef struct
{
    task_state  state; /* how it is now? */

    int         pid; /* process id of the process */
    int         exit_status; /* exit code of the process */
    int         stop_signal; /* signal that stopped the process*/
    log_t       logs; /* logs that the supervisor it's tracing about process */
    task_state  prev_state; /* how it is now? */
    cmd_request cmd_request; /* what to do with this task */
} intern_t;

typedef struct
{
    list_item_t l;

    parser_t    parser; /* parser info */

    intern_t    intern;
} task_t;

#define MAX_LOGS 10

extern bool exit_flag;

void print_logs(task_t* task);
int supervisor(task_t* tasks);
task_t* get_active_tasks();
void* interactive_console(void* param);
void kill_me();
void update_task_cmd_state(task_t* task, cmd_request cmd);
void cmd_kms(void* param);

char* get_autorestart_str(AR_modes ar);

#endif /* TASKMASTER_H */
