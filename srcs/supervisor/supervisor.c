#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdarg.h>

#include <ft_malloc.h>
#include <taskmaster.h>

// static task_t m_active_tasks = NULL;

int start_task(task_t* task)
{
    int pid = 0;

    task->state = STARTING;
    
    pid = fork();
    if (pid == 0)
    {
        execve(task->cmd, task->args, task->env);
        exit(0);
    }
    else if (pid < 0)
    {
        task->state = FATAL;
        return -1;
    }
    else
    {
        task->pid = pid;
        task->state = RUNNING;
    }

    return (0);
}

void update_next_steps(task_t* task)
{
    (void)task;

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

int supervisor(task_t* tasks)
{
    task_t* task = NULL;
    int pid = 0;
    int status = 0;
    int loop = 0;

    task = tasks;
    while (1)
    {
        if (task == NULL)
        {
            loop++;
            task = tasks;
            if (loop == 3)    
                break;
            usleep(200000);
        }
        if (task->state == NEW)
        {
            push_log(task, "Task %s is new", task->name);
            if (start_task(task) == -1)
            {
                /* Fatal. stop all tasks and exit */
                ft_assert(0, "A task failed on launch, something bad going on.");
            }
        }

        pid = task->pid;
        if (pid > 0)
        {
            int result = waitpid(pid, &status, WNOHANG);
            if (result == 0)
            {
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
                    task->state = EXITED;
                    task->exit_status = exit_status;
                    push_log(task, "Task %s exited with status %d", task->name, exit_status);
                }
                else if (WIFSIGNALED(status))
                {
                    int signal = WTERMSIG(status);
                    task->state = SIGNALED;
                    task->stop_signal = signal;
                    push_log(task, "Task %s exited with signal %d", task->name, signal);
                }
                else
                {
                    task->state = FATAL;
                    push_log(task, "Task %s exited with unknown status", task->name);
                }
                /* its no more valid */
                task->pid = -1;
            }
        }

        update_next_steps(task);

        task = FT_LIST_GET_NEXT(&tasks, task);
    }

    task = tasks;
    while (task)
    {
        printf("\n#####################################################\n");
        print_logs(task);
        if (task->state == RUNNING)
        {
            kill(task->pid, SIGKILL);
        }
        delete_logs(task);
        task = FT_LIST_GET_NEXT(&tasks, task);
    }

    return 0;
}