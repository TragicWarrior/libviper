#include <stdio.h>
#include "vdk.h"

int
main(void)
{
    vk_screen_t     *screen;
    vk_histogram_t  *hist;
    vk_label_t      *lab;
    int32_t         key;

    double samples[32] =
    {
         5, 15, 18, 25, 28, 31, 34, 37, 40, 42, 44, 45, 46, 47, 48, 49,
        50, 51, 52, 53, 54, 55, 56, 58, 60, 63, 66, 69, 72, 75, 82, 95
    };

    const char *bin_labels[10] =
    {
        "0-10", "10-20", "20-30", "30-40", "40-50",
        "50-60", "60-70", "70-80", "80-90", "90-100"
    };

    screen = vk_screen_create();
    if(screen == NULL)
    {
        fprintf(stderr, "vk_screen_create failed\n");
        return 1;
    }

    vdk_color_init();

    hist = vk_histogram_create(74, 12);
    vk_histogram_set_range(hist, 0.0, 100.0);
    vk_histogram_set_bins(hist, 10);
    vk_histogram_set_samples(hist, samples, 32);
    vk_graph_set_bar_style(VK_GRAPH(hist), VK_GRAPH_BAR_BLOCK);
    vk_graph_set_colors(VK_GRAPH(hist), COLOR_MAGENTA, COLOR_BLACK);
    vk_graph_set_unit_label(VK_GRAPH(hist), "items");
    vk_graph_set_x_labels(VK_GRAPH(hist), bin_labels, 10);
    vk_histogram_update(hist);

    lab = vk_label_create(44);
    vk_label_set_text(lab, "histogram: 32 samples, 10 bins over [0,100]");
    vk_widget_set_colors(VK_WIDGET(lab), COLOR_WHITE, COLOR_BLACK);
    vk_label_update(lab);

    vk_screen_attach_widget(screen, 0, VK_WIDGET(lab));
    vk_widget_move(VK_WIDGET(lab), 1, 1);
    vk_screen_attach_widget(screen, 0, VK_WIDGET(hist));
    vk_widget_move(VK_WIDGET(hist), 1, 3);

    vk_screen_refresh(screen);

    while((key = wgetch(stdscr)) != 'q')
        ;

    vk_screen_destroy(screen);

    return 0;
}
