#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include <ft_malloc.h>
#include <taskmaster.h>

static bool die = false;
pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

void update_task_state(task_t* task, task_state state);
void push_log(task_t* task, const char* format, ...);
static void cleanup(task_t* tasks, bool kill_all);

int start_task(task_t* task)
{
    int pipefd[2]; /* used just for tracing unexpected errors */
    int pid = 0;
    char buffer[1024];

    if (task->intern.state == RUNNING)
    {
        push_log(task, "Task %s is already running", task->parser.name);
        return 0;
    }

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

        if (task->parser.stdout != NULL)
        {
            int fd_out = open(task->parser.stdout, O_CREAT | O_WRONLY | O_APPEND, 0644);
            if (fd_out == -1)
            {
                snprintf(buffer, sizeof(buffer), "Failed to open stdout file %s: %s", 
                         task->parser.stdout, strerror(errno));
                write(pipefd[1], buffer, strlen(buffer));
                close(pipefd[1]);
                cleanup(get_active_tasks(), false);
                exit(EXIT_FAILURE);
            }
            dup2(fd_out, STDOUT_FILENO);
            close(fd_out);
        }
        else
        {
            int fd_out = open("/dev/null", O_WRONLY);
            if (fd_out != -1)
            {
                dup2(fd_out, STDOUT_FILENO);
                close(fd_out);
            }
        }

        if (task->parser.stderr != NULL)
        {
            int fd_err = open(task->parser.stderr, O_CREAT | O_WRONLY | O_APPEND, 0644);
            if (fd_err == -1)
            {
                snprintf(buffer, sizeof(buffer), "Failed to open stderr file %s: %s", 
                         task->parser.stderr, strerror(errno));
                write(pipefd[1], buffer, strlen(buffer));
                close(pipefd[1]);
                cleanup(get_active_tasks(), false);
                exit(EXIT_FAILURE);
            }
            dup2(fd_err, STDERR_FILENO);
            close(fd_err);
        }
        else
        {
            int fd_err = open("/dev/null", O_WRONLY);
            if (fd_err != -1)
            {
                dup2(fd_err, STDERR_FILENO);
                close(fd_err);
            }
        }

        execve(task->parser.cmd, task->parser.args, task->parser.env);

        snprintf(buffer, sizeof(buffer), "Failed to start task %s due to %s.", 
                 task->parser.name, strerror(errno));
        write(pipefd[1], buffer, strlen(buffer));
        close(pipefd[1]);
        cleanup(get_active_tasks(), false);
        exit(EXIT_FAILURE);
    }
    else if (pid < 0)
    {
        // Fork failed
        update_task_state(task, FATAL);
        close(pipefd[0]);
        close(pipefd[1]);
        return -1;
    }
    else
    {
        // Parent process
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
    if (task->intern.cmd_request != CMD_NONE && cmd != CMD_NONE)
    {
        /* we are not ready to accept new command */
        pthread_mutex_unlock(&g_mutex);
        return;
    }
    printf("Updating task %s with cmd %d\n", task->parser.name, cmd);
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

void modify_task_param(void* param, void* new_value, task_param type, bool should_free)
{
    pthread_mutex_lock(&g_mutex);
    if (should_free)
    {
        switch (type)
        {
        case NEW_PARAM_STRING:
            free(*(char**)param);
            break;
        case NEW_PARAM_ARRAY:
            /* TODO */
            break;
        case NEW_PARAM_INT:
            // No need to free int
            break;
        case NEW_PARAM_BOOL:
            // No need to free bool
            break;
        case NEW_PARAM_AR:
            // No need to free AR_modes
            break;
        }
    }

    switch (type)
    {
    case NEW_PARAM_STRING:
        *(char**)param = *(char**)new_value;
        break;
    case NEW_PARAM_INT:
        *(int*)param = *(int*)new_value;
        break;
    case NEW_PARAM_BOOL:
        *(bool*)param = *(bool*)new_value;
        break;
    case NEW_PARAM_ARRAY:
        *(char**)param = *(char**)new_value; /* TODO */
        break;
    case NEW_PARAM_AR:
        *(AR_modes*)param = *(AR_modes*)new_value;
        break;
    }

    pthread_mutex_unlock(&g_mutex);
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

void free_task(task_t* task)
{
    if (task == NULL)
    {
        return;
    }
    if (task->parser.name)
    {
        free(task->parser.name);
        task->parser.name = NULL;
    }
    if (task->parser.cmd)
    {
        free(task->parser.cmd);
        task->parser.cmd = NULL;
    }
    if (task->parser.args)
    {
        for (size_t i = 0; task->parser.args[i] != NULL; i++)
        {
            free(task->parser.args[i]);
        }
        free(task->parser.args);
        task->parser.args = NULL;
    }
    if (task->parser.dir)
    {
        free(task->parser.dir);
        task->parser.dir = NULL;
    }
    if (task->parser.env)
    {
        for (size_t i = 0; task->parser.env[i] != NULL; i++)
        {
            free(task->parser.env[i]);
        }
        free(task->parser.env);
        task->parser.env = NULL;
    }
    if (task->parser.stdout)
    {
        free(task->parser.stdout);
        task->parser.stdout = NULL;
    }
    if (task->parser.stderr)
    {
        free(task->parser.stderr);
        task->parser.stderr = NULL;
    }
    if (task->parser.exitcodes)
    {
        free(task->parser.exitcodes);
        task->parser.exitcodes = NULL;
    }
    free(task);
}

void delete_task(task_t** task)
{
    task_t* tasks = get_active_tasks();
    
    if ((*task)->intern.state == RUNNING)
    {
        kill((*task)->intern.pid, SIGTERM);
    }
    delete_logs((*task));
    FT_LIST_POP(&tasks, *task);
    set_active_tasks(tasks);
    free_task(*task);
    *task = get_active_tasks();
    // *task = FT_LIST_GET_NEXT(&tasks, *task);

    return;
}

void check_if_start(task_t* task)
{
    /*
        Autostart only start when stopped, otherwise don't do anything
    */
    if (task == NULL)
    {
        return;
    }
    if (task->parser.autostart == true && task->intern.state == STOPPED)
    {
        if (start_task(task) == -1)
        {
            /* Fatal. stop all tasks and exit */
            ft_assert(0, "A task failed on launch, something bad going on.");
        }
    }
}

void kill_me()
{
    /* cannot directly close it here as it could cause a fatal datarace. */
    die = true;
}

void add_task_to_list(task_t* task)
{
    pthread_mutex_lock(&g_mutex);
    task_t* tasks = get_active_tasks();
    FT_LIST_ADD_LAST(&tasks, task);
    set_active_tasks(tasks);
    pthread_mutex_unlock(&g_mutex);
}

/* unsafe to call from outside supervisor. */
static void cleanup(task_t* tasks, bool kill_all)
{
    task_t* task = tasks;
    while (task)
    {
        if (task->intern.state == RUNNING && kill_all == true)
        {
            kill(task->intern.pid, task->parser.stopsignal);
        }
        delete_logs(task);
        delete_task(&task);
        task = get_active_tasks();
    }
}

void update_task_state(task_t* task, task_state state)
{
    task->intern.prev_state = task->intern.state;
    task->intern.state = state;
}

int cmd_requested_action_on_task(task_t** task)
{
    int ret = 0;
    // printf("head foo: %p, cmd_state: %d.\n", *task, (*task)->intern.cmd_request);
    switch ((*task)->intern.cmd_request)
    {
        case CMD_START:
            printf("Starting task %s\n", (*task)->parser.name);
            start_task(*task);
            ret = 1;
            break;
        case CMD_STOP:
            stop_task((*task)->parser.name);
            ret = 1;
            break;
        case CMD_RESTART:
            stop_task((*task)->parser.name);
            /* upgrade this logic to handle signaling log */
            start_task(*task);
            ret = 1;
            break;
        case CMD_DELETE:
            /* remove it from the list */
            delete_task(task);
            ret = 2;
            break;
        case CMD_NONE:
            return 0;
    }

    if (ret != 2 && ret != 0)
        update_task_cmd_state((*task), CMD_NONE);

    return ret;
}


int supervisor(task_t* tasks)
{
    task_t* task = NULL;
    int pid = 0;
    int status = 0;

    task = get_active_tasks();
    while (die == false)
    {
        while (task == NULL)
        {
            task = get_active_tasks();
            if (die == true)
                goto end;
            usleep(200000);
        }
        check_if_start(task);

        if (cmd_requested_action_on_task(&task) != 0)
        {
            task = get_active_tasks();
            continue;
        }

        // if (ret != 0)
        // {
        //     /* something was done, let other taks enter*/
        //     // if (ret != 2)
        //         task = FT_LIST_GET_NEXT(&tasks, task);
        //     // else
        //     //     task = tasks; /* task got deleted, for safety, start again. */
        //     continue;
        // }

        pid = task->intern.pid;
        if (pid > 0)
        {
            int result = waitpid(pid, &status, WNOHANG);
            if (result == 0)
            {
                /* child running do nothing */
                if (task->intern.state != task->intern.prev_state)
                {
                    task->intern.prev_state = task->intern.state;
                    push_log(task, "Task %s is running", task->parser.name);
                }
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

        tasks = get_active_tasks();
        task = FT_LIST_GET_NEXT(&tasks, task);
    }

end:
    printf("Exiting supervisor\n");
    tasks = get_active_tasks();
    cleanup(tasks, true);

    return 0;
}
