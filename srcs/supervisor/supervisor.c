#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

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
            printf("Starting task %s\n", task->name);
            if (start_task(task) == -1)
            {
                /* Fatal. stop all tasks and exit */
                ft_assert(0, "A task failed on launch, something bad going on.");
            }
        }

        pid = task->pid; // Assuming task->pid holds the PID of the task
        if (pid > 0)
        {
            int result = waitpid(pid, &status, WNOHANG);
            if (result == 0)
            {
                printf("Task %s is running\n", task->name);
                /* child running do nothing */
            }
            else if (result == -1)
            {
                /* we assume we'll be able to restore from this */
                perror("waitpid");

            }
            else
            {
                printf("Task %s exited\n", task->name);
                if (WIFEXITED(status))
                {
                    int exit_status = WEXITSTATUS(status);
                    task->state = EXITED;
                    task->exit_status = exit_status;
                    printf("Task %s exited with status %d\n", task->name, exit_status);
                }
                else if (WIFSIGNALED(status))
                {
                    int signal = WTERMSIG(status);
                    task->state = SIGNALED;
                    task->stop_signal = signal;
                    printf("Task %s exited with signal %d\n", task->name, signal);
                }
            }
        }


        task = FT_LIST_GET_NEXT(&tasks, task);
    }

    return 0;
}