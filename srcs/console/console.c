#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

#include <taskmaster.h>
#include <ft_malloc.h>

void cmd_help(void* param);
void cmd_active(void* param);
void cmd_stop(void* param);
void cmd_update(void* param);
void cmd_kms(void* param);
void cmd_print_logs(void* param);
void cmd_kill_supervisor(void* param);
void cmd_start(void* param);
void cmd_new(void* param);
void cmd_restart(void* param);
void cmd_delete(void* param);
void cmd_show_task(void* task);
void cmd_modify_task(void* param);
void cmd_time(void* param);

typedef enum {
    NEW_SUCCESS,
    NEW_FAILURE,
    NEW_EMPTY
} new_task_return;

typedef struct {
    const char* name;
    void (*func)(void* param);
    const char* description;
} command_t;

command_t commands[] = {
    {"help", cmd_help, "Show this help menu"},
    {"new", cmd_new, "Create a new task"},
    {"status", cmd_active, "Show tasks status"},
    {"start", cmd_start, "Start a specific task by name"},
    {"stop", cmd_stop, "Stop a specific task by name"},
    {"restart", cmd_restart, "Restart a specific task by name"},
    {"delete", cmd_delete, "Delete specific task by name."},
    {"update", cmd_update, "Re-read the configuration file"},
    {"logs", cmd_print_logs, "Print logs for a specific task or all tasks"},
    {"kill", cmd_kill_supervisor, "Kill the supervisor"},
    {"kms", cmd_kms, "Finish the program execution."},
    {"show", cmd_show_task, "Show task details."},
    {"modify", cmd_modify_task, "Modify task details."},
    {"time", cmd_time, "Show the current time."},
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

    while (exit_flag == false)
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

static void request_action_on_task(const char* task_name, cmd_request request)
{
    task_t* task = get_task_from_name(task_name);
    if (task)
    {
        update_task_cmd_state(task, request);
    }
    else
    {
        fprintf(stdout, "Requested task not found\n");
    }
}

void cmd_restart(void* param)
{
    if (param == NULL)
    {
        fprintf(stdout, "Usage: restart <task_name>\n");
        return;
    }
    request_action_on_task((const char*)param, CMD_RESTART);
}

void cmd_start(void* param)
{
    if (param == NULL)
    {
        fprintf(stdout, "Usage: start <task_name>\n");
        return;
    }
    request_action_on_task((const char*)param, CMD_START);
}

void cmd_stop(void* param)
{
    if (param == NULL)
    {
        fprintf(stdout, "Usage: stop <task_name>\n");
        return;
    }
    request_action_on_task((const char*)param, CMD_STOP);
}

void cmd_delete(void* param)
{
    if (param == NULL)
    {
        fprintf(stdout, "Usage: stop <task_name>\n");
        return;
    }
    request_action_on_task((const char*)param, CMD_DELETE);
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

new_task_return parse_line(task_format format, void* param)
{
    char* line = NULL;
    size_t len = 0;
    ssize_t read;
    
    read = getline(&line, &len, stdin);
    if (read == -1)
    {
        fprintf(stdout, "Failed to read line.\n");
        return NEW_FAILURE;
    }

    if (line[read - 1] == '\n')
    {
        line[read - 1] = '\0';
    }

    if (line[0] == '\0')
    {
        free(line);
        return NEW_EMPTY;
    }
    
    switch (format)
    {
    case NEW_PARSE_STRING:
        *(char**)param = strdup(line);
        break;
    case NEW_PARSE_INT:
        *(int*)param = atoi(line);
        break;
    case NEW_PARSE_BOOL:
        *(bool*)param = (strcasecmp(line, "true") == 0) ? true : false;
        break;
    case NEW_PARSE_ARRAY:
        *(char**)param = strdup(line); /* TODO */
        break;
    case NEW_PARSE_AR:
        *(AR_modes*)param = parse_autorestart(line);
        break;
    default:
        fprintf(stdout, "Unknown format.\n");
        free(line);
        return NEW_FAILURE;
    }
    free(line);
    return NEW_SUCCESS;
}

int conf_parse_format(const char* string, task_format format, void*  default_value,\
                        void* param, task_param param_type, bool mandatory)
{
    fprintf(stdout, "%s: ", string);

    switch (parse_line(format, param))
    {
    case NEW_SUCCESS:
        break;
    case NEW_EMPTY:
        if (mandatory == true)
            return -1;
        switch (param_type)
        {
        case NEW_PARAM_STRING:
            if (default_value != NULL)
                *(char**)param = strdup((char*)default_value);
            else
                *(char**)param = NULL;
            break;
        case NEW_PARAM_INT:
            *(int*)param = *(int*)default_value;
            break;
        case NEW_PARAM_BOOL:
            *(bool*)param = *(bool*)default_value;
            break;
        case NEW_PARAM_ARRAY:
            *(char**)param = NULL;
            break;
        case NEW_PARAM_AR:
            *(AR_modes*)param = *(AR_modes*)default_value;
            break;
        }
        break;
    case NEW_FAILURE:
        return -1;
    }

    return 1;
}

/* this must prompt and request for whole parser
 * parameters in order to create task.
 */
void cmd_new(void* param)
{
    (void)param;

    task_t* task = NEW(task_t, 1);
    if (!task)
    {
        fprintf(stdout, "Error: Failed to allocate memory for the task.\n");
        return;
    }

    fprintf(stdout, "Request to create a new task. Please fill in the details.\n");
    fprintf(stdout, "Parameters marked with '*' are mandatory.\n");

    typedef struct {
        const char* prompt;
        int parse_type;
        void* default_value;
        void** target;
        int param_type;
        bool required;
    } ParamConfig;

    ParamConfig configs[] = {
        {"Task name*", NEW_PARSE_STRING, NULL, (void**)&task->parser.name, NEW_PARAM_STRING, true},
        {"Command*", NEW_PARSE_STRING, NULL, (void**)&task->parser.cmd, NEW_PARAM_STRING, true},
        {"Args (format: arg1 arg2 ... argN)", NEW_PARSE_ARRAY, NULL, (void**)&task->parser.args, NEW_PARAM_ARRAY, false},
        {"Directory", NEW_PARSE_STRING, NULL, (void**)&task->parser.dir, NEW_PARAM_STRING, false},
        {"Environment (format: env1=value1 env2=value2 ...)", NEW_PARSE_ARRAY, NULL, (void**)&task->parser.env, NEW_PARAM_ARRAY, false},
        {"Autostart (true/false)", NEW_PARSE_BOOL, &(bool){false}, (void**)&task->parser.autostart, NEW_PARAM_BOOL, false},
        {"Autorestart (always/unexpected/success/failure/never)", NEW_PARSE_AR, &(AR_modes){NEVER}, (void**)&task->parser.ar, NEW_PARAM_AR, false},
        {"Startretries", NEW_PARSE_INT, &(int){0}, (void**)&task->parser.startretries, NEW_PARAM_INT, false},
        {"Starttime", NEW_PARSE_INT, &(int){0}, (void**)&task->parser.starttime, NEW_PARAM_INT, false},
        {"Stoptime", NEW_PARSE_INT, &(int){0}, (void**)&task->parser.stoptime, NEW_PARAM_INT, false},
        {"Exitcodes (format: code1 code2 ...)", NEW_PARSE_ARRAY, NULL, (void**)&task->parser.exitcodes, NEW_PARAM_ARRAY, false},
        {"Stopsignal (default: 9)", NEW_PARSE_INT, &(int){9}, (void**)&task->parser.stopsignal, NEW_PARAM_INT, false},
        {"Stoptimeout", NEW_PARSE_INT, &(int){0}, (void**)&task->parser.stoptimeout, NEW_PARAM_INT, false},
        {"Stdout", NEW_PARSE_STRING, NULL, (void**)&task->parser.stdout, NEW_PARAM_STRING, false},
        {"Stderr", NEW_PARSE_STRING, NULL, (void**)&task->parser.stderr, NEW_PARAM_STRING, false},
        {"Umask", NEW_PARSE_INT, &(int){0}, (void**)&task->parser.umask, NEW_PARAM_INT, false},
    };

    for (size_t i = 0; i < sizeof(configs) / sizeof(configs[0]); i++)
    {
        if (conf_parse_format(configs[i].prompt, configs[i].parse_type, configs[i].default_value,
            configs[i].target, configs[i].param_type, configs[i].required) == -1)
        {
            fprintf(stdout, "Error parsing parameter: %s\n", configs[i].prompt);
            goto failure;
        }
    }

    add_task_to_list(task);
    fprintf(stdout, "Task created successfully.\n");
    return;

failure:
    fprintf(stdout, "Failed to create task. Cleaning up resources.\n");
    free_task(task);
    return;
}

void print_task(task_t* task)
{
    const int field_width = 15; // Adjust the width as needed

    fprintf(stdout, "%-*s %s\n", field_width, "Task name:", task->parser.name);
    fprintf(stdout, "%-*s %s", field_width, "Command:", task->parser.cmd);
    if (task->parser.args)
    {
        fprintf(stdout, " ");
        for (size_t i = 0; task->parser.args[i]; ++i)
        {
            fprintf(stdout, "%s ", task->parser.args[i]);
        }
    }
    fprintf(stdout, "\n");
    fprintf(stdout, "%-*s %s\n", field_width, "Directory:", task->parser.dir);
    fprintf(stdout, "%-*s\n", field_width, "Environment:");
    if (task->parser.env == NULL)
        fprintf(stdout, "\tNULL\n");
    else
        for (size_t i = 0; task->parser.env[i]; ++i)
            fprintf(stdout, "\t%s\n", task->parser.env[i]);

    fprintf(stdout, "%-*s %s\n", field_width, "Autostart:", task->parser.autostart ? "true" : "false");
    fprintf(stdout, "%-*s %s\n", field_width, "Autorestart:", get_autorestart_str(task->parser.ar));
    fprintf(stdout, "%-*s %d\n", field_width, "Startretries:", task->parser.startretries);
    fprintf(stdout, "%-*s %d\n", field_width, "Starttime:", task->parser.starttime);
    fprintf(stdout, "%-*s %d\n", field_width, "Stoptime:", task->parser.stoptime);
    fprintf(stdout, "%-*s\n", field_width, "Exitcodes:");
    if (task->parser.exitcodes == NULL)
    {
        fprintf(stdout, "\tNULL\n");
    }
    else
        for (int i = 0; i < task->parser.exitcodes[0]; ++i)
            fprintf(stdout, "\t%d\n", task->parser.exitcodes[i]);

    fprintf(stdout, "%-*s %d\n", field_width, "Stopsignal:", task->parser.stopsignal);
    fprintf(stdout, "%-*s %d\n", field_width, "Stoptimeout:", task->parser.stoptimeout);
    fprintf(stdout, "%-*s %s\n", field_width, "Stdout:", task->parser.stdout);
    fprintf(stdout, "%-*s %s\n", field_width, "Stderr:", task->parser.stderr);
    fprintf(stdout, "%-*s %d\n", field_width, "Umask:", task->parser.umask);
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

static int modify_task(task_t* task, const char* param)
{
    typedef struct
    {
        const char* param_name;
        const char* prompt;
        int parse_type;
        void* default_value;
        void** task_member;
        int param_type;
        bool required;
        bool free;
    } ParamConfig;

    ParamConfig configs[] = {
        {"name", "Task name*", NEW_PARSE_STRING, NULL, (void**)&task->parser.name, NEW_PARAM_STRING, true, true},
        {"cmd", "Command*", NEW_PARSE_STRING, NULL, (void**)&task->parser.cmd, NEW_PARAM_STRING, true, true},
        {"args", "Args format: arg1 arg2 arg3 ... argN", NEW_PARSE_ARRAY, NULL, (void**)&task->parser.args, NEW_PARAM_ARRAY, false, true},
        {"dir", "Directory", NEW_PARSE_STRING, NULL, (void**)&task->parser.dir, NEW_PARAM_STRING, false, true},
        {"env", "Environment format: env1=value1 env2=value2 ... envN=valueN", NEW_PARSE_ARRAY, NULL, (void**)&task->parser.env, NEW_PARAM_ARRAY, false, true},
        {"autostart", "Autostart (true/false)", NEW_PARSE_BOOL, &(bool){false}, (void**)&task->parser.autostart, NEW_PARAM_BOOL, false, false},
        {"ar", "Autorestart (always/unexpected/success/failure/never)", NEW_PARSE_AR, &(AR_modes){NEVER}, (void**)&task->parser.ar, NEW_PARAM_AR, false, false},
        {"startretries", "Startretries", NEW_PARSE_INT, &(int){0}, (void**)&task->parser.startretries, NEW_PARAM_INT, false, false},
        {"starttime", "Starttime", NEW_PARSE_INT, &(int){0}, (void**)&task->parser.starttime, NEW_PARAM_INT, false, false},
        {"stoptime", "Stoptime", NEW_PARSE_INT, &(int){0}, (void**)&task->parser.stoptime, NEW_PARAM_INT, false, false},
        {"exitcodes", "Exitcodes format: code1 code2 ... codeN", NEW_PARSE_ARRAY, NULL, (void**)&task->parser.exitcodes, NEW_PARAM_ARRAY, false, true},
        {"stopsignal", "Stopsignal", NEW_PARSE_INT, &(int){9}, (void**)&task->parser.stopsignal, NEW_PARAM_INT, false, false},
        {"stoptimeout", "Stoptimeout", NEW_PARSE_INT, &(int){0}, (void**)&task->parser.stoptimeout, NEW_PARAM_INT, false, false},
        {"stdout", "Stdout", NEW_PARSE_STRING, NULL, (void**)&task->parser.stdout, NEW_PARAM_STRING, false, true},
        {"stderr", "Stderr", NEW_PARSE_STRING, NULL, (void**)&task->parser.stderr, NEW_PARAM_STRING, false, true},
        {"umask", "Umask", NEW_PARSE_INT, &(int){0}, (void**)&task->parser.umask, NEW_PARAM_INT, false, false},
    };

    if (task == NULL || param == NULL)
    {
        fprintf(stdout, "List of parameters:\n");
        for (size_t i = 0; i < sizeof(configs) / sizeof(configs[0]); i++)
        {
            fprintf(stdout, "%s%s", configs[i].param_name, (i == sizeof(configs) / sizeof(configs[0]) - 1) ? "" : ", ");
        }
        fprintf(stdout, "\n");
        return -1;
    }

    for (size_t i = 0; i < sizeof(configs) / sizeof(configs[0]); i++)
    {
        if (strcasecmp(param, configs[i].param_name) == 0)
        {
            void* temp_value = NULL;
            if (conf_parse_format(configs[i].prompt, configs[i].parse_type, configs[i].default_value,
                &temp_value, configs[i].param_type, configs[i].required) == -1)
            {
                fprintf(stderr, "Error: Failed to parse parameter '%s'.\n", param);
                return -1;
            }

            modify_task_param(configs[i].task_member, &temp_value, configs[i].param_type, configs[i].free);

            return 0;
        }
    }

    fprintf(stdout, "Unknown parameter: %s. Try \"modify -h\" for the full list of params.\n", param);
    return -1;
}

void cmd_modify_task(void* param)
{
    if (!param)
    {
        fprintf(stdout, "Usage: modify <task_name>.\n");
        return;
    }

    if (strcmp((const char*)param, "-h") == 0)
    {
        modify_task(NULL, NULL);
        return;
    }

    const char* task_name = (const char*)param;
    task_t* task = get_task_from_name(task_name);
    if (!task)
    {
        fprintf(stdout, "Task '%s' not found.\n", task_name);
        return;
    }

    fprintf(stdout, "Modifying task: %s\n", task_name);
    fprintf(stdout, "Enter parameters to modify (space-separated, e.g., 'name cmd'):\n");

    char* line = NULL;
    size_t len = 0;

    if (getline(&line, &len, stdin) == -1)
    {
        fprintf(stdout, "Error reading input. Modification cancelled.\n");
        free(line);
        return;
    }

    line[strcspn(line, "\n")] = '\0';

    if (line[0] == '\0')
    {
        fprintf(stdout, "No parameters provided. Modification cancelled.\n");
        free(line);
        return;
    }

    char* token = strtok(line, " ");
    while (token)
    {
        if (modify_task(task, token) == -1)
        {
            fprintf(stdout, "Error modifying parameter: %s\n", token);
        }
        token = strtok(NULL, " ");
    }

    fprintf(stdout, "Task modification completed.\n");
    free(line);
}

void cmd_time(void* param)
{
    (void)param;
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    fprintf(stdout, "Current time: %d-%02d-%02d %02d:%02d:%02d\n", tm.tm_year + 1900,\
    tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
}
