#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <dirent.h>
#include <sys/stat.h>
#include <limits.h>

#include "vk_object.h"
#include "vk_widget.h"
#include "vk_container.h"
#include "vk_box.h"
#include "vk_frame.h"
#include "vk_input.h"
#include "vk_listbox.h"
#include "vk_scroller.h"
#include "vk_button.h"
#include "vk_item.h"
#include "vk_filedialog.h"

/*
    Case-insensitive ASCII compare of two byte ranges of length n.
    Inlined so this file doesn't have to depend on strncasecmp (which
    needs the right feature-test macro to be exposed through <strings.h>
    and was getting shadowed somewhere in the libviper header chain).
*/
static int
_vk_filedialog_icmp(const char *a, const char *b, int n)
{
    int     i;
    int     ca;
    int     cb;

    for(i = 0; i < n; i++)
    {
        ca = (unsigned char)a[i];
        cb = (unsigned char)b[i];
        if(ca >= 'A' && ca <= 'Z') ca += 32;
        if(cb >= 'A' && cb <= 'Z') cb += 32;
        if(ca != cb) return ca - cb;
    }
    return 0;
}

/*
    Match the part of `filename` after its final '.' against a
    comma-separated list of extensions (no leading dots).  Comparison is
    case-insensitive.  exts == NULL or "" means "match anything".  Files
    with no extension never match a non-empty list.
*/
static bool
_vk_filedialog_ext_match(const char *filename, const char *exts)
{
    const char  *dot;
    const char  *p;
    int          ext_len;

    if(exts == NULL || *exts == '\0') return true;

    dot = strrchr(filename, '.');
    if(dot == NULL || dot == filename) return false;

    dot++;
    ext_len = (int)strlen(dot);

    p = exts;
    while(*p != '\0')
    {
        const char *end = strchr(p, ',');
        int         len = (end != NULL) ? (int)(end - p) : (int)strlen(p);

        if(len == ext_len && _vk_filedialog_icmp(p, dot, len) == 0)
            return true;

        if(end == NULL) break;
        p = end + 1;
    }

    return false;
}

static int
_vk_filedialog_ctor(vk_object_t *object, va_list *argp, ...);

static int
_vk_filedialog_dtor(vk_object_t *object);

static int
_vk_filedialog_populate(vk_filedialog_t *dialog);

static void
_vk_filedialog_activate(vk_filedialog_t *dialog);

static void
_vk_filedialog_go_parent(vk_filedialog_t *dialog);

static int
_vk_filedialog_kmio(vk_object_t *object, int32_t keystroke);

static int
_vk_filedialog_input_key(vk_filedialog_t *dialog, int32_t keystroke);

static int
_vk_filedialog_list_key(vk_filedialog_t *dialog, int32_t keystroke);

static void
_vk_filedialog_scroll_info(vk_widget_t *child,
    int *content_h, int *content_w,
    int *scroll_y, int *scroll_x)
{
    vk_listbox_t *lb = VK_LISTBOX(child);

    if(content_h) *content_h = lb->item_count;
    if(content_w) *content_w = 0;
    if(scroll_y) *scroll_y = lb->scroll_top;
    if(scroll_x) *scroll_x = 0;
}

static void
_vk_filedialog_set_child(vk_filedialog_t *dialog, int slot,
    vk_widget_t *widget)
{
    vk_box_t        *box = VK_BOX(dialog);
    vk_container_t  *container = VK_CONTAINER(dialog);

    box->slot_widgets[slot] = widget;

    if(widget != NULL)
    {
        container->add_widget(container, widget);
        vk_widget_set_surface(widget, VK_WIDGET(dialog)->canvas);
    }
}

require_klass(VK_BOX_KLASS);

declare_klass(VK_FILEDIALOG_KLASS)
{
    .size = KLASS_SIZE(vk_filedialog_t),
    .name = KLASS_NAME(vk_filedialog_t),
    .ctor = _vk_filedialog_ctor,
    .dtor = _vk_filedialog_dtor,
};

inline vk_filedialog_t*
vk_filedialog_create(int width, int height, int style, bool multiselect)
{
    vk_filedialog_t *dialog;
    int             btn_h;

    if(width < 10 || height < 5) return NULL;

    if(style != VK_BORDER_SINGLE && style != VK_BORDER_ASCII
        && style != VK_BUTTON_BASIC)
        style = VK_BUTTON_BASIC;

    dialog = (vk_filedialog_t *)vk_object_create(VK_FILEDIALOG_KLASS,
        width, height, VK_BOX_VERTICAL, 3, style, (int)multiselect);

    if(dialog == NULL) return NULL;

    VK_BOX(dialog)->homogeneous = false;

    btn_h = (style == VK_BUTTON_BASIC) ? 1 : 3;

    dialog->path_input = vk_input_create(width);
    if(style != VK_BORDER_SINGLE)
        vk_input_set_border_style(dialog->path_input, style);

    if(multiselect)
    {
        dialog->file_list = VK_LISTBOX(
            vk_selectbox_create(width, height - btn_h * 2,
                VK_SELECTBOX_CHECKBOX));
    }
    else
    {
        dialog->file_list = vk_listbox_create(width, height - btn_h * 2);
    }
    vk_widget_set_expand(VK_WIDGET(dialog->file_list));

    dialog->scroller = vk_scroller_create(VK_SCROLLBAR_VERTICAL);
    vk_scroller_set_border_style(dialog->scroller,
        (style == VK_BORDER_ASCII) ? VK_BORDER_ASCII : VK_BORDER_SINGLE);
    vk_scroller_set_scroll_source(dialog->scroller,
        VK_WIDGET(dialog->file_list));
    vk_scroller_set_scroll_info(dialog->scroller,
        _vk_filedialog_scroll_info);
    vk_widget_attach_scroller(VK_WIDGET(dialog->file_list),
        dialog->scroller);

    dialog->btn_ok = vk_button_create("Okay");
    dialog->btn_cancel = vk_button_create("Cancel");

    if(style == VK_BUTTON_BASIC)
    {
        vk_button_set_border_style(dialog->btn_ok, VK_BUTTON_BASIC);
        vk_button_set_border_style(dialog->btn_cancel, VK_BUTTON_BASIC);
    }
    else if(style == VK_BORDER_ASCII)
    {
        vk_button_set_border_style(dialog->btn_ok, VK_BORDER_ASCII);
        vk_button_set_border_style(dialog->btn_cancel, VK_BORDER_ASCII);
    }

    dialog->button_bar = vk_box_create(width, btn_h, VK_BOX_HORIZONTAL, 2);
    vk_box_set_widget(dialog->button_bar, 0, VK_WIDGET(dialog->btn_ok));
    vk_box_set_widget(dialog->button_bar, 1, VK_WIDGET(dialog->btn_cancel));

    /*
        Wrap the file_list in a sunken-relief frame for visual
        consistency with the rest of the picker tools.  The frame
        occupies slot 1 of the filedialog box; the file_list becomes
        the frame's expanding child.  Costs two rows + two columns
        from the visible listing.
    */
    dialog->list_frame = vk_frame_create(width, height - btn_h * 2);
    vk_frame_set_border_style(dialog->list_frame,
        VK_BORDER_SINGLE | VK_RELIEF_SUNKEN);
    vk_frame_set_border_attrs(dialog->list_frame, A_BOLD);
    vk_widget_set_expand(VK_WIDGET(dialog->list_frame));
    vk_frame_set_child(dialog->list_frame, VK_WIDGET(dialog->file_list));

    _vk_filedialog_set_child(dialog, 0, VK_WIDGET(dialog->path_input));
    _vk_filedialog_set_child(dialog, 1, VK_WIDGET(dialog->list_frame));
    _vk_filedialog_set_child(dialog, 2, VK_WIDGET(dialog->button_bar));

    VK_BOX(dialog)->focused_slot = 1;

    vk_object_set_kmio(VK_OBJECT(dialog), _vk_filedialog_kmio);

    {
        const char *home = getenv("HOME");
        vk_filedialog_set_path(dialog, (home != NULL) ? home : ".");
    }

    return dialog;
}

inline void
vk_filedialog_set_filter(vk_filedialog_t *dialog, const char *exts)
{
    if(dialog == NULL) return;

    if(dialog->exts != NULL)
    {
        free(dialog->exts);
        dialog->exts = NULL;
    }

    if(exts != NULL && *exts != '\0')
        dialog->exts = strdup(exts);

    /* refresh the listing so an already-open dialog reflects the new
       filter immediately on the next update. */
    if(dialog->path != NULL)
        _vk_filedialog_populate(dialog);
}

inline int
vk_filedialog_set_path(vk_filedialog_t *dialog, const char *path)
{
    char resolved[PATH_MAX];

    if(dialog == NULL || path == NULL) return -1;

    if(realpath(path, resolved) == NULL) return -1;

    if(dialog->path != NULL) free(dialog->path);
    dialog->path = strdup(resolved);

    return _vk_filedialog_populate(dialog);
}

inline const char*
vk_filedialog_get_path(vk_filedialog_t *dialog)
{
    if(dialog == NULL) return NULL;

    return dialog->path;
}

inline const char*
vk_filedialog_get_selected(vk_filedialog_t *dialog)
{
    vk_listbox_t        *lb;
    struct list_head    *pos;
    vk_item_t           *item;
    int                 idx = 0;

    if(dialog == NULL) return NULL;

    lb = dialog->file_list;

    list_for_each(pos, &lb->item_list)
    {
        if(idx == lb->curr_item)
        {
            item = list_entry(pos, vk_item_t, list);
            return item->name;
        }
        idx++;
    }

    return NULL;
}

inline int
vk_filedialog_set_colors(vk_filedialog_t *dialog, short fg, short bg)
{
    if(dialog == NULL) return -1;

    vk_widget_set_colors(VK_WIDGET(dialog), fg, bg);
    vk_widget_set_colors(VK_WIDGET(dialog->path_input), fg, bg);
    vk_widget_set_colors(VK_WIDGET(dialog->file_list), fg, bg);
    if(dialog->list_frame != NULL)
    {
        /* widget bg shows briefly between the border and the file_list
           on erase, so keep it in the dialog's theme.  border_bg is the
           background each relief cell paints against. */
        vk_widget_set_colors(VK_WIDGET(dialog->list_frame), fg, bg);
        vk_frame_set_border_colors(dialog->list_frame, fg, bg);
    }
    if(dialog->scroller != NULL)
        vk_scroller_set_border_colors(dialog->scroller, fg, bg);
    vk_widget_set_colors(VK_WIDGET(dialog->btn_ok), fg, bg);
    vk_widget_set_colors(VK_WIDGET(dialog->btn_cancel), fg, bg);
    vk_widget_set_colors(VK_WIDGET(dialog->button_bar), fg, bg);

    vk_widget_fill_pair(VK_WIDGET(dialog->button_bar), L' ', 0,
        vdk_color_pair(fg, bg));

    return 0;
}

inline int
vk_filedialog_set_wrap(vk_filedialog_t *dialog, bool allowed)
{
    if(dialog == NULL) return -1;

    vk_listbox_set_wrap(dialog->file_list, allowed);

    return 0;
}

inline int
vk_filedialog_set_highlight(vk_filedialog_t *dialog, short fg, short bg)
{
    if(dialog == NULL) return -1;

    vk_listbox_set_highlight(dialog->file_list, fg, bg);

    return 0;
}

inline int
vk_filedialog_set_button_colors(vk_filedialog_t *dialog, short fg, short bg)
{
    if(dialog == NULL) return -1;

    vk_widget_set_colors(VK_WIDGET(dialog->btn_ok), fg, bg);
    vk_widget_set_colors(VK_WIDGET(dialog->btn_cancel), fg, bg);
    vk_widget_set_colors(VK_WIDGET(dialog->button_bar), fg, bg);

    vk_widget_fill_pair(VK_WIDGET(dialog->button_bar), L' ', 0,
        vdk_color_pair(fg, bg));

    return 0;
}

inline int
vk_filedialog_set_button_attrs(vk_filedialog_t *dialog, attr_t attrs)
{
    if(dialog == NULL) return -1;

    vk_widget_set_attrs(VK_WIDGET(dialog->btn_ok), attrs);
    vk_widget_set_attrs(VK_WIDGET(dialog->btn_cancel), attrs);

    return 0;
}

inline vk_listbox_t*
vk_filedialog_get_file_list(vk_filedialog_t *dialog)
{
    if(dialog == NULL) return NULL;

    return dialog->file_list;
}

inline int
vk_filedialog_update(vk_filedialog_t *dialog)
{
    if(dialog == NULL) return -1;

    vk_input_update(dialog->path_input);

    /*
        Render the file_list content into its own canvas FIRST, then run
        the frame update.  vk_frame_update erases the frame canvas and
        draws the sunken border on it, then blits the child's pre-
        rendered canvas inside.  Swap the order and the listbox content
        gets wiped by the frame's erase.
    */
    dialog->file_list->_update(dialog->file_list);
    if(dialog->list_frame != NULL)
        vk_frame_update(dialog->list_frame);

    if(dialog->scroller != NULL)
    {
        if(vk_scroller_update(dialog->scroller) > 0)
            vk_widget_draw(VK_WIDGET(dialog->scroller));
    }

    vk_button_update(dialog->btn_ok);
    vk_button_update(dialog->btn_cancel);
    vk_box_update(dialog->button_bar);
    VK_BOX(dialog)->_update(VK_BOX(dialog));

    return 0;
}

inline void
vk_filedialog_destroy(vk_filedialog_t *dialog)
{
    if(dialog == NULL) return;

    if(!vk_object_assert(dialog, vk_filedialog_t)) return;

    dialog->dtor(VK_OBJECT(dialog));
}

static int
_vk_filedialog_ctor(vk_object_t *object, va_list *argp, ...)
{
    vk_filedialog_t *dialog;
    va_list         args;

    if(object == NULL) return -1;

    if(argp == NULL)
    {
        va_start(args, argp);
        argp = &args;
    }

    VK_BOX_KLASS->ctor(object, argp);

    dialog = VK_FILEDIALOG(object);

    dialog->style = va_arg(*argp, int);
    dialog->multiselect = (bool)va_arg(*argp, int);

    va_end(args);

    dialog->path = NULL;
    dialog->exts = NULL;
    dialog->path_input = NULL;
    dialog->list_frame = NULL;
    dialog->file_list = NULL;
    dialog->scroller = NULL;
    dialog->button_bar = NULL;
    dialog->btn_ok = NULL;
    dialog->btn_cancel = NULL;

    dialog->ctor = _vk_filedialog_ctor;
    dialog->dtor = _vk_filedialog_dtor;

    return 0;
}

static int
_vk_filedialog_dtor(vk_object_t *object)
{
    vk_filedialog_t *dialog;
    vk_box_t        *box;
    vk_container_t  *container;
    vk_input_t      *path_input;
    vk_listbox_t    *file_list;
    vk_scroller_t   *scroller;
    vk_box_t        *button_bar;
    vk_button_t     *btn_ok;
    vk_button_t     *btn_cancel;
    bool            multiselect;
    int             i;

    if(object == NULL) return -1;

    if(!vk_object_assert(object, vk_filedialog_t)) return -1;

    dialog = VK_FILEDIALOG(object);
    box = VK_BOX(object);
    container = VK_CONTAINER(object);

    path_input = dialog->path_input;
    file_list = dialog->file_list;
    scroller = dialog->scroller;
    button_bar = dialog->button_bar;
    btn_ok = dialog->btn_ok;
    btn_cancel = dialog->btn_cancel;
    multiselect = dialog->multiselect;

    if(dialog->path != NULL)
        free(dialog->path);

    if(dialog->exts != NULL)
        free(dialog->exts);

    for(i = 0; i < box->slots; i++)
    {
        if(box->slot_widgets[i] != NULL)
            container->remove_widget(container, box->slot_widgets[i]);
        box->slot_widgets[i] = NULL;
    }

    vk_object_demote(object, vk_box_t);
    vk_box_destroy(VK_BOX(object));

    vk_scroller_destroy(scroller);

    vk_box_destroy(button_bar);
    vk_button_destroy(btn_ok);
    vk_button_destroy(btn_cancel);

    /* destroy the frame first: it removes file_list from its container
       but does not free file_list; we free file_list explicitly next. */
    if(dialog->list_frame != NULL)
        vk_frame_destroy(dialog->list_frame);

    if(multiselect)
        vk_selectbox_destroy(VK_SELECTBOX(file_list));
    else
        vk_listbox_destroy(file_list);

    vk_input_destroy(path_input);

    return 0;
}

static int
_vk_filedialog_populate(vk_filedialog_t *dialog)
{
    struct dirent   **namelist;
    struct stat     st;
    char            fullpath[PATH_MAX];
    int             n, i;
    vk_listbox_t    *lb = dialog->file_list;

    lb->_reset(lb);

    n = scandir(dialog->path, &namelist, NULL, alphasort);
    if(n < 0) return -1;

    if(strcmp(dialog->path, "/") != 0)
        lb->_add_item(lb, "..", NULL, NULL);

    for(i = 0; i < n; i++)
    {
        if(strcmp(namelist[i]->d_name, ".") == 0) continue;
        if(strcmp(namelist[i]->d_name, "..") == 0) continue;
        if(namelist[i]->d_name[0] == '.') continue;

        snprintf(fullpath, sizeof(fullpath), "%s/%s",
            dialog->path, namelist[i]->d_name);

        if(stat(fullpath, &st) == 0 && S_ISDIR(st.st_mode))
        {
            char name[NAME_MAX + 2];
            snprintf(name, sizeof(name), "%s/", namelist[i]->d_name);
            lb->_add_item(lb, name, NULL, NULL);
        }
    }

    for(i = 0; i < n; i++)
    {
        if(strcmp(namelist[i]->d_name, ".") == 0) continue;
        if(strcmp(namelist[i]->d_name, "..") == 0) continue;
        if(namelist[i]->d_name[0] == '.') continue;

        snprintf(fullpath, sizeof(fullpath), "%s/%s",
            dialog->path, namelist[i]->d_name);

        if(stat(fullpath, &st) != 0 || !S_ISDIR(st.st_mode))
        {
            /* directories already added in the first pass; this pass
               handles regular files only, and is where the extension
               filter applies. */
            if(!_vk_filedialog_ext_match(namelist[i]->d_name, dialog->exts))
                continue;

            lb->_add_item(lb, namelist[i]->d_name, NULL, NULL);
        }
    }

    for(i = 0; i < n; i++)
        free(namelist[i]);
    free(namelist);

    vk_input_set_text(dialog->path_input, dialog->path);
    lb->curr_item = 0;
    lb->scroll_top = 0;

    return 0;
}

static void
_vk_filedialog_activate(vk_filedialog_t *dialog)
{
    vk_listbox_t        *lb = dialog->file_list;
    struct list_head    *pos;
    vk_item_t           *item;
    int                 idx = 0;

    list_for_each(pos, &lb->item_list)
    {
        if(idx == lb->curr_item)
        {
            item = list_entry(pos, vk_item_t, list);

            if(strcmp(item->name, "..") == 0)
            {
                _vk_filedialog_go_parent(dialog);
                return;
            }

            int len = strlen(item->name);
            if(len > 0 && item->name[len - 1] == '/')
            {
                char dirname[NAME_MAX + 1];
                char newpath[PATH_MAX];

                memcpy(dirname, item->name, len - 1);
                dirname[len - 1] = '\0';

                if(strcmp(dialog->path, "/") == 0)
                    snprintf(newpath, sizeof(newpath), "/%s", dirname);
                else
                    snprintf(newpath, sizeof(newpath), "%s/%s",
                        dialog->path, dirname);

                free(dialog->path);
                dialog->path = strdup(newpath);
                _vk_filedialog_populate(dialog);
                return;
            }

            return;
        }
        idx++;
    }
}

static void
_vk_filedialog_go_parent(vk_filedialog_t *dialog)
{
    char *slash;

    if(strcmp(dialog->path, "/") == 0) return;

    slash = strrchr(dialog->path, '/');
    if(slash != NULL && slash != dialog->path)
        *slash = '\0';
    else if(slash == dialog->path)
        dialog->path[1] = '\0';

    _vk_filedialog_populate(dialog);
}

static int
_vk_filedialog_input_key(vk_filedialog_t *dialog, int32_t keystroke)
{
    vk_input_t  *input = dialog->path_input;
    vk_box_t    *box = VK_BOX(dialog);

    switch(keystroke)
    {
        case 27:
            vk_input_set_text(input, dialog->path);
            box->focused_slot = 1;
            break;

        case KEY_CRLF:
        {
            const char *text = vk_input_get_text(input);
            if(text != NULL && text[0] != '\0')
            {
                char resolved[PATH_MAX];
                if(realpath(text, resolved) != NULL)
                {
                    free(dialog->path);
                    dialog->path = strdup(resolved);
                    _vk_filedialog_populate(dialog);
                }
            }
            box->focused_slot = 1;
            break;
        }

        case KEY_LEFT:
            vk_input_move_cursor(input, -1);
            break;
        case KEY_RIGHT:
            vk_input_move_cursor(input, 1);
            break;
        case KEY_HOME:
            vk_input_home(input);
            break;
        case KEY_END:
            vk_input_end(input);
            break;
        case KEY_BACKSPACE:
        case 127:
            vk_input_backspace(input);
            break;
        case KEY_DC:
            vk_input_delete(input);
            break;
        default:
            if(keystroke >= 32 && keystroke <= 126)
                vk_input_insert_char(input, keystroke);
            break;
    }

    vk_input_update(input);
    return 0;
}

static int
_vk_filedialog_list_key(vk_filedialog_t *dialog, int32_t keystroke)
{
    vk_listbox_t *lb = dialog->file_list;

    switch(keystroke)
    {
        case KEY_UP:
            if(lb->curr_item > 0)
                lb->curr_item--;
            else if(lb->flags & VK_FLAG_ALLOW_WRAP)
                lb->curr_item = lb->item_count - 1;
            break;

        case KEY_DOWN:
            if(lb->curr_item < lb->item_count - 1)
                lb->curr_item++;
            else if(lb->flags & VK_FLAG_ALLOW_WRAP)
                lb->curr_item = 0;
            break;

        case KEY_CRLF:
            _vk_filedialog_activate(dialog);
            break;

        case ' ':
            if(dialog->multiselect)
                vk_selectbox_toggle_item(VK_SELECTBOX(lb), lb->curr_item);
            break;

        case KEY_BACKSPACE:
        case 127:
            _vk_filedialog_go_parent(dialog);
            break;

        default:
            return 0;
    }

    lb->_update(lb);
    return 0;
}

static int
_vk_filedialog_kmio(vk_object_t *object, int32_t keystroke)
{
    vk_filedialog_t *dialog = VK_FILEDIALOG(object);
    vk_box_t        *box = VK_BOX(dialog);

    if(box->focused_slot == 0)
        return _vk_filedialog_input_key(dialog, keystroke);

    if(keystroke == '/')
    {
        box->focused_slot = 0;
        return 0;
    }

    return _vk_filedialog_list_key(dialog, keystroke);
}
