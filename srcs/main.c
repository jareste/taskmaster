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

int main(int argc, char **argv)
{
    argc++;
    signal(SIGINT, handle_sigint);

    task_t *tasks = parse_config(argv[1]);
    if (!tasks) {
        fprintf(stderr, "Error al analizar la configuración.\n");
        return EXIT_FAILURE;
    }

    /*task_t *current = tasks;
    while (current) 
    {
        printf("\nServicio: %s\n", current->parser.name);
        printf("CMD: %s\n", current->parser.cmd);
        int i = -1;
        if (current->parser.args)
        {
            printf("ARGS:\n");
            while (current->parser.args[++i])
            {
                printf("- %s\n", current->parser.args[i]);
            }
        }
        printf("PROCS: %d\n", current->parser.procs);
        printf("UMASK: %s\n", current->parser.umask);
        printf("WDIR: %s\n", current->parser.dir);
        printf("Autostart: %d\n", current->parser.autostart);
        printf("Autorestart: %d\n", current->parser.ar);
        printf("Startretries: %d\n", current->parser.startretries);
        printf("Starttime: %d\n", current->parser.starttime);
        printf("StopSignal: %d\n", current->parser.stopsignal);
        printf("Stoptime: %d\n\n", current->parser.stoptime);
        i = -1;
        if (current->parser.exitcodes)
        {
            int len = current->parser.exitcodes[0];
            while (current->parser.exitcodes && ++i <= len)
            {
                printf("Exitcode [%d] = %d\n", i, current->parser.exitcodes[i]);
            }
        }
        i = -1;
        while (current->parser.env && current->parser.env[++i])
            printf("ENV: [%s]\n", current->parser.env[i]);
        current = FT_LIST_GET_NEXT(&tasks, current);
    }*/
    /*FT_LIST_ADD_LAST(&tasks, m_tasks1);
    FT_LIST_ADD_LAST(&tasks, m_tasks2);
    FT_LIST_ADD_LAST(&tasks, m_tasks3);
    FT_LIST_ADD_LAST(&tasks, m_tasks4);*/

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
