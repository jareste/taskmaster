#include <stdio.h>
#include <taskmaster.h>
#include <signal.h>
#include <stdlib.h>

//     typedef struct
// {
//     char*   name;
//     char*   cmd;
//     char*   dir;
//     char**  env;
//     int         autostart;
//     autorestart autorestart;
//     int     startretries;
//     int     starttime;
//     int     stoptime;
//     int     exitcodes;
//     int     stopsignal;
//     int     stoptimeout;
//     int     stdout;
//     int     stderr;
// } task_t;

static task_t* m_active_tasks = NULL;

void handle_sigint(int sig)
{
    task_t* task = NULL;

    task = FT_LIST_GET_FIRST(&m_active_tasks);

    while (task)
    {
        if (task->state == RUNNING)
        {
            kill(task->pid, SIGINT);
        }
        task = FT_LIST_GET_NEXT(&m_active_tasks, task);
    }
    

    printf("Caught signal %d (SIGINT). Exiting...\n", sig);
    exit(0);
}


int main()
{
    signal(SIGINT, handle_sigint);
    
    int exitcodes1[] = {0,2,3};
    int exitcodes2[] = {1,2};
    int exitcodes3[] = {127,3};

    char *args[] = {"ls", "-l", "-a", NULL};
    char *env[] = {"HOME=/", "LOGNAME=home", "PATH=/usr/bin", NULL};
    char *args2[] = {"echo", "'Hello, World!'", NULL};
    char *env2[] = {"HOME=/home/user", "LOGNAME=user", "PATH=/usr/bin", NULL};
    char *args3[] = {"ping", "-c 4 google.es", NULL};
    char *env3[] = {"HOME=/usr/bin", "LOGNAME=bin", "PATH=/usr/bin", NULL};

    task_t m_tasks1 = {
            .name = "task1",
            .cmd = "ls",
            .args = args,
            .dir = "/",
            .env = env,
            .autostart = 1,
            .ar = ALWAYS,
            .startretries = 3,
            .starttime = 1,
            .stoptime = 1,
            .exitcodes = exitcodes1,
            .stopsignal = 0,
            .stoptimeout = 1,
            .stdout = 1,
            .stderr = 1,
            .state = NEW
    };
    task_t m_tasks2 = {
            .name = "task2",
            .cmd = "echo ",
            .args = args2,
            .dir = "/home/user",
            .env = env2,
            .autostart = 0,
            .ar = SUCCESS,
            .startretries = 5,
            .starttime = 2,
            .stoptime = 2,
            .exitcodes = exitcodes2,
            .stopsignal = 0,
            .stoptimeout = 2,
            .stdout = 1,
            .stderr = 1,
            .state = NEW
    };
    task_t m_tasks3 = {
            .name = "task3",
            .cmd = "ping",
            .args = args3,
            .dir = "/usr/bin",
            .env = env3,
            .autostart = 1,
            .ar = FAILURE,
            .startretries = 2,
            .starttime = 3,
            .stoptime = 3,
            .exitcodes = exitcodes3,
            .stopsignal = 0,
            .stoptimeout = 3,
            .stdout = 1,
            .stderr = 1,
            .state = NEW
    };

    task_t* tasks = NULL;
    /*parser*/
    FT_LIST_ADD_LAST(&tasks, &m_tasks1);
    FT_LIST_ADD_LAST(&tasks, &m_tasks2);
    FT_LIST_ADD_LAST(&tasks, &m_tasks3);


    supervisor(tasks);


    printf("Hello world\n");
    return 0;
}