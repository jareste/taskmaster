#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>

#include <ft_malloc.h>
#include <taskmaster.h>

#ifndef DTACH_PATH
#define DTACH_PATH "/home/jareste-/goinfre/dtach/dtach"
#endif

static bool die = false;
pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

void update_task_state(task_t* task, task_state state);
void push_log(task_t* task, const char* format, ...);
static void cleanup(task_t* tasks, bool kill_all);

void launch_dtach(task_t* task)
{
    int arg_count = 0;
    if (task->parser.args == NULL)
    {
        arg_count = 0;
    }
    else
    {
        while (task->parser.args[arg_count] != NULL)
        {
            arg_count++;
        }
    }

    char** dtach_args = malloc((6 + arg_count) * sizeof(char*));

    dtach_args[0] = DTACH_PATH;
    dtach_args[1] = "-n";
    dtach_args[2] = task->parser.dtach;
    dtach_args[3] = "-Ez";
    dtach_args[4] = task->parser.cmd;
    for (int i = 0; i < arg_count; i++)
    {
        dtach_args[5 + i] = task->parser.args[i];
    }
    dtach_args[5 + arg_count] = NULL;

    push_log(task, "Launching dtach with command %s. Fifo on %s.", task->parser.cmd, task->parser.dtach);
    execve(DTACH_PATH, dtach_args, task->parser.env);
    free(dtach_args);
}

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

    task->intern.pid = -1;

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

        if (task->parser.dir != NULL)
        {
            if (chdir(task->parser.dir) == -1)
            {
                snprintf(buffer, sizeof(buffer), "Failed to change directory to %s: %s", 
                         task->parser.dir, strerror(errno));
                write(pipefd[1], buffer, strlen(buffer));
                close(pipefd[1]);
                cleanup(get_active_tasks(), false);
                exit(EXIT_FAILURE);
            }
        }

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

        if (task->parser.dtach != NULL)
        {
            launch_dtach(task);
        }
        else
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

/* what does it do? */
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
    task->intern.cmd_request = cmd;
    pthread_mutex_unlock(&g_mutex);
}

void push_log(task_t* task, const char* format, ...)
{
    log_t* log = NULL;
    va_list args;
    time_t now;
    struct tm* timeinfo;
    char timestamp[20];

    log = ft_malloc(sizeof(log_t));
    if (log == NULL)
    {
        return;
    }

    if (FT_LIST_GET_SIZE(&task->intern.logs) > 20)
    {
        log_t* old_log = FT_LIST_GET_FIRST(&task->intern.logs);
        FT_LIST_POP(&task->intern.logs, old_log);
        free(old_log);
    }

    time(&now);
    timeinfo = localtime(&now);

    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);

    va_start(args, format);
    snprintf(log->log, sizeof(log->log), "[%s] ", timestamp);
    vsnprintf(log->log + strlen(log->log), sizeof(log->log) - strlen(log->log), format, args);
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

void log_dtach_pipe(task_t* task)
{
    if (task->parser.dtach == NULL)
    {
        return;
    }

    if (task->intern.exit_status == 1)
    {
        push_log(task, "Dtach was already running on: %s", task->parser.dtach);
        push_log(task, "To attach to it, open a new terminal and use: dtach -a %s", task->parser.dtach);
    }
    if (task->intern.exit_status == 0)
    {
        push_log(task, "Dtach started on: %s", task->parser.dtach);
        push_log(task, "To attach to it, open a new terminal and use: dtach -a %s", task->parser.dtach);
    }
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
                    printf("Stopping task %s\n", task->parser.name);
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
    if (task->parser.dtach)
    {
        free(task->parser.dtach);
        task->parser.dtach = NULL;
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

void delete_task(task_t** task, bool kill_all)
{
    task_t* tasks = get_active_tasks();
    
    if ((*task)->intern.state == RUNNING && kill_all == true)
    {
        kill((*task)->intern.pid, SIGTERM);
    }
    delete_logs((*task));
    FT_LIST_POP(&tasks, *task);
    set_active_tasks(tasks);
    free_task(*task);
    *task = get_active_tasks();

    return;
}

/*
 * where to check the exit codes?
 */
void check_if_restart(task_t* task)
{
    switch (task->parser.ar)
    {
        case ALWAYS:
            goto start;
            break;
        case UNEXPECTED:
            if (task->intern.state == SIGNALED)
            {
                goto start;
            }
            break;
        case SUCCESS:
            if (task->intern.state == EXITED)
            {
                goto start;
            }
            break;
        case FAILURE:
            if (task->intern.state == FATAL)
            {
                goto start;
            }
            break;
        case NEVER:
            /* should never happen */
            break;
    }

    return;
start:
    if (start_task(task) == -1)
    {
        /* Fatal. stop all tasks and exit */
        ft_assert(0, "A task failed on launch, something bad going on.");
    }


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

    if (task->intern.pid != -1 && task->intern.state != STOPPED)
    {
        /*
         * Task is running, do nothing.
         */
        return;
    }

    if (task->parser.autostart == true && task->intern.state == STOPPED)
    {
        if (start_task(task) == -1)
        {
            /* Fatal. stop all tasks and exit */
            ft_assert(0, "A task failed on launch, something bad going on.");
        }
        return;
    }

    /*
     * Should we restart under certain conditions?
     */
    if (task->parser.ar != NEVER)
        check_if_restart(task);
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
            printf("Stopping task2222 %s\n", task->parser.name);
            kill(task->intern.pid, task->parser.stopsignal);
        }
        delete_logs(task);
        delete_task(&task, kill_all);
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
    switch ((*task)->intern.cmd_request)
    {
        case CMD_START:
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
            delete_task(task, true);
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
    int i = 0;

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

        /*
         * check if CLI requested any action on this task
         */
        if (cmd_requested_action_on_task(&task) != 0)
        {
            task = get_active_tasks();
            continue;
        }

        /*
         * check task status
         */
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
                /*
                 * we assume we'll be able to restore from this as shouldnt be a fatal.
                 */
                fprintf(stderr, "Failed to waitpid: %s\n", strerror(errno));

            }
            else
            {
                if (WIFEXITED(status))
                {
                    /*
                     * Exited on normal execution.
                     */
                    int exit_status = WEXITSTATUS(status);

                    task->intern.exit_status = exit_status;

                    for (i = 0; i < task->parser.exitcodes[0]; i++)
                    {
                        if (task->parser.exitcodes[i] == exit_status)
                        {
                            update_task_state(task, EXITED);
                            push_log(task, "Task %s exited with status %d", task->parser.name, exit_status);
                            log_dtach_pipe(task);
                            break;
                        }
                    }
                
                    if (i == task->parser.exitcodes[0])
                    {
                        update_task_state(task, FATAL);
                        push_log(task, "Task %s exited with unrecognized status %d. Marking it as FATAL.", task->parser.name, exit_status);
                    }                
                }
                else if (WIFSIGNALED(status))
                {
                    int signal = WTERMSIG(status);
                    
                    if (signal == task->parser.stopsignal)
                    {
                        /*
                         * Expected signal.
                         */
                        update_task_state(task, SIGNALED);
                        push_log(task, "Task %s exited with signal %d", task->parser.name, signal);
                    }
                    else
                    {
                        /*
                         * Exited on unexpected signal.
                         */
                        update_task_state(task, FATAL);
                        push_log(task, "Task %s exited with unrecognized signal %d. Marking it as FATAL.", task->parser.name, signal);
                    }
                    task->intern.stop_signal = signal;
                }
                else
                {
                    /*
                     * Exited on unknown status, mark it as FATAL.
                     */
                    update_task_state(task, FATAL);
                    push_log(task, "Task %s exited with unknown status", task->parser.name);
                }
                /*
                 * its no more valid, this way we'll not check again.
                 */
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
