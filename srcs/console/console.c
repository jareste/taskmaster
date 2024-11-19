#include <taskmaster.h>

int interactive_console(void* param)
{
    task_t* tasks = NULL;

    tasks = get_active_tasks();

    (void)param;
    return (0);
}
