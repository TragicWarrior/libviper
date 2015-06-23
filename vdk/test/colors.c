#include <stdio.h>
#include <unistd.h>

#include <curses.h>

#include <vdk/vdk_object.h>
#include <vdk/vdk_context.h>
#include <vdk/vdk_screen.h>
#include <vdk/vdk_surface.h>
#include <vdk/vdk_widget.h>
#include <vdk/vdk_label.h>

int main(int argc,char **argv)
{
    vdk_screen_t    *myscreen;
    vdk_widget_t    *mywidget;
    WINDOW          *mywin;

    short           colors;
    int             width = 20;
    int             height = 12;
    int             x = 5;
    int             y = 5;

    myscreen = vdk_screen_create(NULL,stdin,stdout);
    vdk_screen_init_color(myscreen,TRUE);

    mywidget = vdk_widget_create(width,height);
    vdk_widget_set_context(mywidget,VDK_CONTEXT(myscreen));

    mywin = *(WINDOW**)mywidget;

    colors = vdk_screen_get_color_pair(myscreen,COLOR_BLACK,COLOR_MAGENTA);
    vdk_screen_set_colors(myscreen,colors);
    vdk_screen_clear(myscreen);
    // vdk_screen_fill(myscreen,ACS_HLINE);

    colors = vdk_screen_get_color_pair(myscreen,COLOR_CYAN,COLOR_BLUE);
    vdk_widget_set_colors(mywidget,colors);
    vdk_widget_clear(mywidget);
    vdk_widget_fill(mywidget,ACS_CKBOARD);

    colors = vdk_screen_get_color_pair(myscreen,COLOR_WHITE,COLOR_GREEN);

    wattron(mywin,COLOR_PAIR(colors) | A_BOLD);
    wprintw(mywin,"%dw %dh",
        vdk_screen_get_width(myscreen),
        vdk_screen_get_height(myscreen));

    do
    {
        vdk_screen_clear(myscreen);

        vdk_surface_move(VDK_SURFACE(mywidget),x,y);
        vdk_surface_blit(VDK_SURFACE(mywidget));

        vdk_screen_draw(myscreen);
        x++;
        y++;

        sleep(1);
    }
    while(x < 10);

    vdk_screen_pause(myscreen);
    vdk_screen_destroy(myscreen);

    return 0;
}
