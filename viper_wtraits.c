#include "viper.h"
#include "private.h"

void
viper_window_set_title(vwnd_t *vwnd, const char *title)
{
    if(vwnd == NULL) return;

    vwnd->title = title;
}

const char*
viper_window_get_title(vwnd_t *vwnd)
{
    if(vwnd == NULL) return NULL;

    return vwnd->title;
}

void
viper_window_set_class(vwnd_t *vwnd, void *classid)
{
    if(vwnd == NULL) return;

    vwnd->classid = classid;

    return;
}
