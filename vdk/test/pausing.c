#include <stdio.h>
#include <unistd.h>

#include <curses.h>

#include <vdk/vdk_object.h>
#include <vdk/vdk_context.h>
#include <vdk/vdk_screen.h>
#include <vdk/vdk_surface.h>
#include <vdk/vdk_widget.h>


int main(int argc,char **argv)
{
    vdk_screen_t    *myscreen;
    vdk_widget_t    *mywidget;

    int             width = 10;
    int             height = 10;

    myscreen = vdk_screen_create(NULL,stdin,stdout);

    mywidget = vdk_widget_create(width,height);
    vdk_widget_set_context(mywidget,VDK_CONTEXT(myscreen));

    wprintw(*((WINDOW**)mywidget),"this is a test");

    vdk_surface_blit(VDK_SURFACE(mywidget));

    vdk_screen_draw(myscreen);
    sleep(5);
    vdk_screen_pause(myscreen);
    sleep(5);
    vdk_screen_resume(myscreen);
    sleep(5);

    vdk_screen_pause(myscreen);
    vdk_screen_destroy(myscreen);

    return 0;
}
