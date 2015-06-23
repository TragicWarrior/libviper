#ifndef _VIPER_COLOR_H_
#define _VIPER_COLOR_H_

#include <glib.h>

void	viper_color_init(void);

struct color_mtx {
		gint	fg;
		gint	bg;
};

#endif
