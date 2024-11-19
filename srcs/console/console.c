#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <taskmaster.h>
#include <signal.h>

// Command function prototypes
void cmd_help(void* param);
void cmd_active(void* param);
void cmd_stop(void* param);
void cmd_update(void* param);
void cmd_kms(void* param);
void cmd_print_logs(void* param);
void cmd_kill_supervisor(void* param);
void cmd_start(void* param);

typedef struct {
    const char* name;
    void (*func)(void* param);
    const char* description;
} command_t;

// Command table
command_t commands[] = {
    {"help", cmd_help, "Show this help menu"},
    {"status", cmd_active, "Show tasks status"},
    {"stop", cmd_stop, "Stop a specific task by name"},
    {"update", cmd_update, "Re-read the configuration file"},
    {"kms", cmd_kms, "Kill the master supervisor"},
    {"logs", cmd_print_logs, "Print logs for a specific task or all tasks"},
    {"kill", cmd_kill_supervisor, "Kill the supervisor"},
    {"start", cmd_start, "Start a specific task by name"},
    {NULL, NULL, NULL} // Sentinel value to mark end of table
};

// Helper function to find a command in the table
command_t* find_command(const char* name)
{
    for (command_t* cmd = commands; cmd->name != NULL; ++cmd)
    {
        if (strcmp(cmd->name, name) == 0)
        {
            return cmd;
        }
    }
    return NULL;
}

void* interactive_console(void* param)
{
    char input[256];
    (void)param;

    printf("Interactive Console. Type 'help' for a list of commands.\n");

    while (1)
    {
        printf("-> ");
        if (fgets(input, sizeof(input), stdin) == NULL)
        {
            break; // Handle EOF or error
        }

        // Remove trailing newline
        input[strcspn(input, "\n")] = 0;

        if (strcmp(input, "exit") == 0)
        {
            break;
        }

        // Parse the command
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
            printf("Unknown command: '%s'. Type 'help' for a list of commands.\n", cmd_name);
        }
    }

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

// Command implementations
void cmd_help(void* param)
{
    (void)param;
    printf("Available commands:\n");
    for (command_t* cmd = commands; cmd->name != NULL; ++cmd)
    {
        printf("  %s: %s\n", cmd->name, cmd->description);
    }
}

void cmd_active(void* param)
{
    (void)param;
    task_t* tasks = get_active_tasks();
    if (tasks)
    {
        printf("Active tasks:\n");
        task_t* task = tasks;
        while (task)
        {
            printf("\t- %s :\t%s\n", task->name, get_state_string(task->state));
            task = FT_LIST_GET_NEXT(&tasks, task);
        }
    }
    else
    {
        printf("No active tasks.\n");
    }
}

void cmd_start(void* param)
{
    if (param == NULL)
    {
        printf("Usage: start <task_name>\n");
        return;
    }
    const char* task_name = (const char*)param;
    task_t* tasks = get_active_tasks();
    if (tasks)
    {
        task_t* task = tasks;
        while (task)
        {
            if (strcmp(task->name, task_name) == 0)
            {
                update_task_cmd_state(task, CMD_START);
                return;
            }
            task = FT_LIST_GET_NEXT(&tasks, task);
        }
        printf("Task not found: %s\n", task_name);
    }
    else
    {
        printf("No active tasks.\n");
    }
}

void cmd_stop(void* param)
{
    if (param == NULL)
    {
        printf("Usage: stop <task_name>\n");
        return;
    }
    // const char* task_name = (const char*)param;
    // if (stop_task(task_name) == 0)
    // {
    //     printf("Stopped task: %s\n", task_name);
    // }
    // else
    // {
    //     printf("Failed to stop task: %s\n", task_name);
    // }
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
            if (strcmp(task->name, task_name) == 0)
            {
                print_logs(task);
                return;
            }
            task = FT_LIST_GET_NEXT(&tasks, task);
        }
        printf("Task not found: %s\n", task_name);
    }
    else
    {
        task_t* task = tasks;
        while (task)
        {
            printf("\n#################### %s ####################\n", task->name);
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
