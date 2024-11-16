#include <taskmaster.h>

enum
{
    NAME,
    CMD,
    DIR,
    ENV,
    AUTOSTART,
    AUTORESTART,
    STARTRETRIES,
    STARTTIME,
    STOPTIME,
    EXITCODES,
    STOPSIGNAL,
    STOPTIMEOUT,
    STDOUT,
    STDERR,
    STDIN,
    UMASK,
} TYPE;

int parse_int(char* str);
char* parse_string(char* str);
char** parse_env(char* str);
autorestart parse_autorestart(char* str);

typedef struct
{
    char*   field_name;
    TYPE    field_enum;
    void*   (*parse_me)(char*);
} t_parser;

static t_parser m_parser[] = {
    {"name", NAME, parse_string},
    {"cmd", CMD, parse_string},
    {"dir", DIR, parse_string},
    {"env", ENV, parse_env},
    {"autostart", AUTOSTART, parse_int},
    {"autorestart", AUTORESTART, parse_autorestart},
    {"startretries", STARTRETRIES, parse_int},
    {"starttime", STARTTIME, parse_int},
    {"stoptime", STOPTIME, parse_int},
    {"exitcodes", EXITCODES, parse_int},
    {"stopsignal", STOPSIGNAL, parse_int},
    {"stoptimeout", STOPTIMEOUT, parse_int},
    {"stdout", STDOUT, parse_int},
    {"stderr", STDERR, parse_int},
    {"stdin", STDIN, parse_int},
    {"umask", UMASK, parse_int},
};

int parse_int(char* str)
{
    return (atoi(str));
}

char* parse_string(char* str)
{
    return (str);
}

char** parse_env(char* str)
{
    return NULL;
}

autorestart parse_autorestart(char* str)
{
    if (strcmp(str, "always") == 0)
        return (ALWAYS);
    else if (strcmp(str, "unexpected") == 0)
        return (UNEXPECTED);
    else if (strcmp(str, "success") == 0)
        return (SUCCESS);
    else if (strcmp(str, "failure") == 0)
        return (FAILURE);
    else if (strcmp(str, "never") == 0)
        return (NEVER);
    return (UNKNOWN);
}
