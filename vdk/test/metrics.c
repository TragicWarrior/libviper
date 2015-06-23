#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>

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

    int             width = 20;
    int             height = 20;

    myscreen = vdk_screen_create(NULL,stdin,stdout);

    mywidget = vdk_widget_create(width,height);
    vdk_widget_set_context(mywidget,VDK_CONTEXT(myscreen));

    wprintw(*((WINDOW**)mywidget),"%dw %dh\n line 2",
        vdk_screen_get_width(myscreen),
        vdk_screen_get_height(myscreen));

    vdk_surface_move(VDK_SURFACE(mywidget),20,10);
    vdk_surface_blit(VDK_SURFACE(mywidget));


    vdk_screen_draw(myscreen);
    sleep(5);


    vdk_screen_pause(myscreen);
    vdk_screen_destroy(myscreen);

    return 0;
}
