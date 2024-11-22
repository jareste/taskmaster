#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <taskmaster.h>
#include <signal.h>
#include <unistd.h>

void cmd_help(void* param);
void cmd_active(void* param);
void cmd_stop(void* param);
void cmd_update(void* param);
void cmd_kms(void* param);
void cmd_print_logs(void* param);
void cmd_kill_supervisor(void* param);
void cmd_start(void* param);
void cmd_new(void* param); /* TODO */
void cmd_restart(void* param); /* TODO */
void cmd_show_task(void* task);
void cmd_modify_task(void* param); /* TODO */

typedef struct {
    const char* name;
    void (*func)(void* param);
    const char* description;
} command_t;

command_t commands[] = {
    {"help", cmd_help, "Show this help menu"},
    {"new", cmd_new, "Create a new task"}, /* TODO */
    {"status", cmd_active, "Show tasks status"},
    {"start", cmd_start, "Start a specific task by name"},
    {"stop", cmd_stop, "Stop a specific task by name"},
    {"restart", cmd_restart, "Restart a specific task by name"},
    {"update", cmd_update, "Re-read the configuration file"}, /* TODO */
    {"logs", cmd_print_logs, "Print logs for a specific task or all tasks"},
    {"kill", cmd_kill_supervisor, "Kill the supervisor"},
    {"kms", cmd_kms, "Finish the program execution."},
    {"show", cmd_show_task, "Show task details."},
    {"modify", cmd_modify_task, "Modify task details."}, /* TODO */
    {NULL, NULL, NULL}
};

task_t* get_task_from_name(const char* name)
{
    task_t* tasks = get_active_tasks();
    if (tasks)
    {
        task_t* task = tasks;
        while (task)
        {
            if (strcmp(task->parser.name, name) == 0)
            {
                return task;
            }
            task = FT_LIST_GET_NEXT(&tasks, task);
        }
    }
    else
    {
        fprintf(stdout, "No active tasks.\n");
    }
    return NULL;
}

command_t* find_command(const char* name)
{
    for (command_t* cmd = commands; cmd->name != NULL; ++cmd)
    {
        if (strcmp(cmd->name, name) == 0)
        {
            return cmd;
        }
    }

    if (strcmp(name, "ls") == 0 || strcmp(name, "?") == 0)
    {
        return commands;
    }

    return NULL;
}

void* interactive_console(void* param)
{
    int* pipefd = (int*)param;
    char input[256];

    fprintf(stdout, "Interactive Console. Type 'help' for a list of commands.\n");
    fflush(stdout);

    while (1)
    {
        fprintf(stdout, "-> ");
        fflush(stdout);
        
        /* Propper threat cancelation ZzzzZZZzzz */
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(pipefd[0], &readfds);

        int maxfd = (STDIN_FILENO > pipefd[0]) ? STDIN_FILENO : pipefd[0];

        int retval = select(maxfd + 1, &readfds, NULL, NULL, NULL);
        if (retval == -1)
        {
            perror("select");
            break;
        }

        if (FD_ISSET(pipefd[0], &readfds))
        {
            char buf[4];
            read(pipefd[0], buf, 4);
            break;
        }
        /* Propper threat cancelation ZzzzZZZzzz */

        if (FD_ISSET(STDIN_FILENO, &readfds))
        {
            if (fgets(input, sizeof(input), stdin) == NULL)
            {
                break;
            }

            input[strcspn(input, "\n")] = 0;

            if (strcmp(input, "exit") == 0)
            {
                break;
            }

            char* cmd_name = strtok(input, " ");
            char* arg = strtok(NULL, "");

            if (cmd_name == NULL)
            {
                continue;
            }

            command_t* cmd = find_command(cmd_name);
            if (cmd != NULL)
            {
                if (arg)
                {
                    cmd->func(arg);
                }
                else
                {
                    cmd->func(NULL);
                }
            }
            else
            {
                fprintf(stdout, "Unknown command: '%s'. Type 'help' for a list of commands.\n", cmd_name);
            }
        }
    }

    /* something went wrong, lets just end safely. */
    kill_me();

    return NULL;
}
char* get_state_string(task_state ts)
{
    switch (ts)
    {
    case STOPPED:
        return "STOPPED";
    case RUNNING:
        return "RUNNING";
    case FATAL:
        return "FATAL";
    case STARTING:
        return "STARTING";
    case STOPPING:
        return "STOPPING";
    case EXITED:
        return "EXITED";
    case BACKOFF:
        return "BACKOFF";
    case SIGNALED:
        return "SIGNALED";
    case UNKNOWN:
        return "UNKNOWN";
    }
    return "UNKNOWN";
}

void cmd_help(void* param)
{
    (void)param;
    fprintf(stdout, "Available commands:\n");
    for (command_t* cmd = commands; cmd->name != NULL; ++cmd)
    {
        fprintf(stdout, "  %s: %s\n", cmd->name, cmd->description);
    }
}

void cmd_active(void* param)
{
    (void)param;
    task_t* tasks = get_active_tasks();
    if (tasks)
    {
        fprintf(stdout, "Active tasks:\n");
        task_t* task = tasks;
        while (task)
        {
            fprintf(stdout, "\t- %s :\t%s\n", task->parser.name, get_state_string(task->intern.state));
            task = FT_LIST_GET_NEXT(&tasks, task);
        }
    }
    else
    {
        fprintf(stdout, "No active tasks.\n");
    }
}

void cmd_restart(void* param)
{
    if (param == NULL)
    {
        fprintf(stdout, "Usage: restart <task_name>\n");
        return;
    }
    const char* task_name = (const char*)param;
    task_t* task = get_task_from_name(task_name);
    if (task)
    {
        fprintf(stdout, "Restart of task %s requested.\n", task_name);
        update_task_cmd_state(task, CMD_RESTART);
    }
    else
    {
        fprintf(stdout, "Requested task not found\n");
    }
}

void cmd_start(void* param)
{
    if (param == NULL)
    {
        fprintf(stdout, "Usage: start <task_name>\n");
        return;
    }
    const char* task_name = (const char*)param;
    task_t* task = get_task_from_name(task_name);
    if (task)
    {
        fprintf(stdout, "Start of task %s requested.\n", task_name);
        update_task_cmd_state(task, CMD_START);
    }
    else
    {
        fprintf(stdout, "Requested task not found\n");
    }
}

void cmd_stop(void* param)
{
    if (param == NULL)
    {
        fprintf(stdout, "Usage: stop <task_name>\n");
        return;
    }
    const char* task_name = (const char*)param;
    task_t* task = get_task_from_name(task_name);
    if (task)
    {
        fprintf(stdout, "Stop of task %s requested.\n", task_name);
        update_task_cmd_state(task, CMD_STOP);
    }
    else
    {
        fprintf(stdout, "Requested task not found\n");
    }
}

void cmd_update(void* param)
{
    (void)param;
    // if (update_configuration() == 0)
    // {
    //     printf("Configuration updated.\n");
    // }
    // else
    // {
    //     printf("Failed to update configuration.\n");
    // }
}

void cmd_kms(void* param)
{
    (void)param;
    kill_me();
    pthread_exit(NULL);
}

void cmd_print_logs(void* param)
{
    task_t* tasks = get_active_tasks();

    if (param && strcmp((const char*)param, "all") != 0)
    {
        const char* task_name = (const char*)param;
        task_t* task = tasks;
        while (task)
        {
            if (strcmp(task->parser.name, task_name) == 0)
            {
                print_logs(task);
                return;
            }
            task = FT_LIST_GET_NEXT(&tasks, task);
        }
        fprintf(stdout, "Task not found: %s\n", task_name);
    }
    else
    {
        task_t* task = tasks;
        while (task)
        {
            fprintf(stdout, "\n#################### %s ####################\n", task->parser.name);
            print_logs(task);
            task = FT_LIST_GET_NEXT(&tasks, task);
        }
    }
}

void cmd_kill_supervisor(void* param)
{
    (void)param;
    kill_me();
}

/* this must prompt and request for whole parser
 * parameters in order to create task.
 */
void cmd_new(void* param)
{
    (void)param;
    // if (create_new_task() == 0)
    // {
    //     printf("New task created.\n");
    // }
    // else
    // {
    //     printf("Failed to create new task.\n");
    // }
}

void print_task(task_t* task)
{    
    fprintf(stdout, "Task name: %s\n", task->parser.name);
    fprintf(stdout, "Command: %s\n", task->parser.cmd);
    fprintf(stdout, "Directory: %s\n", task->parser.dir);
    fprintf(stdout, "Environment:\n");
    for (size_t i = 0; task->parser.env[i]; ++i)
    {
        fprintf(stdout, "\t%s\n", task->parser.env[i]);
    }
    fprintf(stdout, "Autostart: %s\n", task->parser.autostart ? "true" : "false");
    fprintf(stdout, "Autorestart: %s\n", get_autorestart_str(task->parser.ar));
    fprintf(stdout, "Startretries: %d\n", task->parser.startretries);
    fprintf(stdout, "Starttime: %d\n", task->parser.starttime);
    fprintf(stdout, "Stoptime: %d\n", task->parser.stoptime);
    fprintf(stdout, "Exitcodes:\n");
    for (size_t i = 0; task->parser.exitcodes[i]; ++i)
    {
        fprintf(stdout, "\t%d\n", task->parser.exitcodes[i]);
    }
    fprintf(stdout, "Stopsignal: %d\n", task->parser.stopsignal);
    fprintf(stdout, "Stoptimeout: %d\n", task->parser.stoptimeout);
    fprintf(stdout, "Stdout: %s\n", task->parser.stdout);
    fprintf(stdout, "Stderr: %s\n", task->parser.stderr);
    fprintf(stdout, "Umask: %d\n", task->parser.umask);

}

void cmd_show_task(void* task)
{
    const char* task_name = (const char*)task;
    if (task_name)
    {
        task_t* t = get_task_from_name(task_name);
        if (t)
        {
            print_task(t);
        }
        else
        {
            fprintf(stdout, "Task not found: %s\n", task_name);
        }
    }
    else
    {
        task_t* tasks = get_active_tasks();
        task_t* t = tasks;
        while (t)
        {
            print_task(t);
            t = FT_LIST_GET_NEXT(&tasks, t);
            if (t)
            {
                fprintf(stdout, "\n##############################################################\n");
            }
        }
    }
}

void cmd_modify_task(void* param)
{
    if (param == NULL)
    {
        fprintf(stdout, "Usage: modify <task_name>\n");
        return;
    }
    const char* task_name = (const char*)param;
    task_t* task = get_task_from_name(task_name);
    if (task)
    {
        fprintf(stdout, "Modify of task %s requested.\n", task_name);
        // if (modify_task() == 0)
        // {
        //     printf("Task modified.\n");
        // }
        // else
        // {
        //     printf("Failed to modify task.\n");
        // }
    }
    else
    {
        fprintf(stdout, "Requested task not found\n");
    }
}