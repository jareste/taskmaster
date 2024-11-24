#include <taskmaster.h>
#include <strings.h>

// const char *get_signal_name(int signal)
// {
//     if (signal >= MAX_SIGNAME && signal <= SIGRTMAX)
//     {
//         static char rt_signal_name[20];
//         if (signal == MAX_SIGNAME)
//             return "SIGRTMIN";
//         else
//         {
//             sprintf(rt_signal_name, "SIGRT_%d", signal - MAX_SIGNAME);
//             return rt_signal_name;
//         }
//     }

//     switch (signal)
//     {
//         case 1: return "SIGHUP";
//         case 2: return "SIGINT";
//         case 3: return "SIGQUIT";
//         case 4: return "SIGILL";
//         case 5: return "SIGTRAP";
//         case 6: return "SIGABRT";
//         case 7: return "SIGBUS";
//         case 8: return "SIGFPE";
//         case 9: return "SIGKILL";
//         case 10: return "SIGUSR1";
//         case 11: return "SIGSEGV";
//         case 12: return "SIGUSR2";
//         case 13: return "SIGPIPE";
//         case 14: return "SIGALRM";
//         case 15: return "SIGTERM";
//         case 16: return "SIGSTKFLT";
//         case 17: return "SIGCHLD";
//         case 18: return "SIGCONT";
//         case 19: return "SIGSTOP";
//         case 20: return "SIGTSTP";
//         case 21: return "SIGTTIN";
//         case 22: return "SIGTTOU";
//         case 23: return "SIGURG";
//         case 24: return "SIGXCPU";
//         case 25: return "SIGXFSZ";
//         case 26: return "SIGVTALRM";
//         case 27: return "SIGPROF";
//         case 28: return "SIGWINCH";
//         case 29: return "SIGIO";
//         case 30: return "SIGPWR";
//         case 31: return "SIGSYS";
//         case 32: return "SIGRTMIN";
//         default: return "NULL";
//     }
// }

// enum
// {
//     NAME,
//     CMD,
//     DIR,
//     ENV,
//     AUTOSTART,
//     AUTORESTART,
//     STARTRETRIES,
//     STARTTIME,
//     STOPTIME,
//     EXITCODES,
//     STOPSIGNAL,
//     STOPTIMEOUT,
//     STDOUT,
//     STDERR,
//     STDIN,
//     UMASK,
// } TYPE;

// int parse_int(char* str);
// char* parse_string(char* str);
// char** parse_env(char* str);
// autorestart parse_autorestart(char* str);

// typedef struct
// {
//     char*   field_name;
//     TYPE    field_enum;
//     void*   (*parse_me)(char*);
// } t_parser;

// static t_parser m_parser[] = {
//     {"name", NAME, parse_string},
//     {"cmd", CMD, parse_string},
//     {"dir", DIR, parse_string},
//     {"env", ENV, parse_env},
//     {"autostart", AUTOSTART, parse_int},
//     {"autorestart", AUTORESTART, parse_autorestart},
//     {"startretries", STARTRETRIES, parse_int},
//     {"starttime", STARTTIME, parse_int},
//     {"stoptime", STOPTIME, parse_int},
//     {"exitcodes", EXITCODES, parse_int},
//     {"stopsignal", STOPSIGNAL, parse_int},
//     {"stoptimeout", STOPTIMEOUT, parse_int},
//     {"stdout", STDOUT, parse_int},
//     {"stderr", STDERR, parse_int},
//     {"stdin", STDIN, parse_int},
//     {"umask", UMASK, parse_int},
// };

// int parse_int(char* str)
// {
//     return (atoi(str));
// }

// char* parse_string(char* str)
// {
//     return (str);
// }

char** parse_array(char* str)
{
    (void)str;
    return NULL;
}



// t_task* new_task()
// {
//     t_task* task = (t_task*)malloc(sizeof(t_task));
//     if (task == NULL)
//         return (NULL);
//     task->name = NULL;
//     task->cmd = NULL;
//     task->args = NULL;
//     task->dir = NULL;
//     task->env = NULL;
//     task->stdout = NULL;
//     task->stderr = NULL;
//     task->autostart = true;
//     task->autorestart = ALWAYS;
//     task->startretries = 0;
//     task->starttime = 0;
//     task->stoptime = 0;
//     task->exitcodes = NULL;
//     task->stopsignal = 0;
//     task->stoptimeout = 0;
//     task->umask = 0;
//     return (task);
// }

char* get_autorestart_str(AR_modes ar)
{
    switch (ar)
    {
        case ALWAYS: return "always";
        case UNEXPECTED: return "unexpected";
        case SUCCESS: return "success";
        case FAILURE: return "failure";
        case NEVER: return "never";
        default: return "unknown";
    }
}

AR_modes parse_autorestart(char* str)
{
    if (strcasecmp(str, "always") == 0)
        return (ALWAYS);
    else if (strcasecmp(str, "unexpected") == 0)
        return (UNEXPECTED);
    else if (strcasecmp(str, "success") == 0)
        return (SUCCESS);
    else if (strcasecmp(str, "failure") == 0)
        return (FAILURE);
    else if (strcasecmp(str, "never") == 0)
        return (NEVER);
    /* failure!!! */
    return (NEVER);
}
