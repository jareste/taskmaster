
#include <signal.h>
#include <ctype.h>
#include <string.h> 
#include <errno.h>
#include <limits.h>
#include <stdio.h>

#include <ft_malloc.h>

#include <taskmaster.h>

struct task_t* parse_config(char *file_path);
int validate_cmd(char *value, struct task_t *task, unsigned int line_number);
bool validate_ints(char *value, struct task_t *task, int identify, unsigned int line_number);
char	*ft_substr(char *s, unsigned int start, size_t len);
char *ft_strtrim(char *s1, char *set);
task_t *new_task(char *name_service);
int validate_str(char *value, struct task_t *task, unsigned int line_number, int identify);
void validate_exitcodes(char *value, struct task_t *task, unsigned int line_number);
void validate_envs(char *line, struct task_t *task, unsigned int line_number);
char	**ft_split(char *s);

char** parse_array(char* str)
{
(void)str;
return NULL;
}

int env_count = 0;
int env_active = 0;

int parse_stopsignal(const char *str, int line_number)
{
    if (strcasecmp(str, "SIGHUP") == 0 || strcasecmp(str, "HUP") == 0) return SIGHUP;
    else if (strcasecmp(str, "SIGINT") == 0 || strcasecmp(str, "INT") == 0) return SIGINT;
    else if (strcasecmp(str, "SIGQUIT") == 0 || strcasecmp(str, "QUIT") == 0) return SIGQUIT;
    else if (strcasecmp(str, "SIGILL") == 0 || strcasecmp(str, "KILL") == 0) return SIGILL;
    else if (strcasecmp(str, "SIGABRT") == 0 || strcasecmp(str, "ABRT") == 0) return SIGABRT;
    else if (strcasecmp(str, "SIGFPE") == 0 || strcasecmp(str, "FPE") == 0) return SIGFPE;
    else if (strcasecmp(str, "SIGKILL") == 0 || strcasecmp(str, "KILL") == 0) return SIGKILL;
    else if (strcasecmp(str, "SIGUSR1") == 0 || strcasecmp(str, "USR1") == 0) return SIGUSR1;
    else if (strcasecmp(str, "SIGSEGV") == 0 || strcasecmp(str, "SEGV") == 0) return SIGSEGV;
    else if (strcasecmp(str, "SIGUSR2") == 0 || strcasecmp(str, "USR2") == 0) return SIGUSR2;
    else if (strcasecmp(str, "SIGPIPE") == 0 || strcasecmp(str, "PIPE") == 0) return SIGPIPE;
    else if (strcasecmp(str, "SIGALRM") == 0 || strcasecmp(str, "ALRM") == 0) return SIGALRM;
    else if (strcasecmp(str, "SIGTERM") == 0 || strcasecmp(str, "TERM") == 0) return SIGTERM;
    else if (strcasecmp(str, "SIGCHLD") == 0 || strcasecmp(str, "CHLD") == 0) return SIGCHLD;
    else if (strcasecmp(str, "SIGCONT") == 0 || strcasecmp(str, "CONT") == 0) return SIGCONT;
    else if (strcasecmp(str, "SIGSTOP") == 0 || strcasecmp(str, "STOP") == 0) return SIGSTOP;
    else if (strcasecmp(str, "SIGTSTP") == 0 || strcasecmp(str, "TSTP") == 0) return SIGTSTP;
    else if (strcasecmp(str, "SIGTTIN") == 0 || strcasecmp(str, "TTIN") == 0) return SIGTTIN;
    else if (strcasecmp(str, "SIGTTOU") == 0 || strcasecmp(str, "TTOU") == 0) return SIGTTOU;
    else printf("Error linea %d: Introduce una señal valida.\n", line_number);
    return 0; // valor si no encuentra la señal
}

void fill_fields(char *name, char *value, struct task_t *task, unsigned int line_number)
{
    if (strcmp("cmd", name) == 0)
        validate_cmd(value, task, line_number);
    else if (strcmp("numprocs", name) == 0)
        validate_ints(value, task, 1, line_number);
    else if (strcmp("umask", name) == 0)
        validate_str(value, task, line_number, 1);
    else if (strcmp("workingdir", name) == 0)
        validate_str(value, task, line_number, 2);
    else if (strcmp("autostart", name) == 0)
        validate_str(value, task, line_number, 3);
    else if (strcmp("autorestart", name) == 0)
        validate_str(value, task, line_number, 4);
    else if (strcmp("exitcodes", name) == 0)
        validate_exitcodes(value, task, line_number);
    else if (strcmp("startretries", name) == 0)
        validate_ints(value, task, 2, line_number);
    else if (strcmp("starttime", name) == 0)
        validate_ints(value, task, 3, line_number);
    else if (strcmp("stopsignal", name) == 0)
        task->parser.stopsignal = parse_stopsignal(value, line_number);
    else if (strcmp("stoptime", name) == 0)
        validate_ints(value, task, 4, line_number);
    else if (strcmp("stoptimeout", name) == 0)
        validate_ints(value, task, 5, line_number);
    else if (strcmp("stdout", name) == 0)
        validate_str(value, task, line_number, 5);
    else if (strcmp("stderr", name) == 0)
        validate_str(value, task, line_number, 6);
    else
        fprintf(stderr, "Error linea %d: El campo introducido %s no es valido.\n", line_number, name);//esto deberia ir en los logs
}

void check_config_values(char *line, struct task_t *task, unsigned int line_number)
{
    //printf("N: %d\n", line_number);
    char *name = ft_substr(line, 0, strchr(line, ':') - line);
    char *tmp_value = ft_substr(line, strchr(line, ':') - line, strlen(line) - (size_t)(strchr(line, ':') - line));
    char *tmp_value_cp = tmp_value;
    if (strcmp(name, "env") == 0)
    {
        env_active = 1;
        return ;
    } 
    //char *tmp_value = ft_substr(line, (unsigned int)(strchr(line, ':') - line), (unsigned int)strlen(line) - (unsigned int)(strchr(line, ':') - line));
    if (tmp_value && tmp_value[0] == ':' && tmp_value[1])
        ++tmp_value_cp;
    else if (strcmp(name, "env") != 0)
    {
        free(tmp_value);
        free(name);
        printf("Error linea %d: Despues de ':' debes introducir un valor.\n", line_number);
        return ;
    }
    char *value = ft_strtrim(tmp_value_cp, " \t\n\r\v\f");
    if (strlen(value) != 0)
        fill_fields(name, value, task, line_number);
    else
        printf("Error linea %d: Despues de ':' debes introducir un valor.\n", line_number);
    if (tmp_value)
        free(tmp_value);
    if (value)
        free(value);
    if (name)
        free(name);
}

char *rm_space(char *s)
{
    char *res;
    int i = 0;

    while (isspace(s[i]))
            ++i;
    int j = strlen(s) - 1;
    while (j > 0 && isspace(s[j]))
        j--;
    if (j == 0)
        j = strlen(s) - 1;
    res = ft_substr(s, i, j - i + 1);
    return (res);
}


task_t* parse_config(char *file_path)
{
    char *line = NULL;
    size_t len = 0;
    unsigned int line_number = 0;
    task_t *current_task = NULL;
    task_t *tasks = NULL;

    if (!file_path)
        return NULL;
    FILE *file = fopen(file_path, "r");
    if (!file)
        return NULL;
    while (getline(&line, &len, file) != -1)
    {
        line_number++;
        line[strcspn(line, "\n")] = '\0';
        char *trimmed_line = line;
        while (isspace(*trimmed_line))
            ++trimmed_line;
        int j = strlen(trimmed_line) - 1;
        while (j > 0 && isspace(trimmed_line[j]))
            j--;
        if (j == 0)
            j = strlen(trimmed_line) - 1;
        if (*trimmed_line == '\0')
            continue;
        if (trimmed_line[0] == '-' && current_task && env_active == 1)
        {
            validate_envs(ft_strtrim(++trimmed_line, " \t\v"), current_task, line_number);
            continue ;
        }
        if (trimmed_line[0] == '[' && trimmed_line[1] && trimmed_line[j] == ']')
        {
            if (current_task)
                env_active = 0;
            trimmed_line++; //aqui antes estaba asi *trimedline++;
            trimmed_line[j - 1] = '\0';
            trimmed_line = rm_space(trimmed_line);
            current_task = new_task(trimmed_line);
            if (!current_task)
            {
                printf("FALLA MALLOC\n"); //meter logs
                fclose(file);
                //free_tasks(tasks); NO BORRAR, DESCOMENTAR CUANDO SE PASE A LIMPIO 
                free(line);
                return (NULL);
            }
            FT_LIST_ADD_LAST(&tasks, current_task);
        }
        else if (current_task && strchr(trimmed_line, ':'))
        {
            env_active = 0;
            while (isspace(*trimmed_line))
                ++trimmed_line;
            check_config_values(trimmed_line, current_task, line_number);
        }
        else
        {
            fprintf(stderr, "Error linea %d: El valor introducido %s no es valido.\n", line_number, line);
        }
    }
    free(line);
    fclose(file);
    create_config_file("nada", tasks); //cambiar nombre del fichero al bueno
    return (tasks);
}

task_t *new_task(char *name_service)
{
    task_t *task = (task_t*)malloc(sizeof(task_t));
    task->parser.name = strdup(name_service);
    task->parser.cmd = NULL;
    task->parser.args = NULL;
    task->parser.dir = NULL;
    task->parser.stdout = NULL;
    task->parser.stderr = NULL;
    task->parser.autostart = true;
    task->parser.ar = ALWAYS;
    task->parser.startretries = 0;
    task->parser.starttime = 0;
    task->parser.stoptime = 0;
    task->parser.exitcodes = NULL;
    task->parser.stopsignal = 0;
    task->parser.stoptimeout = 0;
    task->parser.umask = 0;
    task->parser.procs = 0;
    task->parser.env = NULL;
    return task;
}

char* get_autorestart_str(AR_modes ar)
{
    switch (ar)
    {
        case ALWAYS: return "always";
        case UNEXPECTED: return "unexpected";
        case SUCCESS: return "success";
        case FAILURE: return "failure";
        case NEVER: return "never";
        default: return "unknown";
    }
}

AR_modes parse_autorestart(char *str)
{
    if (strcasecmp(str, "always") == 0)
        return (ALWAYS);
    else if (strcasecmp(str, "unexpected") == 0)
        return (UNEXPECTED);
    else if (strcasecmp(str, "success") == 0)
        return (SUCCESS);
    else if (strcasecmp(str, "failure") == 0)
        return (FAILURE);
    else if (strcasecmp(str, "never") == 0)
        return (NEVER);
    /* failure!!! */
    return (NEVER);
}


//VALIDATE FILE

bool is_numeric(char *str, int line_number)
{
    if (*str == '+' || *str == '-')
        str++;
    
    while (*str)
    {
        if (!isdigit(*str))
        {
            printf("Error linea %d: Caracter no numerico.\n", line_number);
            return false;
        }
        ++str;
    }
    return true;
}

bool validate_ints(char *value, struct task_t *task, int identify, unsigned int line_number)
{
    if (is_numeric(value, line_number))
    {
         errno = 0;
         long result = strtol(value, NULL, 10);

        if (errno == ERANGE || result > INT_MAX || result < INT_MIN)
        {
            printf("Error linea %d: El número proporcionado está fuera del rango de int.\n", line_number);
            return false;
        }
        if (identify == 1)
            task->parser.procs = (int)result;
        else if (identify == 2)
            task->parser.startretries = (int)result;
        else if (identify == 3)
            task->parser.starttime = (int)result;
        else if (identify == 4)
            task->parser.stoptime = (int)result;
        else if (identify == 5)
            task->parser.stoptimeout = (int)result;
        return true;
    }
    return false;    
}

void allocate_envs(struct task_t *task)
{
    if (!task->parser.env)
    {
        env_count = 1;
        task->parser.env = malloc(sizeof(char *) * (env_count + 1));
    }
    else
    {
        env_count++;
        task->parser.env = realloc(task->parser.env, sizeof(char *) * (env_count + 1));
    }
    task->parser.env[env_count] = NULL;
}

void validate_envs(char *line, struct task_t *task, unsigned int line_number)
{
    int i = 0;
    int j = 0;

    if (!line || line[0] == '\0')
    {
        printf("Error linea %d: Debes introducir valores despues del '-'.\n", line_number);
    }
    while (line[i])
    {
        if (isspace(line[i]))
        {
            printf("Error linea %d: Hay espacios no validos.\n", line_number);
            return ;
        }
        if (line[i] == '=')
            j++;
        if (j > 1)
        {
            printf("Error linea %d: Hay mas de un '='.\n", line_number);
            return ;
        }
        i++;
    }
    i = 0;
    while (line[i] && line[i] != '=')
        i++;
    if (line[i] != '=')
    {
        printf("Error linea %d: Debes incluir '=' para asignar valor a la env.\n", line_number);
        return ;
    }
    j = strlen(line);
    if (j == i + 1)
    {
        printf("Error linea %d: Debes introducir un valor despues de '='.\n", line_number);
        return ;
    }
    allocate_envs(task);
    task->parser.env[env_count - 1] = line;
}

int validate_cmd(char *value, struct task_t *task, unsigned int line_number)
{
    char *res;
    int i = -1;
    int quotes = 0;

    if (value[0] != '"' || value[strlen(value) - 1] != '"')
        return (printf("Error linea %u: El comando debe estar escrito entre comillas dobles\n", line_number));
    while (value[++i])
    {
        if (value[i] == '"')
            quotes++;
    }
    if (quotes != 2)
        return (printf("Error linea %u: El comando debe contener 2 comillas dobles\n", line_number));
    res = ft_substr(value, 1, strlen(value) - 2); //el startt 1 y len -2 es para qu itar las comillas dobles
    if (strlen(res) == 0)
    {
        printf("Error linea %d: Comillas dobles vacias\n", line_number);
        free(res);
    }
    else
    {
        i = 0;
        while (res[i] && !isspace(res[i]))
            i++;
        task->parser.cmd = ft_substr(res, 0, i);
        if (task->parser.cmd && task->parser.cmd[0] != '/')
        {
            printf("Error linea %d: El comando debe empezar por '/'\n", line_number);
            free(task->parser.cmd);
            task->parser.cmd = NULL;
        }
        else
            task->parser.args = ft_split(res);
    }
    return (0);
}

void validate_bool(char *value, struct task_t *task, unsigned int line_number)
{
    if (strcmp(value, "1") == 0 || strcmp(value, "true") == 0)
        task->parser.autostart = true;
    else if (strcmp(value, "0") == 0 || strcmp(value, "false") == 0)
        task->parser.autostart = false;
    else
    {
        printf("Error linea %d: Autostart solo acepta valores true/false.\n", line_number);
    }
}

int validate_str(char *value, struct task_t *task, unsigned int line_number, int identify)
{
    int i = 0;

    while (value[i])
    {
        if (isspace(value[i]))
        {
            printf("Error linea %d: Solo se espera un argumento.\n", line_number);
            return (-1);
        }
        i++;
    }
    if (identify == 1)
        task->parser.umask = strdup(value);
    else if (identify == 2)
        task->parser.dir = strdup(value);
    else if (identify == 3)
        validate_bool(value, task, line_number);
    else if (identify == 4)
        task->parser.ar = parse_autorestart(value);
    else if (identify == 5)
        task->parser.stdout = strdup(value);
    else if (identify == 6)
        task->parser.stderr = strdup(value);
    else if (identify == 7)
        validate_exitcodes(value, task, line_number);
    return (0);
}

void validate_exitcodes(char *value, struct task_t *task, unsigned int line_number)
{
    char *s;
    int j = 0;
    int k;
    int i = strlen(value) - 1;
    int count_exitcodes = 0;
    if (value[0] != '{' && value[i] != '}')
    {
        printf("Error linea %d: Los exitcodes deben estar entre {}.\n", line_number);
        return ;
    }
    value[i] = '\0';
    i = 1;
    while (value[i])
    {   
        if (value[i] == ',' && value[i + 1] == '\0')
        {
            printf("Error linea %d: Caracter |%c| no valido. Despues de un ',' debe ir un valor.\n", line_number, value[i]);
            return ;
        }    
        if ((!isdigit(value[i]) && value[i] != '+' && value[i] != '-' && value[i] != ',') || (value[i] == ',' && value[i + 1] && value[i + 1] == ','))
        {
            printf("Error linea %d: Caracter |%c| no valido. Los exitcodes no siguen esta sintaxis {1,127,2}.\n", line_number, value[i]);
            return ;
        }
        if (value[i] == ',' && value[i + 1] && (isdigit(value[i + 1]) || value[i + 1] == '-' || value[i + 1] == '+'))
            j++;
        i++;
    }
    task->parser.exitcodes = (int *)malloc((j + 2) * sizeof(int));
    j = 0;
    i = 1;
    while (value[i])
    {
        k = i;
        while (value[k] && value[k] != ',')
            k++;
        s = ft_substr(value, i, k - i);
        count_exitcodes++;
        char *endptr;
        long num = strtol(s, &endptr, 10);
        if (*endptr != '\0' || num < INT_MIN || num > INT_MAX) 
        {
            printf("Error línea %d: |%s| no es un exitcode válido.\n", line_number, s);
            free(s);
            if (task->parser.exitcodes)
                task->parser.exitcodes = NULL;
            return;
        }
        free(s);
        task->parser.exitcodes[j + 1] = (int)num;
        j++;
        i = k + 1;
    }
    task->parser.exitcodes[0] = count_exitcodes;
}


/*UTILS*/

char	*ft_substr(char *s, unsigned int start, size_t len)
{
	size_t	i;
	char	*res;

	i = 0;
	if (!s)
		return (0);
	if (start > strlen(s))
	{
		res = malloc(1);
		res[0] = '\0';
		return (res);
	}
	if (strlen(s) - start < len)
		len = strlen(s) - start;
	res = malloc(sizeof(char) * (len + 1));
	while (start < strlen(s) && i < len && s[start])
		res[i++] = s[start++];
	res[i] = '\0';
	return (res);
}

static	void	ft_free_split(char **res)
{
	int	i;

	i = -1;
	while (res[++i])
		free(res[i]);
	free(res);
}

static	size_t	count_words(char *s)
{
	size_t	i;
	size_t	words;

	i = 0;
	words = 0;
	while (s[i])
	{
		if ((isspace((unsigned char)s[i + 1]) || s[i + 1] == '\0') && !isspace((unsigned char)s[i]))
			words++;
		i++;
	}
	return (words);
}

static	char	**write_result(char *s, char	**res)
{
	size_t	start;
	size_t	i;
	size_t	word;

	start = 0;
	i = 0;
	word = 0;
	while (s[i])
	{
		if ((isspace((unsigned char)s[i + 1]) || s[i + 1] == '\0') && !isspace((unsigned char)s[i]))
		{
			res[word] = ft_substr(s, start, i - start + 1);
			if (!res[word])
			{
				ft_free_split(res);
				return (0);
			}
			word++;
		}
		if (isspace((unsigned char)s[i]) && (!isspace((unsigned char)s[i + 1]) || s[i + 1] != '\0'))
			start = i + 1;
		i++;
	}
	res[word] = 0;
	return (res);
}

char	**ft_split(char *s)
{
	size_t	i;
	char	**res;

	i = 0;
	if (!s)
		return (0);
	res = malloc(sizeof(char *) * (count_words(s) + 1));
	res = write_result(s, res);
	return (res);
}

char	*ft_strtrim(char *s1, char *set)
{
	unsigned int	len;

	if (!s1 || !set)
		return (0);
	while (*s1 && strchr(set, *s1))
		s1++;
	len = strlen(s1);
	while (strrchr(set, s1[len]) && len)
		len--;
	return (ft_substr(s1, 0, len + 1));
}

void create_config_file(char *file_name, struct task_t *tasks)
{
    const char *AR_modes_strings[] = {"ALWAYS", "UNEXPECTED", "SUCCESS", "FAILURE", "NEVER"};
    const char *Signals_strings[] = {"", "HUP", "INT", "QUIT", "KILL", "", "ABRT", "", "FPE", "KILL", "USR1", "SEGV", "USR2", "PIPE", "ALRM", "TERM", "", "CHLD", "CONT", "STOP", "TSTP", "TTIN", "TTOU"};

    FILE *file = fopen(file_name, "w");
    if (file == NULL)
    {
        perror("Error al abrir el config file\n");
        return ;
    }
    task_t *current = tasks;
    int i = 0;
    int len = 0;
    while (current)
    {
        fprintf(file, "[%s]\n", current->parser.name);
        fprintf(file, " cmd: \"%s\"\n", current->parser.cmd);
        fprintf(file, " numprocs: %d\n", current->parser.procs);
        fprintf(file, " umask: %s\n", current->parser.umask);
        fprintf(file, " autostart: %s\n", current->parser.autostart ? "true" : "false");
        fprintf(file, " autorestart: %s\n", AR_modes_strings[current->parser.ar]);
        i = 0;
        if (current->parser.exitcodes)
        {
            len = current->parser.exitcodes[0];
            fprintf(file, " exitcodes: {");
            while (current->parser.exitcodes && ++i < len)
                fprintf(file, "%d,", current->parser.exitcodes[i]);
            fprintf(file, "%d}\n", current->parser.exitcodes[i]);
        }
        fprintf(file, " startretries: %d\n", current->parser.startretries);\
        fprintf(file, " starttime: %d\n", current->parser.starttime);
        fprintf(file, " stopsignal: %s\n", Signals_strings[current->parser.stopsignal]);
        fprintf(file, " stoptime: %d\n", current->parser.stoptime);
        fprintf(file, " stdout: %s\n", current->parser.stdout);
        fprintf(file, " stderr: %s\n", current->parser.stderr);
        i = 0;
        if (current->parser.env)
        {
            fprintf(file, " env:\n");
            while (current->parser.env && i + 1 < env_count && current->parser.env[++i])
                fprintf(file, " -%s\n", current->parser.env[i]);
        }
        current = FT_LIST_GET_NEXT(&tasks, current);
    }
}