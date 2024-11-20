#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include <ft_malloc.h>
#include <taskmaster.h>

static bool die = false;
pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

void update_task_state(task_t* task, task_state state);
void push_log(task_t* task, const char* format, ...);
static void cleanup(task_t* tasks);

int start_task(task_t* task)
{
    int pipefd[2];
    int pid = 0;
    char buffer[1024];

    push_log(task, "Starting task %s", task->parser.name);
    update_task_state(task, STARTING);
    
    if (pipe(pipefd) == -1)
    {
        push_log(task, "Failed to start task %s due to %s.", task->parser.name, strerror(errno));
        update_task_state(task, FATAL);
        return -1;
    }

    pid = fork();
    if (pid == 0)
    {
        close(pipefd[0]);
        /* if we gotta redirect something
         * do it here so father its clear
         */
        
        /* clear everything before child run and die */
        // char* task_cmd = strdup(task->cmd);
        // cleanup(task);

        execve(task->parser.cmd, task->parser.args, task->parser.env);
        
        snprintf(buffer, sizeof(buffer), "Failed to start task %s due to %s.", task->parser.name, strerror(errno));
        write(pipefd[1], buffer, strlen(buffer));
        close(pipefd[1]);
        cleanup(task);
        exit(EXIT_FAILURE);
    }
    else if (pid < 0)
    {
        update_task_state(task, FATAL);
        close(pipefd[0]);
        close(pipefd[1]);
        return -1;
    }
    else
    {
        close(pipefd[1]);

        fd_set readfds;
        struct timeval timeout;
        int retval;

        FD_ZERO(&readfds);
        FD_SET(pipefd[0], &readfds);

        timeout.tv_sec = 0;
        timeout.tv_usec = 500000;

        retval = select(pipefd[0] + 1, &readfds, NULL, NULL, &timeout);

        if (retval == -1)
        {
            close(pipefd[0]);
            return -1;
        }
        else if (retval == 0)
        {
            task->intern.pid = pid;
            update_task_state(task, RUNNING);
        }
        else
        {
            int nbytes = read(pipefd[0], buffer, sizeof(buffer));
            if (nbytes > 0)
            {
                buffer[nbytes] = '\0';
                push_log(task, buffer);
                update_task_state(task, FATAL);
            }
            else
            {
                task->intern.pid = pid;
                update_task_state(task, RUNNING);
            }
        }
        close(pipefd[0]);
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
    task->intern.cmd_request = cmd;
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

    if (FT_LIST_GET_SIZE(&task->intern.logs) > 10)
    {
        log_t* old_log = FT_LIST_GET_FIRST(&task->intern.logs);
        FT_LIST_POP(&task->intern.logs, old_log);
        free(old_log);
    }

    va_start(args, format);
    vsnprintf(log->log, sizeof(log->log), format, args);
    va_end(args);

    FT_LIST_ADD_LAST(&task->intern.logs, log);
}

void print_logs(task_t* task)
{
    log_t* log = NULL;

    log = FT_LIST_GET_FIRST(&task->intern.logs);
    printf("Logs from task %s:\n", task->parser.name);
    while (log)
    {
        printf("%s\n", log->log);
        log = FT_LIST_GET_NEXT(&task->intern.logs, log);
    }
}

void delete_logs(task_t* task)
{
    log_t* log = NULL;

    while (FT_LIST_GET_SIZE(&task->intern.logs) > 0)
    {
        log = FT_LIST_GET_FIRST(&task->intern.logs);
        FT_LIST_POP(&task->intern.logs, log);
        free(log);
    }
}

int stop_task(const char* task_name)
{
    /* protect it with mutex it can be used from outside!! */
    task_t* tasks = get_active_tasks();
    if (tasks)
    {
        task_t* task = tasks;
        while (task)
        {
            if (strcmp(task->parser.name, task_name) == 0)
            {
                if (task->intern.state == RUNNING)
                {
                    kill(task->intern.pid, task->parser.stopsignal);
                }
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
    if (task->parser.autostart == true && task->intern.state == STOPPED)
    {
        if (start_task(task) == -1)
        {
            /* Fatal. stop all tasks and exit */
            ft_assert(0, "A task failed on launch, something bad going on.");
        }
    }
    if (task->intern.cmd_request == CMD_START)
    {
        start_task(task);
        update_task_cmd_state(task, CMD_NONE);
    }
    else if (task->intern.cmd_request == CMD_STOP)
    {
        stop_task(task->parser.name);
        update_task_cmd_state(task, CMD_NONE);
    }
    else if (task->intern.cmd_request == CMD_RESTART)
    {
        stop_task(task->parser.name);
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
        if (task->intern.state == RUNNING)
        {
            kill(task->intern.pid, SIGKILL);
        }
        delete_logs(task);
        task = FT_LIST_GET_NEXT(&tasks, task);
    }
}

void update_task_state(task_t* task, task_state state)
{
    task->intern.prev_state = task->intern.state;
    task->intern.state = state;
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

        pid = task->intern.pid;
        if (pid > 0)
        {
            int result = waitpid(pid, &status, WNOHANG);
            if (result == 0)
            {
                if (task->intern.state != task->intern.prev_state)
                {
                    task->intern.prev_state = task->intern.state;
                    push_log(task, "Task %s is running", task->parser.name);
                }
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
                    task->intern.exit_status = exit_status;
                    push_log(task, "Task %s exited with status %d", task->parser.name, exit_status);
                }
                else if (WIFSIGNALED(status))
                {
                    int signal = WTERMSIG(status);
                    update_task_state(task, SIGNALED);
                    task->intern.stop_signal = signal;
                    push_log(task, "Task %s exited with signal %d", task->parser.name, signal);
                }
                else
                {
                    update_task_state(task, FATAL);
                    push_log(task, "Task %s exited with unknown status", task->parser.name);
                }
                /* its no more valid */
                task->intern.pid = -1;
            }
        }

        update_next_steps(task);

        task = FT_LIST_GET_NEXT(&tasks, task);
    }

    cleanup(tasks);

    return 0;
}