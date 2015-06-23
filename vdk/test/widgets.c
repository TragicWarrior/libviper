#include <stdio.h>
#include <unistd.h>

#include <curses.h>

#undef addch

#include <vdk/vdk_object.h>
#include <vdk/vdk_context.h>
#include <vdk/vdk_screen.h>
#include <vdk/vdk_surface.h>
#include <vdk/vdk_widget.h>
#include <vdk/vdk_label.h>

int main(int argc,char **argv)
{
    vdk_screen_t    *myscreen;
    vdk_label_t     *mylabel;

    short           colors;
    int             x = 5;
    int             y = 5;

    myscreen = vdk_screen_create(NULL,stdin,stdout);
    vdk_screen_init_color(myscreen,TRUE);

    mylabel = vdk_label_create("test label");
    vdk_widget_set_context(VDK_WIDGET(mylabel),VDK_CONTEXT(myscreen));

    colors = vdk_screen_get_color_pair(myscreen,COLOR_BLACK,COLOR_MAGENTA);
    vdk_screen_set_colors(myscreen,colors);
    vdk_screen_clear(myscreen);

    colors = vdk_screen_get_color_pair(myscreen,COLOR_BLACK,COLOR_BLUE);
    vdk_widget_set_colors(VDK_WIDGET(mylabel),colors);
    vdk_label_set_text(mylabel,"this is a test label",TRUE);

    do
    {
        vdk_screen_clear(myscreen);

        vdk_surface_move(VDK_SURFACE(mylabel),x,y);
        vdk_surface_blit(VDK_SURFACE(mylabel));

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
