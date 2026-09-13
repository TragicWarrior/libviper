#include <stdio.h>
#include "vdk.h"

int
main(void)
{
    vk_screen_t     *screen;
    vk_graph_t      *graph1, *graph2, *graph3;
    vk_label_t      *label1, *label2, *label3;
    int32_t         key;

    const double data[20] =
    {
        5, 12, 20, 30, 42, 55, 68, 78, 88, 95,
        98, 92, 83, 70, 58, 45, 33, 22, 13, 6
    };

    screen = vk_screen_create();
    if(screen == NULL)
    {
        fprintf(stderr, "vk_screen_create failed\n");
        return 1;
    }

    vdk_color_init();

    graph1 = vk_graph_create(74, 6);
    vk_graph_set_y_range(graph1, 0.0, 100.0);
    vk_graph_set_data(graph1, data, 20);
    vk_graph_set_bar_style(graph1, VK_GRAPH_BAR_BLOCK);
    vk_graph_set_colors(graph1, COLOR_GREEN, COLOR_BLACK);
    vk_graph_update(graph1);

    graph2 = vk_graph_create(74, 6);
    vk_graph_set_y_range(graph2, 0.0, 100.0);
    vk_graph_set_data(graph2, data, 20);
    vk_graph_set_bar_style(graph2, VK_GRAPH_BAR_BRAILLE);
    vk_graph_set_colors(graph2, COLOR_CYAN, COLOR_BLACK);
    vk_graph_update(graph2);

    graph3 = vk_graph_create(74, 6);
    vk_graph_set_y_range(graph3, 0.0, 100.0);
    vk_graph_set_data(graph3, data, 20);
    vk_graph_set_bar_style(graph3, VK_GRAPH_BAR_ASCII);
    vk_graph_set_colors(graph3, COLOR_YELLOW, COLOR_BLACK);
    vk_graph_update(graph3);

    label1 = vk_label_create(34);
    vk_label_set_text(label1, "block  (VK_GRAPH_BAR_BLOCK)");
    vk_widget_set_colors(VK_WIDGET(label1), COLOR_WHITE, COLOR_BLACK);
    vk_label_update(label1);

    label2 = vk_label_create(34);
    vk_label_set_text(label2, "braille  (VK_GRAPH_BAR_BRAILLE)");
    vk_widget_set_colors(VK_WIDGET(label2), COLOR_WHITE, COLOR_BLACK);
    vk_label_update(label2);

    label3 = vk_label_create(34);
    vk_label_set_text(label3, "ascii  (VK_GRAPH_BAR_ASCII)");
    vk_widget_set_colors(VK_WIDGET(label3), COLOR_WHITE, COLOR_BLACK);
    vk_label_update(label3);

    vk_screen_attach_widget(screen, 0, VK_WIDGET(label1));
    vk_widget_move(VK_WIDGET(label1), 1, 1);

    vk_screen_attach_widget(screen, 0, VK_WIDGET(graph1));
    vk_widget_move(VK_WIDGET(graph1), 1, 2);

    vk_screen_attach_widget(screen, 0, VK_WIDGET(label2));
    vk_widget_move(VK_WIDGET(label2), 1, 9);

    vk_screen_attach_widget(screen, 0, VK_WIDGET(graph2));
    vk_widget_move(VK_WIDGET(graph2), 1, 10);

    vk_screen_attach_widget(screen, 0, VK_WIDGET(label3));
    vk_widget_move(VK_WIDGET(label3), 1, 17);

    vk_screen_attach_widget(screen, 0, VK_WIDGET(graph3));
    vk_widget_move(VK_WIDGET(graph3), 1, 18);

    vk_screen_refresh(screen);

    while((key = wgetch(stdscr)) != 'q')
        ;

    vk_screen_destroy(screen);

    return 0;
}
