#include "viper.h"
#include "viper_private.h"

void viper_window_set_title(WINDOW *window,const gchar *title)
{
	VIPER_WND	*viper_wnd;

	if(window==NULL) return;

	viper_wnd=viper_get_viper_wnd(window);
	if(viper_wnd==NULL) return;

	viper_wnd->title=title;
}

const gchar* viper_window_get_title(WINDOW *window)
{
	VIPER_WND	*viper_wnd;

	if(window==NULL) return NULL;

	viper_wnd=viper_get_viper_wnd(window);
	if(viper_wnd==NULL) return NULL;

	return viper_wnd->title;
}

void viper_window_set_class(WINDOW *window,gpointer classid)
{
   VIPER_WND	*viper_wnd;

	if(window==NULL) return;

   viper_wnd=viper_get_viper_wnd(window);
   if(viper_wnd==NULL) return;

   viper_wnd->classid=classid;

   return;
}

