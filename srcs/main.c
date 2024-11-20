#include <stdio.h>
#include <taskmaster.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

static pthread_t console_thread;

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

    char *args[] = {"/bin/ls", "-l", "-a", NULL};
    char *env[] = {"HOME=/", "LOGNAME=home", "PATH=/usr/bin", NULL};
    char *args2[] = {"/bin/echo", "'Hello, World!'", NULL};
    char *env2[] = {"HOME=/home/user", "LOGNAME=user", "PATH=/usr/bin", NULL};
    char *args3[] = {"ping", "-c 4 google.es", NULL};
    char *env3[] = {"HOME=/usr/bin", "LOGNAME=bin", "PATH=/usr/bin", NULL};
    char *argv[] = {"/bin/sh", "-c", "while :; do sleep 1; done", NULL};
    char *envp[] = {"HOME=/usr/bin", "LOGNAME=bin", "PATH=/usr/bin", NULL};

    task_t m_tasks1 = {
            .parser.name = "task1",
            .parser.cmd = "/bin/ls",
            .parser.args = args,
            .parser.dir = "/",
            .parser.env = env,
            .parser.autostart = true,
            .parser.ar = ALWAYS,
            .parser.startretries = 3,
            .parser.starttime = 1,
            .parser.stoptime = 1,
            .parser.exitcodes = exitcodes1,
            .parser.stopsignal = 15,
            .parser.stoptimeout = 1,
            .parser.stdout = "/dev/null",
            .parser.stderr = "/dev/null",
            .intern.state = STOPPED
    };
    task_t m_tasks2 = {
            .parser.name = "task2",
            .parser.cmd = "echo ",
            .parser.args = args2,
            .parser.dir = "/home/user",
            .parser.env = env2,
            .parser.autostart = false,
            .parser.ar = SUCCESS,
            .parser.startretries = 5,
            .parser.starttime = 2,
            .parser.stoptime = 2,
            .parser.exitcodes = exitcodes2,
            .parser.stopsignal = 15,
            .parser.stoptimeout = 2,
            .parser.stdout = "/dev/null",
            .parser.stderr = "/dev/null",
            .intern.state = STOPPED
    };
    task_t m_tasks3 = {
            .parser.name = "task3",
            .parser.cmd = "ping",
            .parser.args = args3,
            .parser.dir = "/usr/bin",
            .parser.env = env3,
            .parser.autostart = true,
            .parser.ar = FAILURE,
            .parser.startretries = 2,
            .parser.starttime = 3,
            .parser.stoptime = 3,
            .parser.exitcodes = exitcodes3,
            .parser.stopsignal = 15,
            .parser.stoptimeout = 3,
            .parser.stdout = "/dev/null",
            .parser.stderr = "/dev/null",
            .intern.state = STOPPED
    };
    task_t m_tasks4 = {
        .parser.name = "task4",
        .parser.cmd = "/bin/sh",
        .parser.args = argv,
        .parser.dir = "/",
        .parser.env = envp,
        .parser.autostart = true,
        .parser.ar = ALWAYS,
        .parser.startretries = 3,
        .parser.starttime = 1,
        .parser.stoptime = 1,
        .parser.exitcodes = exitcodes1,
        .parser.stopsignal = 15,
        .parser.stoptimeout = 1,
        .parser.stdout = "/dev/null",
        .parser.stderr = "/dev/null",
        .intern.state = STOPPED
    };

    task_t* tasks = NULL;
    /*parser*/
    FT_LIST_ADD_LAST(&tasks, &m_tasks1);
    FT_LIST_ADD_LAST(&tasks, &m_tasks2);
    FT_LIST_ADD_LAST(&tasks, &m_tasks3);
    FT_LIST_ADD_LAST(&tasks, &m_tasks4);

    m_active_tasks = tasks;

    pthread_create(&console_thread, NULL, interactive_console, NULL);

    supervisor(tasks);

    // sleep(10);

    // pthread_cancel(console_thread);
    pthread_join(console_thread, NULL);

    printf("Hello world\n");
    return 0;
}
