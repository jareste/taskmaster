#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <taskmaster.h>
#include <ft_malloc.h>

static pthread_t console_thread;
static task_t* m_active_tasks = NULL;
int pipefd[2];
bool exit_flag = false;

task_t* get_active_tasks()
{
    return m_active_tasks;
}

void set_active_tasks(task_t* tasks)
{
    m_active_tasks = tasks;
}

void handle_sigint(int sig)
{
    kill_me();
    /* this causing leaks, maybe instead communicate with the thread
     * to end it gently?
     */
    printf("\nCaught signal %d (SIGINT). Exiting...\n", sig);
    /* wake up console exit condition. */
    write(pipefd[1], "exit", 4);
    exit_flag = true;
}

int main()
{
    signal(SIGINT, handle_sigint);

    /*
    int *exitcodes1 = NEW(int, 4);
    int *exitcodes2 = NEW(int, 3);
    int *exitcodes3 = NEW(int, 3);
    int *exitcodes4 = NEW(int, 4);

    exitcodes1[0] = 4; exitcodes1[1] = 0; exitcodes1[2] = 2; exitcodes1[3] = 3;
    exitcodes2[0] = 3; exitcodes2[1] = 1; exitcodes2[2] = 2;
    exitcodes3[0] = 3; exitcodes3[1] = 127; exitcodes3[2] = 3;
    exitcodes4[0] = 4; exitcodes4[1] = 0; exitcodes4[2] = 1; exitcodes4[3] = 3;

    char **args = NEW(char*, 4);
    args[0] = strdup("/bin/ls"); args[1] = strdup("-l"); args[2] = strdup("-a"); args[3] = NULL;

    char **env = NEW(char*, 4);
    env[0] = strdup("HOME=/"); env[1] = strdup("LOGNAME=home"); env[2] = strdup("PATH=/usr/bin"); env[3] = NULL;

    char **args2 = NEW(char*, 4);
    args2[0] = strdup("/bin/echo"); args2[1] = strdup("'Hello, World!'"); args2[2] = NULL;

    char **env2 = NEW(char*, 44);
    env2[0] = strdup("HOME=/home/user"); env2[1] = strdup("LOGNAME=user"); env2[2] = strdup("PATH=/usr/bin"); env2[3] = NULL;

    char **args3 = NEW(char*, 4);
    args3[0] = strdup("ping"); args3[1] = strdup("-c 4 google.es"); args3[2] = NULL;

    char **env3 = NEW(char*, 4);
    env3[0] = strdup("HOME=/usr/bin"); env3[1] = strdup("LOGNAME=bin"); env3[2] = strdup("PATH=/usr/bin"); env3[3] = NULL;

    // char **argv = NEW(char*, 4);
    // argv[0] = strdup("/bin/sh"); argv[1] = strdup("-c"); argv[2] = strdup("while :; do sleep 1; done"); argv[3] = NULL;

    char **envp = NEW(char*, 4);
    envp[0] = strdup("HOME=/usr/bin"); envp[1] = strdup("LOGNAME=bin"); envp[2] = strdup("PATH=/usr/bin"); envp[3] = NULL;

    task_t *m_tasks1 = NEW(task_t, 1);
    m_tasks1->parser.name = strdup("task1");
    m_tasks1->parser.cmd = strdup("/bin/ls");
    m_tasks1->parser.args = args;
    m_tasks1->parser.dir = strdup("/");
    m_tasks1->parser.env = env;
    m_tasks1->parser.dtach = strdup("/tmp/dtach/task1");
    m_tasks1->parser.autostart = true;
    m_tasks1->parser.ar = ALWAYS;
    m_tasks1->parser.startretries = 3;
    m_tasks1->parser.starttime = 1;
    m_tasks1->parser.stoptime = 1;
    m_tasks1->parser.exitcodes = exitcodes1;
    m_tasks1->parser.stopsignal = 15;
    m_tasks1->parser.stoptimeout = 1;
    m_tasks1->parser.stdout = strdup("outputs/task1");
    m_tasks1->parser.stderr = strdup("outputs/task1_error");
    m_tasks1->intern.state = STOPPED;

    task_t *m_tasks2 = NEW(task_t, 1);
    m_tasks2->parser.name = strdup("task2");
    m_tasks2->parser.cmd = strdup("echo ");
    m_tasks2->parser.args = args2;
    m_tasks2->parser.dir = strdup("/home/user");
    m_tasks2->parser.env = env2;
    m_tasks2->parser.dtach = strdup("/tmp/dtach/task2");
    m_tasks2->parser.autostart = false;
    m_tasks2->parser.ar = SUCCESS;
    m_tasks2->parser.startretries = 5;
    m_tasks2->parser.starttime = 2;
    m_tasks2->parser.stoptime = 2;
    m_tasks2->parser.exitcodes = exitcodes2;
    m_tasks2->parser.stopsignal = 15;
    m_tasks2->parser.stoptimeout = 2;
    m_tasks2->parser.stdout = strdup("/dev/null");
    m_tasks2->parser.stderr = strdup("outputs/task2_error");
    m_tasks2->intern.state = STOPPED;

    task_t *m_tasks3 = NEW(task_t, 1);
    m_tasks3->parser.name = strdup("task3");
    m_tasks3->parser.cmd = strdup("ping");
    m_tasks3->parser.args = args3;
    m_tasks3->parser.dir = strdup("/usr/bin");
    m_tasks3->parser.env = env3;
    m_tasks3->parser.dtach = strdup("/tmp/dtach/task3");
    m_tasks3->parser.autostart = true;
    m_tasks3->parser.ar = FAILURE;
    m_tasks3->parser.startretries = 2;
    m_tasks3->parser.starttime = 3;
    m_tasks3->parser.stoptime = 3;
    m_tasks3->parser.exitcodes = exitcodes3;
    m_tasks3->parser.stopsignal = 15;
    m_tasks3->parser.stoptimeout = 3;
    m_tasks3->parser.stdout = strdup("/dev/null");
    m_tasks3->parser.stderr = strdup("outputs/task3_error");
    m_tasks3->intern.state = STOPPED;

    task_t *m_tasks4 = NEW(task_t, 1);
    m_tasks4->parser.name = strdup("task4");
    m_tasks4->parser.cmd = strdup("/bin/sh");
    m_tasks4->parser.args = NULL;
    m_tasks4->parser.dir = strdup("/");
    m_tasks4->parser.env = envp;
    m_tasks4->parser.dtach = strdup("/tmp/dtach/task4");
    m_tasks4->parser.autostart = true;
    m_tasks4->parser.ar = UNEXPECTED;
    m_tasks4->parser.startretries = 3;
    m_tasks4->parser.starttime = 1;
    m_tasks4->parser.stoptime = 1;
    m_tasks4->parser.exitcodes = exitcodes4;
    m_tasks4->parser.stopsignal = 14;
    m_tasks4->parser.stoptimeout = 1;
    m_tasks4->parser.stdout = strdup("/dev/null");
    m_tasks4->parser.stderr = strdup("/dev/null");
    m_tasks4->intern.state = STOPPED;

    task_t* tasks = NULL;
    /*parser*/

    FT_LIST_ADD_LAST(&tasks, m_tasks1);
    FT_LIST_ADD_LAST(&tasks, m_tasks2);
    FT_LIST_ADD_LAST(&tasks, m_tasks3);
    FT_LIST_ADD_LAST(&tasks, m_tasks4);

    m_active_tasks = tasks;

    if (pipe(pipefd) == -1) { perror("pipe"); return 1; }

    /* maybe the one launched on task must be supervisor?
     * so i can kill/relaunch it from console
     */
    pthread_create(&console_thread, NULL, interactive_console, (void*)pipefd);

    supervisor(tasks);

    pthread_join(console_thread, NULL);

    printf("Hello world\n");

    return 0;
}
