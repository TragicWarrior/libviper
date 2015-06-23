#include <poll.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

#include "viper.h"
#include "viper_private.h"
#include "viper_kmio.h"

#if !defined(_NO_GPM) && defined(__linux)

#define  X_GPM(a,b,c,d)    a,
guint x_gpm_mode[]={
#include "viper_gpm.def"
};
#undef   X_GPM

#define  X_GPM(a,b,c,d)    b,
mmask_t x_ncurses_state[]={
#include "viper_gpm.def"
};
#undef   X_GPM

#define  X_GPM(a,b,c,d)    c,
short x_gpm_button[]={
#include "viper_gpm.def"
};
#undef   X_GPM

#define  X_GPM(a,b,c,d)    d,
unsigned short x_gpm_event[]={
#include "viper_gpm.def"
};
#undef   X_GPM

#endif

static void viper_kmio_show_mouse(MEVENT *mouse_event);

gint32 viper_kmio_fetch(MEVENT *mouse_event)
{
   gint32            keystroke=-1;
   gint32            key_code=0;
   guint             shift_op=4;

#if !defined(_NO_GPM) && defined(__linux)
   viper_kmio_gpm(mouse_event,0);
#endif

   key_code=getch();

   if(key_code!=-1)
   {
      if(key_code!=27)
      {
         if(key_code==KEY_MOUSE) getmouse(mouse_event);
         return key_code;
      }

      keystroke=27;
      do
      {
         shift_op=shift_op<<1;
         key_code=getch();
         if(key_code==-1) break;
         keystroke |= (key_code<<shift_op);
      }
      while(shift_op<24);
   }
   return keystroke;
}


void viper_kmio_dispatch(gint32 keystroke,MEVENT *mouse_event)
{
   extern VIPER            *viper;
   static WINDOW           *event_window=NULL;
   static MEVENT           previous_mouse_event;
   static VIPER_WKEY_FUNC  func;
   static gint             event_mode=0;
   VIPER_KMIO_HOOK         kmio_dispatch_hook;
   gint                    beg_x,beg_y;
   gint                    max_x,max_y;
   MEVENT                  *new_mouse=NULL;     /* strictly for      */
   MEVENT                  *old_mouse=NULL;     /* for readability   */
#if !defined(_NO_GPM) && defined(__linux)
	extern int        		gpm_fd;
#endif

   // run the dispatch pre-processing hook
   if(viper->kmio_dispatch_hook[KMIO_HOOK_ENTER] != NULL)
   {
      kmio_dispatch_hook=viper->kmio_dispatch_hook[KMIO_HOOK_ENTER];
      keystroke=kmio_dispatch_hook(keystroke);
   }

   if(keystroke == -1) return;

   if(keystroke==KEY_RESIZE)
   {
      viper_event_run(VIPER_EVENT_BROADCAST,"term-resized");
      /* todo event handle for screen window instead  */
      viper_screen_redraw(REDRAW_ALL | REDRAW_BACKGROUND);
   }

   if(keystroke==KEY_MOUSE && mouse_event!=NULL)
   {
      new_mouse=mouse_event;
      old_mouse=&previous_mouse_event;

      if((new_mouse->bstate & REPORT_MOUSE_POSITION)
          && event_mode==EVENTMODE_MOVE)
      {
         viper_mvwin_rel(event_window,new_mouse->x-old_mouse->x,
            new_mouse->y-old_mouse->y);
         memcpy(old_mouse,new_mouse,sizeof(MEVENT));
      }

      if((new_mouse->bstate & REPORT_MOUSE_POSITION)
         && event_mode==EVENTMODE_RESIZE)
      {
         viper_wresize_rel(event_window,new_mouse->x-old_mouse->x,
            new_mouse->y-old_mouse->y);
         memcpy(old_mouse,new_mouse,sizeof(MEVENT));
      }

      /* check for a button press and a window hit */
      if((new_mouse->bstate & BUTTON1_PRESSED) && event_mode==EVENTMODE_IDLE)
      {
         event_window=viper_deck_hit_test(new_mouse->x,new_mouse->y);
         if(event_window!=NULL)
         {
            viper_window_set_top(event_window);
            memcpy(old_mouse,new_mouse,sizeof(MEVENT));
            getbegyx(WINDOW_FRAME(event_window),beg_y,beg_x);
            getmaxyx(WINDOW_FRAME(event_window),max_y,max_x);
            if(new_mouse->x==(beg_x+max_x-1) &&
               new_mouse->y==(beg_y+max_y)-1) event_mode=EVENTMODE_RESIZE;
            else event_mode=EVENTMODE_MOVE;
         }
         else event_mode=EVENTMODE_IDLE;
      }

     if(new_mouse->bstate & BUTTON1_RELEASED)
      {
         if(!(new_mouse->bstate & REPORT_MOUSE_POSITION))
         {
            if(event_mode==EVENTMODE_MOVE) viper_mvwin_rel(event_window,
               new_mouse->x-old_mouse->x,new_mouse->y-old_mouse->y);

            /* resize window  */
            if(event_mode==EVENTMODE_RESIZE)
            {
               viper_wresize_rel(event_window,new_mouse->x-old_mouse->x,
                  new_mouse->y-old_mouse->y);
               viper_screen_redraw(REDRAW_ALL);
            }
         }

         event_window=NULL;
         event_mode=EVENTMODE_IDLE;
      }

      if(new_mouse->bstate & BUTTON1_CLICKED)
      {
         event_window=viper_deck_hit_test(new_mouse->x,new_mouse->y);
         if(event_window!=NULL)
         {
            viper_window_set_top(event_window);
            viper_window_redraw(event_window);

            getbegyx(WINDOW_FRAME(event_window),beg_y,beg_x);
            getmaxyx(WINDOW_FRAME(event_window),max_y,max_x);
            if(new_mouse->x==(beg_x+max_x-2) && new_mouse->y==beg_y)
            {
               viper_window_close(event_window);
               keystroke=-1;
            }
            if(new_mouse->x==(beg_x+max_x-4) && new_mouse->y==beg_y)
            {
               viper_window_hide(event_window);
               viper_deck_cycle(VECTOR_BOTTOM_TO_TOP);
               keystroke=-1;
            }
         }
         event_window=NULL;
         event_mode=EVENTMODE_IDLE;
      }

      if(new_mouse->bstate & BUTTON1_DOUBLE_CLICKED)
      {
         event_window=viper_deck_hit_test(new_mouse->x,new_mouse->y);
         if(event_window!=NULL)
         {
            viper_window_set_top(event_window);
            viper_window_redraw(event_window);
         }

         event_window=NULL;
         event_mode=EVENTMODE_IDLE;
      }
   }

   // run the post-processing dispatch hook
   if(viper->kmio_dispatch_hook[KMIO_HOOK_LEAVE] != NULL)
   {
      kmio_dispatch_hook=viper->kmio_dispatch_hook[KMIO_HOOK_LEAVE];
      keystroke=kmio_dispatch_hook(keystroke);
   }

   // pass keystroke on to toplevel window
   if(keystroke != KEY_RESIZE && keystroke != -1)
   {
      func=viper_window_get_key_func(TOPMOST_WINDOW);
      if(func!=NULL) func(keystroke,(gpointer)TOPMOST_WINDOW);
   }

#if !defined(_NO_GPM) && defined(__linux)
	if(gpm_fd>0)
	{
   	viper_kmio_show_mouse(new_mouse);
   	viper_screen_redraw(REDRAW_ALL);
	}
#endif



}

void viper_kmio_dispatch_set_hook(gint sequence,VIPER_KMIO_HOOK hook)
{
   extern VIPER   *viper;

   if(hook == NULL) return;
   if(sequence < 0) return;

   viper->kmio_dispatch_hook[sequence]=hook;

   return; 
}


static void viper_kmio_show_mouse(MEVENT *mouse_event)
{
   extern VIPER      *viper;
   extern WINDOW     *SCREEN_WINDOW;
   WINDOW            *screen_window;
   static chtype     color;
   gshort            fg,bg;

   screen_window=SCREEN_WINDOW;

   if(viper->console_mouse==NULL)
   {
      viper->console_mouse=newwin(1,1,0,0);
      color=mvwinch(screen_window,0,0);
      pair_content(PAIR_NUMBER(color & A_COLOR),&fg,&bg);
      if(bg==COLOR_RED || bg==COLOR_YELLOW || bg==COLOR_MAGENTA)
         color=VIPER_COLORS(COLOR_CYAN,COLOR_CYAN);
      if(bg==COLOR_CYAN || bg==COLOR_BLUE)
         color=VIPER_COLORS(COLOR_YELLOW,COLOR_YELLOW);
   }

   if(mouse_event!=NULL)
   {
      color=mvwinch(screen_window,mouse_event->y,mouse_event->x);
      pair_content(PAIR_NUMBER(color & A_COLOR),&fg,&bg);
      if(bg==COLOR_RED || bg==COLOR_YELLOW || bg==COLOR_MAGENTA) 
         color=VIPER_COLORS(COLOR_CYAN,COLOR_CYAN);
      else
         color=VIPER_COLORS(COLOR_YELLOW,COLOR_YELLOW);
      mvwin(viper->console_mouse,mouse_event->y,mouse_event->x);
   }

   mvwaddch(viper->console_mouse,0,0,' ' | color);

   return;
}

#if !defined(_NO_GPM) && defined(__linux)
gint viper_kmio_gpm(MEVENT *mouse_event,guint cmd)
{
	extern guint32		viper_global_flags;
   extern int        gpm_tried;
   extern int        gpm_fd;
   struct pollfd     mio_poll;
   struct timespec   sleep_time={.tv_sec=0,.tv_nsec=5000};
   static int        mio_fd=-1;
   Gpm_Connect       gpm_connect;
   Gpm_Event         g_event;
   gint              array_sz;
   gint              i;
	int					fflags;

   if(cmd == CMD_GPM_CLOSE)
   {
      if(mio_fd > 0) Gpm_Close();
      mio_fd=-1;
      return 0;
   }

   if(mouse_event==NULL) return -1;

   /* could not connect to the GPM server.   */
   if(gpm_fd==-2 || (gpm_fd==-1 && gpm_tried==TRUE)) return -1;

   memset(&g_event,0,sizeof(g_event));

   if(mio_fd==-1)
   {
      memset(&gpm_connect,0,sizeof(gpm_connect));
      gpm_connect.defaultMask=0; /* do not propgate any GPM events            */
      gpm_connect.eventMask=GPM_MOVE | GPM_UP | GPM_DOWN | GPM_DRAG;
      gpm_connect.maxMod=~0;     /* allow all modifiers ie. CTRL, SHFT, ALT   */
      mio_fd=Gpm_Open(&gpm_connect,0);
		if(mio_fd > 0 && (viper_global_flags & VIPER_GPM_SIGIO))
		{
			fcntl(mio_fd,F_SETOWN,getpid());
   		fflags=fcntl(mio_fd,F_GETFL);
   		fcntl(mio_fd,F_SETFL,fflags | FASYNC);
		}
   }

   if(mio_fd==-1) return -1;

   memset(&mio_poll,0,sizeof(mio_poll));
   mio_poll.events=POLLIN;
   mio_poll.fd=mio_fd;

   if(poll(&mio_poll,1,1)<1) return -1;
   if(Gpm_GetEvent(&g_event)<1) return -1;

   memset(mouse_event,0,sizeof(MEVENT));
   mouse_event->bstate=g_event.modifiers;
   mouse_event->x=g_event.x-1;
   mouse_event->y=g_event.y-1;

   array_sz=sizeof(x_ncurses_state)/sizeof(x_ncurses_state[0]);

   if(!(GPM_CLICK_STRICT(g_event.type)))
   {
      for(i=0;i<array_sz;i++)
      {
         /* sift by mode... ignore COOKED table entries        */
         if(x_gpm_mode[i]==X_GPM_COOKED) continue;

         /* sift raw event... GPM_UP, GPM_DOWN, etc.           */
         if(!(g_event.type & x_gpm_event[i])) continue;

         /* sift which physical button... GPM_B_LEFT, etc.     */
         if(g_event.buttons != x_gpm_button[i]) continue;

         mouse_event->bstate |= x_ncurses_state[i];
         break;
      }
   }

   if(GPM_CLICK_STRICT(g_event.type))
   {
      for(i=0;i<array_sz;i++)
      {
         /* sift by mode... ignore RAW table entries           */
         if(x_gpm_mode[i]==X_GPM_RAW) continue;

         /* sift cooked event... GPM_SINGLE, GPM_DOUBLE, etc.  */
         if(!(g_event.type & x_gpm_event[i])) continue;

         /* sift which physical button... GPM_B_LEFT, etc.     */
         if(g_event.buttons != x_gpm_button[i]) continue;

         mouse_event->bstate=x_ncurses_state[i];
         break;
      }
   }

   if((g_event.type & GPM_DRAG) || (g_event.type & GPM_MOVE))
      mouse_event->bstate = REPORT_MOUSE_POSITION;

   if(mouse_event->bstate!=0)
   {
      while(ungetmouse(mouse_event)==ERR)
      {
         nanosleep(&sleep_time,NULL);
      }
   }

   return 0;
}
#endif
