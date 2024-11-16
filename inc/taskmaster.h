#ifndef TASKMASTER_H
#define TASKMASTER_H

enum
{
    STOPPED,
    STARTING,
    RUNNING,
    STOPPING,
    EXITED,
    FATAL,
    BACKOFF,
    UNKNOWN
} task_state;

enum
{
    true,
    false
} bool;

enum
{
    ALWAYS,
    UNEXPECTED,
    SUCCESS,
    FAILURE,
    NEVER
} autorestart;

typedef struct
{
    char*   name;
    char*   cmd;
    char*   dir;
    char**  env;
    int         autostart;
    autorestart autorestart;
    int     startretries;
    int     starttime;
    int     stoptime;
    int     exitcodes;
    int     stopsignal;
    int     stoptimeout;
    int     stdout;
    int     stderr;
} task_t;

#endif /* TASKMASTER_H */