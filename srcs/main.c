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
// pthread_mutex_t g_mutex_task = PTHREAD_MUTEX_INITIALIZER;

task_t* get_active_tasks()
{
    // pthread_mutex_lock(&g_mutex_task);
    // task_t* tasks = m_active_tasks;
    // pthread_mutex_unlock(&g_mutex_task);
    // return tasks;
    return m_active_tasks;
}

void set_active_tasks(task_t* tasks)
{
    // pthread_mutex_lock(&g_mutex_task);
    m_active_tasks = tasks;
    // pthread_mutex_unlock(&g_mutex_task);
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
    if (!tasks) { fprintf(stderr, "Error parsing config.\n"); return EXIT_FAILURE; }

    set_active_tasks(tasks);

    if (pipe(pipefd) == -1) { perror("pipe"); return EXIT_FAILURE; }

    /* maybe the one launched on task must be supervisor?
     * so i can kill/relaunch it from console
     */
    pthread_create(&console_thread, NULL, interactive_console, (void*)pipefd);

    supervisor(tasks);

    pthread_join(console_thread, NULL);

    return 0;
}
