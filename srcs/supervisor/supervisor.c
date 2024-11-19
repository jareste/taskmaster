#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdarg.h>
#include <string.h>

#include <ft_malloc.h>
#include <taskmaster.h>

static bool die = false;
pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

void update_task_state(task_t* task, task_state state);
void push_log(task_t* task, const char* format, ...);
static void cleanup(task_t* tasks);

int start_task(task_t* task)
{
    int pid = 0;

    push_log(task, "Starting task %s", task->name);
    update_task_state(task, STARTING);
    
    pid = fork();
    if (pid == 0)
    {
        /* if we gotta redirect something
         * do it here so father its clear
         */
        
        /* clear everything before child run and die */
        // char* task_cmd = strdup(task->cmd);
        // cleanup(task);
        execve(task->cmd, task->args, task->env);
        exit(0);
    }
    else if (pid < 0)
    {
        update_task_state(task, FATAL);
        return -1;
    }
    else
    {
        task->pid = pid;
        update_task_state(task, RUNNING);
    }

    return (0);
}

void update_next_steps(task_t* task)
{
    (void)task;

}

void update_task_cmd_state(task_t* task, cmd_request cmd)
{
    /* put mutex here */
    pthread_mutex_lock(&g_mutex);
    task->cmd_request = cmd;
    pthread_mutex_unlock(&g_mutex);
}

void push_log(task_t* task, const char* format, ...)
{
    log_t* log = NULL;
    va_list args;

    log = ft_malloc(sizeof(log_t));
    if (log == NULL)
    {
        return;
    }

    if (FT_LIST_GET_SIZE(&task->logs) > 10)
    {
        log_t* old_log = FT_LIST_GET_FIRST(&task->logs);
        FT_LIST_POP(&task->logs, old_log);
        free(old_log);
    }

    va_start(args, format);
    vsnprintf(log->log, sizeof(log->log), format, args);
    va_end(args);

    FT_LIST_ADD_LAST(&task->logs, log);
}

void print_logs(task_t* task)
{
    log_t* log = NULL;

    log = FT_LIST_GET_FIRST(&task->logs);
    printf("Logs from task %s:\n", task->name);
    while (log)
    {
        printf("%s\n", log->log);
        log = FT_LIST_GET_NEXT(&task->logs, log);
    }
}

void delete_logs(task_t* task)
{
    log_t* log = NULL;

    while (FT_LIST_GET_SIZE(&task->logs) > 0)
    {
        log = FT_LIST_GET_FIRST(&task->logs);
        FT_LIST_POP(&task->logs, log);
        free(log);
    }
}

void force_start_task(task_t* task)
{
    /* able to be called from console, put a mutex here */
    switch (task->state)
    {
        case RUNNING:
            push_log(task, "Task %s is already running", task->name);
            break;
        case STARTING:
            push_log(task, "Task %s is starting, cannot start it now", task->name);
            break;
        case STOPPING:
            push_log(task, "Task %s is stopping, cannot start it now", task->name);
            break;
        case FATAL:
        case EXITED:
        case SIGNALED:
        case UNKNOWN:
        case BACKOFF:
        case STOPPED:
            if (start_task(task) == -1)
            {
                /* Fatal. stop all tasks and exit */
                ft_assert(0, "A task failed on launch, something bad going on.");
            }
            break;
    }
}

int stop_task(const char* task_name)
{
    /* protect it with mutex it can be used from outside!! */
    return 0;
    task_t* tasks = get_active_tasks();
    if (tasks)
    {
        task_t* task = tasks;
        while (task)
        {
            if (strcmp(task->name, task_name) == 0)
            {
                if (task->state == RUNNING)
                {
                    kill(task->pid, SIGKILL);
                }
                task->state = STOPPED;
                return 0;
            }
            task = FT_LIST_GET_NEXT(&tasks, task);
        }
    }
    return -1;
}

void check_if_start(task_t* task)
{
    /*
        Autostart only start when stopped, otherwise don't do anything
    */
    if (task->autostart == true && task->state == STOPPED)
    {
        if (start_task(task) == -1)
        {
            /* Fatal. stop all tasks and exit */
            ft_assert(0, "A task failed on launch, something bad going on.");
        }
    }
    if (task->cmd_request == CMD_START)
    {
        start_task(task);
        update_task_cmd_state(task, CMD_NONE);
    }
    else if (task->cmd_request == CMD_STOP)
    {
        stop_task(task->name);
        update_task_cmd_state(task, CMD_NONE);
    }
    else if (task->cmd_request == CMD_RESTART)
    {
        stop_task(task->name);
        start_task(task);
        update_task_cmd_state(task, CMD_NONE);
    }
}

void kill_me()
{
    /* cannot directly close it here as it could cause a fatal datarace. */
    die = true;
}

/* unsafe to call from outside supervisor. */
static void cleanup(task_t* tasks)
{
    task_t* task = tasks;
    while (task)
    {
        if (task->state == RUNNING)
        {
            kill(task->pid, SIGKILL);
        }
        delete_logs(task);
        task = FT_LIST_GET_NEXT(&tasks, task);
    }
}

void update_task_state(task_t* task, task_state state)
{
    task->prev_state = task->state;
    task->state = state;
}

int supervisor(task_t* tasks)
{
    task_t* task = NULL;
    int pid = 0;
    int status = 0;
    int loop = 0;

    task = tasks;
    while (die == false)
    {
        if (task == NULL)
        {
            loop++;
            task = tasks;
            // if (loop == 3)    
            //     break;
            usleep(200000);
        }
        check_if_start(task);

        pid = task->pid;
        if (pid > 0)
        {
            int result = waitpid(pid, &status, WNOHANG);
            if (result == 0)
            {
                if (task->state != task->prev_state)
                    push_log(task, "Task %s is running", task->name);
                /* child running do nothing */
            }
            else if (result == -1)
            {
                /* we assume we'll be able to restore from this */
                perror("waitpid");

            }
            else
            {
                if (WIFEXITED(status))
                {
                    int exit_status = WEXITSTATUS(status);
                    update_task_state(task, EXITED);
                    task->exit_status = exit_status;
                    push_log(task, "Task %s exited with status %d", task->name, exit_status);
                }
                else if (WIFSIGNALED(status))
                {
                    int signal = WTERMSIG(status);
                    update_task_state(task, SIGNALED);
                    task->stop_signal = signal;
                    push_log(task, "Task %s exited with signal %d", task->name, signal);
                }
                else
                {
                    update_task_state(task, FATAL);
                    push_log(task, "Task %s exited with unknown status", task->name);
                }
                /* its no more valid */
                task->pid = -1;
            }
        }

        update_next_steps(task);

        task = FT_LIST_GET_NEXT(&tasks, task);
    }

    cleanup(tasks);

    return 0;
}