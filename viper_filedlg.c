/*************************************************************************
 * All portions of code are copyright by their respective author/s.
 * Copyright (C) 2007      Bryan Christ <bryan.christ@hp.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *----------------------------------------------------------------------*/

#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <time.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "viper.h"
#include "viper_filedlg.h"
#include "viper_events.h"

WINDOW*  viper_filedlg_create(WINDOW *parent,gchar *title,gfloat x,gfloat y,
   gfloat width,gfloat height,gchar *dir,gint32 flags)
{
   extern WINDOW  *SCREEN_WINDOW;
   WINDOW         *window;
   VIPER_FILEDLG  *viper_filedlg;
   FIELD          **fields;
   gchar          *controls[]={"[ Okay ]","[Cancel]",NULL};
   gint           max_x,max_y;

   if(parent==NULL) return NULL;
   if(title==NULL && (flags & FILEDLG_SAVE)) title=" Save to... ";
   if(title==NULL && ~(flags & FILEDLG_SAVE)) title=" Open... ";

   /* reject requests that will yeild less than 8 lines for display. */
   if(height>0 && height<1)
   {
      window_get_size_scaled(SCREEN_WINDOW,NULL,&max_y,0,height);
      height=max_y;
   }
   if(height<8) return NULL;
   
   window=viper_window_create(title,x,y,width,height,TRUE);
   /* todo:  not allowing window resizing is temporary.  it wouldn't take
      much to create the window-resize event handler required to fix
      make the window resize work.   */
   getmaxyx(window,max_y,max_x);

   viper_filedlg=(VIPER_FILEDLG*)g_malloc0(sizeof(VIPER_FILEDLG));
   viper_filedlg->parent=parent;
   viper_filedlg->file_win=derwin(window,max_y-6,max_x-2,1,1);
   viper_filedlg->path_win=derwin(window,1,max_x-2,max_y-4,1);
   viper_filedlg->ctrl_win=derwin(window,1,max_x-2,max_y-2,1);

   if(dir==NULL) dir=getcwd(NULL,0);
   viper_filedlg->fstats_list=filedlg_readdir(dir,flags);
   viper_filedlg->items=filedlg_format_items(viper_filedlg->fstats_list,
      flags,max_x-2);
   viper_filedlg->flags=flags;
   
   viper_filedlg->file_menu=viper_menu_create(viper_filedlg->items);
   set_menu_format(viper_filedlg->file_menu,max_y-6,1);
   set_menu_mark(viper_filedlg->file_menu,NULL);
   set_menu_win(viper_filedlg->file_menu,viper_filedlg->file_win);
	set_menu_fore(viper_filedlg->file_menu,
      VIPER_COLORS(COLOR_WHITE,COLOR_BLUE) | A_BOLD);
	set_menu_back(viper_filedlg->file_menu,
      VIPER_COLORS(COLOR_BLACK,COLOR_WHITE));
   /* menu_opts_off(viper_filedlg->file_menu,O_ONEVALUE); */

   viper_filedlg->ctrl_menu=viper_menu_create(controls);
   set_menu_format(viper_filedlg->ctrl_menu,1,2);
   set_menu_mark(viper_filedlg->ctrl_menu,NULL);
   set_menu_sub(viper_filedlg->ctrl_menu,viper_filedlg->ctrl_win);
   set_menu_fore(viper_filedlg->ctrl_menu,
      VIPER_COLORS(COLOR_WHITE,COLOR_BLUE) | A_BOLD);
	set_menu_back(viper_filedlg->ctrl_menu,
      VIPER_COLORS(COLOR_BLACK,COLOR_WHITE));

   fields=(FIELD**)g_malloc0(sizeof(FIELD*)*2);
   fields[0]=new_field(1,max_x-2,0,0,0,0);
   set_field_type(fields[0],TYPE_ALNUM,1);
    set_max_field(fields[0],NAME_MAX);
   set_field_back(fields[0],VIPER_COLORS(COLOR_WHITE,COLOR_BLACK) | A_BOLD);
   viper_filedlg->path_form=new_form(fields);
   set_form_win(viper_filedlg->path_form,viper_filedlg->path_win);
 
   viper_window_set_userptr(window,(gpointer)viper_filedlg);
   viper_window_set_key_func(window,viper_kbd_default_FILEDLG_MENU);
   viper_event_set(window,"window-move",viper_filedlg_ON_MOVE,
      (gpointer)viper_filedlg);
   viper_event_set(window,"window-close",viper_filedlg_ON_CLOSE,
      (gpointer)viper_filedlg);

   post_menu(viper_filedlg->file_menu);
   post_menu(viper_filedlg->ctrl_menu);
   post_form(viper_filedlg->path_form);
   viper_window_redraw(window);

   return window;
}

gint viper_kbd_default_FILEDLG_MENU(gint32 keystroke,WINDOW *window)
{
   VIPER_FILEDLG  *viper_filedlg;
   VIPER_FSTATS   *viper_fstats;
	MEVENT		   mevent;
   GSList         *file_list;
	gchar			   **list;
   FIELD          **fields;
   gint           max_x,max_y;
   gint           mouse_traf=0;

   viper_thread_enter();
	viper_filedlg=(VIPER_FILEDLG*)viper_window_get_userptr(window);

	if(keystroke==KEY_MOUSE)
	{
		menu_driver(viper_filedlg->file_menu,keystroke);
		getmouse(&mevent);
      if(mevent.bstate & BUTTON1_DOUBLE_CLICKED) keystroke=KEY_CRLF;
      mouse_traf=1;
	}

   if(keystroke==KEY_TAB)
   {
      viper_window_set_key_func(window,viper_kbd_default_FILEDLG_PATH);
      viper_form_driver(viper_filedlg->path_form,REQ_END_FIELD,
         FORM_CURSOR_BLOCK | FORM_COLORIZE,
         VIPER_COLORS(COLOR_WHITE,COLOR_BLUE) | A_BOLD,
         VIPER_COLORS(COLOR_WHITE,COLOR_BLACK) | A_BOLD,-1); 
      viper_thread_leave();
      return 1;
   }

   if(keystroke==KEY_UP) menu_driver(viper_filedlg->file_menu,REQ_UP_ITEM);
  	if(keystroke==KEY_DOWN) menu_driver(viper_filedlg->file_menu,REQ_DOWN_ITEM);
   if(keystroke==KEY_NPAGE) menu_driver(viper_filedlg->file_menu,REQ_SCR_DPAGE);
   if(keystroke==KEY_PPAGE) menu_driver(viper_filedlg->file_menu,REQ_SCR_UPAGE);

   if(keystroke==' ') menu_driver(viper_filedlg->file_menu,REQ_TOGGLE_ITEM);   

   if(keystroke==KEY_UP || keystroke==KEY_DOWN || keystroke==KEY_NPAGE ||
      keystroke==KEY_PPAGE || mouse_traf==1)
   {
      fields=form_fields(viper_filedlg->path_form);
      viper_fstats=g_slist_nth_data(viper_filedlg->fstats_list,
         CURRENT_MENU_ITEM(viper_filedlg->file_menu));
      set_field_buffer(fields[0],0,viper_fstats->filepath);
      form_driver(viper_filedlg->path_form,REQ_VALIDATION);
   }

   if(keystroke==KEY_CRLF)
   {
      viper_fstats=(VIPER_FSTATS*)g_slist_nth_data(viper_filedlg->fstats_list,
         CURRENT_MENU_ITEM(viper_filedlg->file_menu));
      if(S_ISDIR(viper_fstats->mode))
      {
         file_list=filedlg_readdir(viper_fstats->filepath,viper_filedlg->flags);
         getmaxyx(viper_filedlg->file_win,max_y,max_x);
         list=filedlg_format_items(file_list,viper_filedlg->flags,max_x);
         viper_menu_items_change(viper_filedlg->file_menu,list);
         g_slist_foreach(viper_filedlg->fstats_list,g_slist_free_data,NULL);
         g_slist_free(viper_filedlg->fstats_list);
         viper_filedlg->fstats_list=file_list;
         post_menu(viper_filedlg->file_menu);
         g_strfreev(list);
      }
   }

   viper_window_redraw(window);
	viper_thread_leave();
	return 1;
}

gint viper_kbd_default_FILEDLG_PATH(gint32 keystroke,WINDOW *window)
{
   VIPER_FILEDLG  *viper_filedlg;
 
   viper_thread_enter();
	viper_filedlg=(VIPER_FILEDLG*)viper_window_get_userptr(window);
 
   if(keystroke==KEY_BACKSPACE)
   {
      form_driver(viper_filedlg->path_form,REQ_LEFT_CHAR);
      keystroke=REQ_DEL_CHAR;
   }

   if(keystroke==KEY_LEFT) keystroke=REQ_LEFT_CHAR;
   if(keystroke==KEY_RIGHT) keystroke=REQ_RIGHT_CHAR;
   if(keystroke==KEY_END) keystroke=REQ_END_FIELD;
   if(keystroke==KEY_BEG) keystroke=REQ_BEG_FIELD;
   if(keystroke==' ') keystroke=REQ_INS_CHAR;

   if(keystroke==KEY_TAB || keystroke==KEY_CRLF)
   {
      viper_window_set_key_func(window,viper_kbd_default_FILEDLG_MENU);
      viper_form_normalize(viper_filedlg->path_form,
         VIPER_COLORS(COLOR_WHITE,COLOR_BLACK) | A_BOLD,
         VIPER_COLORS(COLOR_WHITE,COLOR_BLACK) | A_BOLD);
      viper_thread_leave();
      return 1;
   }

   viper_form_driver(viper_filedlg->path_form,keystroke,
      FORM_CURSOR_BLOCK | FORM_COLORIZE,
      VIPER_COLORS(COLOR_WHITE,COLOR_BLUE) | A_BOLD,
      VIPER_COLORS(COLOR_WHITE,COLOR_BLACK) | A_BOLD,-1);  
   
   viper_window_redraw(window);
	viper_thread_leave();
	return 1;
}

gint viper_filedlg_ON_MOVE(WINDOW *window,gpointer arg)
{
   VIPER_FILEDLG  *viper_filedlg;

   viper_filedlg=(VIPER_FILEDLG*)arg;

	viper_thread_enter();
   subwin_move_realign(viper_filedlg->file_win);
   subwin_move_realign(viper_filedlg->ctrl_win);
	viper_thread_leave();
	return 0;
}

gint viper_filedlg_ON_CLOSE(WINDOW *window,gpointer arg)
{
   VIPER_FILEDLG  *viper_filedlg;
   VIPER_FSTATS   *viper_fstats;
   GSList         *fstats_list;
   FIELD          **fields;
   ITEM           **items;
   gchar          **results=NULL;
   gint           sz_buf=1;
   gint           idx;
   gboolean       has_event=FALSE;

   viper_filedlg=(VIPER_FILEDLG*)arg;
   fields=form_fields(viper_filedlg->path_form);

   /* check to see if target window has will catch the event   */
   if(viper_get_viper_event(window,"filedlg-results")!=NULL)
   {
      has_event=TRUE;

      if(!(viper_filedlg->flags & FILEDLG_MULTISELECT))
      {
         results=(gchar**)g_malloc0(2*sizeof(gchar*));
         results[0]=g_strndup(field_buffer(fields[0],0),NAME_MAX);
      }
      else
      {
         items=menu_items(viper_filedlg->file_menu);
         fstats_list=viper_filedlg->fstats_list;
         while(*items!=NULL)
         {
            if(item_value(*items))
            {
               sz_buf++;
               results=(gchar**)g_realloc((gpointer)results,
                  sz_buf*sizeof(gchar*));
               idx=item_index(*items);
               viper_fstats=(VIPER_FSTATS*)g_slist_nth_data(fstats_list,idx);
               results[sz_buf-1]=g_strndup(viper_fstats->filepath,NAME_MAX);
            }
            items++;
         }
         if(results!=NULL) results[sz_buf]=NULL;
      }
   }

   viper_thread_enter();
   
   if(has_event) viper_event_exec(viper_filedlg->parent,
      "filedlg-results",(gpointer)results);
   viper_menu_destroy(viper_filedlg->file_menu,TRUE);
   viper_menu_destroy(viper_filedlg->ctrl_menu,TRUE);
   viper_form_destroy(viper_filedlg->path_form,TRUE);
   viper_thread_leave();

   g_slist_foreach(viper_filedlg->fstats_list,g_slist_free_data,NULL);
   g_slist_free(viper_filedlg->fstats_list);
   g_strfreev(viper_filedlg->items);

   return VIPER_EVENT_WINDOW_DESIST;
}

GSList* filedlg_readdir(gchar *dir,gint32 flags)
{
   DIR            *directory;
   struct dirent  *entry;
   GSList         *fstats_list=NULL;
   GSList         *dir_list=NULL;
   GSList         *node;
   VIPER_FSTATS   *viper_fstats;
   struct stat64  fstats;
   gchar          filepath[NAME_MAX];
   gint           idx;
 
   directory=opendir(dir);
   if(directory==NULL) return NULL;

   /* create the ".." entry manually because some *nix systems do return them
      during a readdir()   */
   viper_fstats=(VIPER_FSTATS*)g_malloc0(sizeof(VIPER_FSTATS));
   sprintf(viper_fstats->filename,"..");
   strncpy(viper_fstats->filepath,dir,NAME_MAX);
   idx=strlen(viper_fstats->filepath)-1;
   while(idx>0)
   {
      viper_fstats->filepath[idx]='\0';
      idx--;
      if(viper_fstats->filepath[idx]=='/') break;
   }
   
   viper_fstats->mode=S_IFDIR;
   fstats_list=g_slist_prepend(fstats_list,(gpointer)viper_fstats);
  
   while((entry=readdir(directory))!=NULL)
   {
      if(strcmp(entry->d_name,".")==0) continue;
      if(strcmp(entry->d_name,"..")==0) continue;

      if(dir[strlen(dir)-1]=='/') sprintf(filepath,"%s%s",dir,entry->d_name);
      else sprintf(filepath,"%s/%s",dir,entry->d_name);
   
      /* filter list */
      if((entry->d_name[0]=='.') && !(flags & FILEDLG_SHOW_HIDDEN)) continue;
      stat64(filepath,&fstats);

      if(S_ISDIR(fstats.st_mode) && !(flags & FILEDLG_SHOW_DIRS)) continue;
      if(S_ISREG(fstats.st_mode) && !(flags & FILEDLG_SHOW_FILES)) continue;

      /* create a new file list item   */
      viper_fstats=(VIPER_FSTATS*)g_malloc0(sizeof(VIPER_FSTATS));
      strncpy(viper_fstats->filename,entry->d_name,NAME_MAX);
      strncpy(viper_fstats->filepath,filepath,NAME_MAX);
      viper_fstats->mode=fstats.st_mode;
      viper_fstats->size=fstats.st_size;
      viper_fstats->ctime=fstats.st_ctime;
      
      /* append the new item  */
      fstats_list=g_slist_prepend(fstats_list,(gpointer)viper_fstats);  
   }

   closedir(directory);

   /* do an simple alpha string sort on the listing   */
   fstats_list=g_slist_sort(fstats_list,g_slist_sort_func);
   
   /* if dirs dont need to be displayed first then we're done  */
   if(!(flags & FILEDLG_SORT_DIRS)) return fstats_list;
  
   /* handle directory sorting   */
   node=fstats_list;
   while(node!=NULL)
   {
      viper_fstats=(VIPER_FSTATS*)node->data;
      if(S_ISDIR(viper_fstats->mode))
      {
         dir_list=g_slist_prepend(dir_list,(gpointer)viper_fstats);
         fstats_list=g_slist_delete_link(fstats_list,node);
         node=fstats_list;
         continue;
      }
      node=node->next;
   }
   
   fstats_list=g_slist_reverse(fstats_list);
   dir_list=g_slist_concat(dir_list,fstats_list);

   return dir_list;
}
   
gchar** filedlg_format_items(GSList *fstats_list,gint32 flags,gint width)
{
   VIPER_FSTATS   *viper_fstats;
   gchar          **items;
   GSList         *node;
   guint          count;
   guint          idx=0;
   gint8          offset;
   gchar          buffer[NAME_MAX];
   gint           len;

   count=g_slist_length(fstats_list);
   items=(gchar**)g_malloc0((count+1)*sizeof(gchar*));

   node=fstats_list;
   while(node!=NULL)
   {
      viper_fstats=(VIPER_FSTATS*)node->data;
      items[idx]=(gchar*)g_malloc0(width+1);

      offset=width;
      if(flags & FILEDLG_SHOW_SIZE) offset-=FIELD_WIDTH_SIZE;
      if(flags & FILEDLG_SHOW_CTIME) offset-=FIELD_WIDTH_CTIME;
      if(flags & FILEDLG_SHOW_PRIV) offset-=FIELD_WIDTH_PRIV;

      /* put brackets around entries which are directories  */   
      if(S_ISDIR(viper_fstats->mode))
      {
         len=strlen(viper_fstats->filename);
         sprintf(items[idx],"[%-*s",offset-2,viper_fstats->filename);
         if(len<offset) items[idx][len+1]=']';
      }
      else sprintf(items[idx],"%-*s",offset,viper_fstats->filename);

      /* skip file size information for directories   */
      if(flags & FILEDLG_SHOW_SIZE)
      {
         if(S_ISDIR(viper_fstats->mode)) sprintf(buffer," ");
         else filedlg_field_size(viper_fstats->size,buffer);
            
         sprintf(&items[idx][offset-1],"|%*s",FIELD_WIDTH_SIZE-1,buffer);
         offset+=FIELD_WIDTH_SIZE;
      }

      if(flags & FILEDLG_SHOW_CTIME)
      { 
         if(strcmp(viper_fstats->filename,"..")==0) sprintf(buffer," ");
         else filedlg_field_ctime(viper_fstats->ctime,buffer);
         sprintf(&items[idx][offset-1],"|%*s",FIELD_WIDTH_CTIME-1,buffer);
         offset+=FIELD_WIDTH_CTIME;
      }

      if(flags & FILEDLG_SHOW_PRIV)
      { 
         filedlg_field_permissions(viper_fstats->mode,buffer);
         sprintf(&items[idx][offset-1],"|%*s",FIELD_WIDTH_PRIV-1,buffer);
         offset+=FIELD_WIDTH_PRIV;
      } 

      idx++;
      node=node->next;
   }      
   
   return items;
}   

      
void filedlg_field_size(off_t size,gchar *buffer)
{
   gchar    metric[]={'K','M','G','T','P'};
   gint     idx=0;
   gfloat   printable;

   if(size<100000)
   {
      sprintf(buffer,"%d",(guint32)size);
      return;
   }

   printable=(gfloat)(size/1000);
   while(printable>1000)
   {
      printable=printable/1000;
      idx++;
   }

   sprintf(buffer,"%2.1f%c",printable,metric[idx]);
   return;
}

void filedlg_field_ctime(time_t ctime,gchar *buffer)
{
   struct tm	*local_time;

   local_time=localtime((time_t*)&ctime);

   sprintf(buffer,"%02d-%02d-%04d %02d:%02d:%02d",
		local_time->tm_mon+1,local_time->tm_mday,local_time->tm_year+1900,
		local_time->tm_hour,local_time->tm_min,local_time->tm_sec);
}

void filedlg_field_permissions(mode_t mode,gchar *buffer)
{
   memset(buffer,'-',10);
   buffer[10]=0;

   if(S_ISDIR(mode)) buffer[0]='d';
   if(S_ISCHR(mode)) buffer[0]='c';
   if(S_ISFIFO(mode)) buffer[0]='f';
   if(S_ISBLK(mode)) buffer[0]='b';
   
   if(mode & S_IRUSR) buffer[1]='r';
   if(mode & S_IWUSR) buffer[2]='w';
   if(mode & S_IXUSR) buffer[3]='x';

   if(mode & S_IRGRP) buffer[4]='r';
   if(mode & S_IWGRP) buffer[5]='w';
   if(mode & S_IXGRP) buffer[6]='x';

   if(mode & S_IROTH) buffer[7]='r';
   if(mode & S_IWOTH) buffer[8]='w';
   if(mode & S_IXOTH) buffer[9]='x';

   return;
}

void g_slist_free_data(gpointer data,gpointer anything)
{
   g_free(data);
   return;
}

gint g_slist_sort_func(gconstpointer a,gconstpointer b)
{
   gchar    *string1;
   gchar    *string2;

   string1=((VIPER_FSTATS*)a)->filename;
   string2=((VIPER_FSTATS*)b)->filename;

   return -(strncmp(string1,string2,NAME_MAX));
}




