#include <stdio.h>
#include <taskmaster.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

pthread_t console_thread;

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

task_t* get_active_tasks()
{
    return m_active_tasks;
}

void handle_sigint(int sig)
{
    kill_me();
    pthread_cancel(console_thread);
    printf("Caught signal %d (SIGINT). Exiting...\n", sig);
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
            .autostart = true,
            .ar = ALWAYS,
            .startretries = 3,
            .starttime = 1,
            .stoptime = 1,
            .exitcodes = exitcodes1,
            .stopsignal = 0,
            .stoptimeout = 1,
            .stdout = 1,
            .stderr = 1,
            .state = STOPPED
    };
    task_t m_tasks2 = {
            .name = "task2",
            .cmd = "echo",
            .args = args2,
            .dir = "/home/user",
            .env = env2,
            .autostart = false,
            .ar = SUCCESS,
            .startretries = 5,
            .starttime = 2,
            .stoptime = 2,
            .exitcodes = exitcodes2,
            .stopsignal = 0,
            .stoptimeout = 2,
            .stdout = 1,
            .stderr = 1,
            .state = STOPPED
    };
    task_t m_tasks3 = {
            .name = "task3",
            .cmd = "ping",
            .args = args3,
            .dir = "/usr/bin",
            .env = env3,
            .autostart = true,
            .ar = FAILURE,
            .startretries = 2,
            .starttime = 3,
            .stoptime = 3,
            .exitcodes = exitcodes3,
            .stopsignal = 0,
            .stoptimeout = 3,
            .stdout = 1,
            .stderr = 1,
            .state = STOPPED
    };

    task_t* tasks = NULL;
    /*parser*/
    FT_LIST_ADD_LAST(&tasks, &m_tasks1);
    FT_LIST_ADD_LAST(&tasks, &m_tasks2);
    FT_LIST_ADD_LAST(&tasks, &m_tasks3);

    m_active_tasks = tasks;

    pthread_create(&console_thread, NULL, interactive_console, NULL);

    supervisor(tasks);

    // sleep(10);

    // pthread_cancel(console_thread);
    pthread_join(console_thread, NULL);

    printf("Hello world\n");
    return 0;
}