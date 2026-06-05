#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "vk_object.h"
#include "vk_widget.h"
#include "vk_button.h"

static int
_vk_button_ctor(vk_object_t *object, va_list *argp, ...);

static int
_vk_button_dtor(vk_object_t *object);

static int
_vk_button_update(vk_button_t *button);

static void
_vk_button_build_cchar(cchar_t *dest, const cchar_t *src, short pair,
    attr_t extra);


require_klass(VK_WIDGET_KLASS);

declare_klass(VK_BUTTON_KLASS)
{
    .size = KLASS_SIZE(vk_button_t),
    .name = KLASS_NAME(vk_button_t),
    .ctor = _vk_button_ctor,
    .dtor = _vk_button_dtor,
};


inline vk_button_t*
vk_button_create(const char *text)
{
    vk_button_t *button;
    int         text_len;
    int         width;

    if(text == NULL) return NULL;

    text_len = strlen(text);
    if(text_len < 1) return NULL;

    width = 1 + text_len + 1;

    button = (vk_button_t*)vk_object_create(VK_BUTTON_KLASS, width, 3);

    if(button != NULL)
        button->text = strdup(text);

    return button;
}

inline int
vk_button_set_text(vk_button_t *button, const char *text)
{
    if(button == NULL) return -1;

    if(!vk_object_assert(button, vk_button_t)) return -1;

    if(button->text != NULL)
    {
        free(button->text);
        button->text = NULL;
    }

    if(text != NULL)
        button->text = strdup(text);

    return 0;
}

inline const char*
vk_button_get_text(vk_button_t *button)
{
    if(button == NULL) return NULL;

    if(!vk_object_assert(button, vk_button_t)) return NULL;

    return button->text;
}

inline int
vk_button_set_relief_style(vk_button_t *button, int style)
{
    if(button == NULL) return -1;

    if(!vk_object_assert(button, vk_button_t)) return -1;

    if(style != VK_FRAME_SINGLE && style != VK_FRAME_ASCII) return -1;

    button->relief_style = style;

    return 0;
}

inline void
vk_button_set_pressed_colors(vk_button_t *button, short fg, short bg)
{
    if(button == NULL) return;

    if(!vk_object_assert(button, vk_button_t)) return;

    button->pressed_fg = fg;
    button->pressed_bg = bg;
}

inline int
vk_button_set_on_press(vk_button_t *button, VkWidgetFunc func, void *anything)
{
    if(button == NULL) return -1;

    if(!vk_object_assert(button, vk_button_t)) return -1;

    button->on_press = func;
    button->anything = anything;

    return 0;
}

inline int
vk_button_press(vk_button_t *button)
{
    if(button == NULL) return -1;

    if(!vk_object_assert(button, vk_button_t)) return -1;

    button->pressed = true;

    if(button->on_press != NULL)
        return button->on_press(VK_WIDGET(button), button->anything);

    return 0;
}

inline int
vk_button_release(vk_button_t *button)
{
    if(button == NULL) return -1;

    if(!vk_object_assert(button, vk_button_t)) return -1;

    button->pressed = false;

    return 0;
}

inline int
vk_button_update(vk_button_t *button)
{
    if(button == NULL) return -1;

    if(!vk_object_assert(button, vk_button_t)) return -1;

    return button->_update(button);
}

inline void
vk_button_destroy(vk_button_t *button)
{
    if(button == NULL) return;

    if(!vk_object_assert(button, vk_button_t)) return;

    button->dtor(VK_OBJECT(button));
}


static int
_vk_button_ctor(vk_object_t *object, va_list *argp, ...)
{
    vk_button_t *button;
    va_list     args;

    if(object == NULL) return -1;

    if(argp == NULL)
    {
        va_start(args, argp);
        argp = &args;
    }

    VK_WIDGET_KLASS->ctor(object, argp);

    va_end(args);

    button = VK_BUTTON(object);

    button->text = NULL;
    button->relief_style = VK_FRAME_SINGLE;
    button->pressed = false;
    button->pressed_fg = COLOR_WHITE;
    button->pressed_bg = COLOR_BLACK;
    button->on_press = NULL;
    button->anything = NULL;

    button->ctor = _vk_button_ctor;
    button->dtor = _vk_button_dtor;
    button->_update = _vk_button_update;

    return 0;
}

static int
_vk_button_dtor(vk_object_t *object)
{
    vk_button_t *button;

    if(object == NULL) return -1;

    if(!vk_object_assert(object, vk_button_t)) return -1;

    button = VK_BUTTON(object);

    if(button->text != NULL)
    {
        free(button->text);
        button->text = NULL;
    }

    vk_object_demote(object, vk_widget_t);
    vk_widget_destroy(VK_WIDGET(object));

    return 0;
}

static void
_vk_button_build_cchar(cchar_t *dest, const cchar_t *src, short pair,
    attr_t extra)
{
    wchar_t     wch[CCHARW_MAX];
    attr_t      attrs;
    short       dummy;

    getcchar(src, wch, &attrs, &dummy, NULL);
    setcchar(dest, wch, attrs | extra, pair, NULL);
}

static int
_vk_button_update(vk_button_t *button)
{
    vk_widget_t *widget;
    short       fg, bg;
    int         face_colors;
    short       hi_pair;
    short       sh_pair;
    int         right_col;
    int         bottom_row;
    int         i;

    if(button == NULL) return -1;

    widget = VK_WIDGET(button);
    widget->_erase(widget);

    if(button->pressed)
    {
        fg = button->pressed_fg;
        bg = button->pressed_bg;
    }
    else
    {
        fg = widget->fg;
        bg = widget->bg;
    }

    face_colors = COLOR_PAIR(vdk_color_pair(fg, bg)) | widget->attrs;

    hi_pair = button->pressed
        ? vdk_color_pair(COLOR_BLACK, bg)
        : vdk_color_pair(COLOR_WHITE, bg);

    sh_pair = button->pressed
        ? vdk_color_pair(COLOR_WHITE, bg)
        : vdk_color_pair(COLOR_BLACK, bg);

    wbkgd(widget->canvas, ' ' | face_colors);

    right_col = widget->width - 1;
    bottom_row = widget->height - 1;

    if(button->text != NULL)
    {
        int text_len = strlen(button->text);
        int max_text = widget->width - 2;
        int text_col;

        if(text_len > max_text) text_len = max_text;
        text_col = 1 + (max_text - text_len) / 2;

        wattron(widget->canvas, face_colors);
        mvwprintw(widget->canvas, 1, text_col, "%.*s", max_text, button->text);
        wattroff(widget->canvas, face_colors);
    }

    switch(button->relief_style)
    {
        case VK_FRAME_ASCII:
        {
            int hi_colors = COLOR_PAIR(hi_pair) | widget->attrs;
            int sh_colors = COLOR_PAIR(sh_pair) | widget->attrs;

            mvwaddch(widget->canvas, 0, 0, '+' | hi_colors);
            for(i = 1; i < right_col; i++)
                mvwaddch(widget->canvas, 0, i, '-' | hi_colors);
            mvwaddch(widget->canvas, 0, right_col, '+' | sh_colors);

            for(i = 1; i < bottom_row; i++)
            {
                mvwaddch(widget->canvas, i, 0, '|' | hi_colors);
                mvwaddch(widget->canvas, i, right_col, '|' | sh_colors);
            }

            mvwaddch(widget->canvas, bottom_row, 0, '+' | hi_colors);
            for(i = 1; i < right_col; i++)
                mvwaddch(widget->canvas, bottom_row, i, '-' | sh_colors);
            mvwaddch(widget->canvas, bottom_row, right_col, '+' | sh_colors);
            break;
        }

        default:
        {
            cchar_t cc;

            _vk_button_build_cchar(&cc, WACS_ULCORNER, hi_pair, 0);
            mvwadd_wch(widget->canvas, 0, 0, &cc);

            _vk_button_build_cchar(&cc, WACS_HLINE, hi_pair, 0);
            for(i = 1; i < right_col; i++)
                mvwadd_wch(widget->canvas, 0, i, &cc);

            _vk_button_build_cchar(&cc, WACS_URCORNER, sh_pair, 0);
            mvwadd_wch(widget->canvas, 0, right_col, &cc);

            _vk_button_build_cchar(&cc, WACS_VLINE, hi_pair, 0);
            for(i = 1; i < bottom_row; i++)
                mvwadd_wch(widget->canvas, i, 0, &cc);

            _vk_button_build_cchar(&cc, WACS_VLINE, sh_pair, 0);
            for(i = 1; i < bottom_row; i++)
                mvwadd_wch(widget->canvas, i, right_col, &cc);

            _vk_button_build_cchar(&cc, WACS_LLCORNER, hi_pair, 0);
            mvwadd_wch(widget->canvas, bottom_row, 0, &cc);

            _vk_button_build_cchar(&cc, WACS_HLINE, sh_pair, 0);
            for(i = 1; i < right_col; i++)
                mvwadd_wch(widget->canvas, bottom_row, i, &cc);

            _vk_button_build_cchar(&cc, WACS_LRCORNER, sh_pair, 0);
            mvwadd_wch(widget->canvas, bottom_row, right_col, &cc);
            break;
        }
    }

    return 0;
}
