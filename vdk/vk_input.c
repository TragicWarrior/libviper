#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "vk_object.h"
#include "vk_widget.h"
#include "vk_input.h"

#define VK_INPUT_INIT_CAP       256

static int
_vk_input_ctor(vk_object_t *object, va_list *argp, ...);

static int
_vk_input_dtor(vk_object_t *object);

static int
_vk_input_update(vk_input_t *input);

static void
_vk_input_build_cchar(cchar_t *dest, const cchar_t *src, short pair,
    attr_t extra);

static int
_vk_input_field_width(vk_input_t *input)
{
    vk_widget_t *widget = VK_WIDGET(input);

    if(input->relief_style == VK_BUTTON_BASIC)
        return widget->width - 4;

    return widget->width - 2;
}

static int
_vk_input_ensure_capacity(vk_input_t *input, int needed)
{
    int     new_cap;
    char    *new_text;

    if(needed < input->capacity) return 0;

    new_cap = input->capacity;
    while(new_cap <= needed)
        new_cap *= 2;

    new_text = realloc(input->text, new_cap);
    if(new_text == NULL) return -1;

    input->text = new_text;
    input->capacity = new_cap;

    return 0;
}


require_klass(VK_WIDGET_KLASS);

declare_klass(VK_INPUT_KLASS)
{
    .size = KLASS_SIZE(vk_input_t),
    .name = KLASS_NAME(vk_input_t),
    .ctor = _vk_input_ctor,
    .dtor = _vk_input_dtor,
};


inline vk_input_t*
vk_input_create(int width)
{
    vk_input_t  *input;

    if(width < 5) return NULL;

    input = (vk_input_t*)vk_object_create(VK_INPUT_KLASS, width, 3);

    if(input != NULL)
    {
        input->text = calloc(VK_INPUT_INIT_CAP, 1);
        input->capacity = VK_INPUT_INIT_CAP;
    }

    return input;
}

inline int
vk_input_set_text(vk_input_t *input, const char *text)
{
    int len;

    if(input == NULL) return -1;

    if(!vk_object_assert(input, vk_input_t)) return -1;

    if(text == NULL)
    {
        input->text[0] = '\0';
        input->text_len = 0;
        input->cursor = 0;
        input->scroll = 0;
        return 0;
    }

    len = strlen(text);

    if(input->max_len > 0 && len > input->max_len)
        len = input->max_len;

    if(_vk_input_ensure_capacity(input, len + 1) != 0)
        return -1;

    memcpy(input->text, text, len);
    input->text[len] = '\0';
    input->text_len = len;
    input->cursor = len;
    input->scroll = 0;

    return 0;
}

inline const char*
vk_input_get_text(vk_input_t *input)
{
    if(input == NULL) return NULL;

    if(!vk_object_assert(input, vk_input_t)) return NULL;

    return input->text;
}

inline int
vk_input_set_relief_style(vk_input_t *input, int style)
{
    vk_widget_t *widget;

    if(input == NULL) return -1;

    if(!vk_object_assert(input, vk_input_t)) return -1;

    if(style != VK_FRAME_SINGLE && style != VK_FRAME_ASCII
        && style != VK_BUTTON_BASIC)
        return -1;

    input->relief_style = style;

    widget = VK_WIDGET(input);

    if(style == VK_BUTTON_BASIC)
        vk_widget_resize(widget, widget->width, 1);
    else
        vk_widget_resize(widget, widget->width, 3);

    return 0;
}

inline int
vk_input_set_max_length(vk_input_t *input, int max)
{
    if(input == NULL) return -1;

    if(!vk_object_assert(input, vk_input_t)) return -1;

    input->max_len = (max > 0) ? max : 0;

    if(input->max_len > 0 && input->text_len > input->max_len)
    {
        input->text[input->max_len] = '\0';
        input->text_len = input->max_len;

        if(input->cursor > input->text_len)
            input->cursor = input->text_len;
    }

    return 0;
}

inline int
vk_input_insert_char(vk_input_t *input, int ch)
{
    if(input == NULL) return -1;

    if(!vk_object_assert(input, vk_input_t)) return -1;

    if(ch < 32 || ch > 126) return -1;

    if(input->max_len > 0 && input->text_len >= input->max_len)
        return -1;

    if(_vk_input_ensure_capacity(input, input->text_len + 2) != 0)
        return -1;

    memmove(&input->text[input->cursor + 1],
        &input->text[input->cursor],
        input->text_len - input->cursor + 1);

    input->text[input->cursor] = (char)ch;
    input->text_len++;
    input->cursor++;

    return 0;
}

inline int
vk_input_backspace(vk_input_t *input)
{
    if(input == NULL) return -1;

    if(!vk_object_assert(input, vk_input_t)) return -1;

    if(input->cursor == 0) return -1;

    memmove(&input->text[input->cursor - 1],
        &input->text[input->cursor],
        input->text_len - input->cursor + 1);

    input->text_len--;
    input->cursor--;

    return 0;
}

inline int
vk_input_delete(vk_input_t *input)
{
    if(input == NULL) return -1;

    if(!vk_object_assert(input, vk_input_t)) return -1;

    if(input->cursor >= input->text_len) return -1;

    memmove(&input->text[input->cursor],
        &input->text[input->cursor + 1],
        input->text_len - input->cursor);

    input->text_len--;

    return 0;
}

inline int
vk_input_move_cursor(vk_input_t *input, int offset)
{
    int new_pos;

    if(input == NULL) return -1;

    if(!vk_object_assert(input, vk_input_t)) return -1;

    new_pos = input->cursor + offset;

    if(new_pos < 0) new_pos = 0;
    if(new_pos > input->text_len) new_pos = input->text_len;

    input->cursor = new_pos;

    return 0;
}

inline int
vk_input_home(vk_input_t *input)
{
    if(input == NULL) return -1;

    if(!vk_object_assert(input, vk_input_t)) return -1;

    input->cursor = 0;

    return 0;
}

inline int
vk_input_end(vk_input_t *input)
{
    if(input == NULL) return -1;

    if(!vk_object_assert(input, vk_input_t)) return -1;

    input->cursor = input->text_len;

    return 0;
}

inline int
vk_input_clear(vk_input_t *input)
{
    if(input == NULL) return -1;

    if(!vk_object_assert(input, vk_input_t)) return -1;

    input->text[0] = '\0';
    input->text_len = 0;
    input->cursor = 0;
    input->scroll = 0;

    return 0;
}

inline int
vk_input_update(vk_input_t *input)
{
    if(input == NULL) return -1;

    if(!vk_object_assert(input, vk_input_t)) return -1;

    return input->_update(input);
}

inline void
vk_input_destroy(vk_input_t *input)
{
    if(input == NULL) return;

    if(!vk_object_assert(input, vk_input_t)) return;

    input->dtor(VK_OBJECT(input));
}


static int
_vk_input_ctor(vk_object_t *object, va_list *argp, ...)
{
    vk_input_t  *input;
    va_list     args;

    if(object == NULL) return -1;

    if(argp == NULL)
    {
        va_start(args, argp);
        argp = &args;
    }

    VK_WIDGET_KLASS->ctor(object, argp);

    va_end(args);

    input = VK_INPUT(object);

    input->text = NULL;
    input->text_len = 0;
    input->capacity = 0;
    input->max_len = 0;
    input->cursor = 0;
    input->scroll = 0;
    input->relief_style = VK_FRAME_SINGLE;

    input->ctor = _vk_input_ctor;
    input->dtor = _vk_input_dtor;
    input->_update = _vk_input_update;

    return 0;
}

static int
_vk_input_dtor(vk_object_t *object)
{
    vk_input_t *input;

    if(object == NULL) return -1;

    if(!vk_object_assert(object, vk_input_t)) return -1;

    input = VK_INPUT(object);

    if(input->text != NULL)
    {
        free(input->text);
        input->text = NULL;
    }

    vk_object_demote(object, vk_widget_t);
    vk_widget_destroy(VK_WIDGET(object));

    return 0;
}

static void
_vk_input_build_cchar(cchar_t *dest, const cchar_t *src, short pair,
    attr_t extra)
{
    wchar_t     wch[CCHARW_MAX];
    attr_t      attrs;
    short       dummy;

    getcchar(src, wch, &attrs, &dummy, NULL);
    setcchar(dest, wch, attrs | extra, pair, NULL);
}

static int
_vk_input_update(vk_input_t *input)
{
    vk_widget_t *widget;
    short       fg, bg;
    int         face_colors;
    int         field_start;
    int         field_width;
    int         text_row;
    int         right_col;
    int         bottom_row;
    int         cursor_col;
    int         i;

    if(input == NULL) return -1;

    widget = VK_WIDGET(input);
    widget->_erase(widget);

    fg = widget->fg;
    bg = widget->bg;

    face_colors = COLOR_PAIR(vdk_color_pair(fg, bg)) | widget->attrs;

    vk_widget_fill(widget, ' ' | face_colors);

    right_col = widget->width - 1;
    bottom_row = widget->height - 1;

    switch(input->relief_style)
    {
        case VK_BUTTON_BASIC:
        {
            field_start = 2;
            text_row = 0;

            wattron(widget->canvas, face_colors);
            mvwaddch(widget->canvas, 0, 0, '[');
            mvwaddch(widget->canvas, 0, right_col, ']');
            wattroff(widget->canvas, face_colors);

            break;
        }

        case VK_FRAME_ASCII:
        {
            short tl_pair = vdk_color_pair(COLOR_BLACK, bg);
            short br_pair = vdk_color_pair(COLOR_WHITE, bg);
            int tl_colors = COLOR_PAIR(tl_pair) | widget->attrs;
            int br_colors = COLOR_PAIR(br_pair) | widget->attrs;

            field_start = 1;
            text_row = 1;

            mvwaddch(widget->canvas, 0, 0, '+' | tl_colors);
            for(i = 1; i < right_col; i++)
                mvwaddch(widget->canvas, 0, i, '-' | tl_colors);
            mvwaddch(widget->canvas, 0, right_col, '+' | tl_colors);

            for(i = 1; i < bottom_row; i++)
            {
                mvwaddch(widget->canvas, i, 0, '|' | tl_colors);
                mvwaddch(widget->canvas, i, right_col, '|' | br_colors);
            }

            mvwaddch(widget->canvas, bottom_row, 0, '+' | br_colors);
            for(i = 1; i < right_col; i++)
                mvwaddch(widget->canvas, bottom_row, i, '-' | br_colors);
            mvwaddch(widget->canvas, bottom_row, right_col, '+' | br_colors);

            break;
        }

        default:
        {
            cchar_t cc;
            short tl_pair = vdk_color_pair(COLOR_BLACK, bg);
            short br_pair = vdk_color_pair(COLOR_WHITE, bg);

            field_start = 1;
            text_row = 1;

            _vk_input_build_cchar(&cc, WACS_ULCORNER, tl_pair, 0);
            mvwadd_wch(widget->canvas, 0, 0, &cc);

            _vk_input_build_cchar(&cc, WACS_HLINE, tl_pair, 0);
            for(i = 1; i < right_col; i++)
                mvwadd_wch(widget->canvas, 0, i, &cc);

            _vk_input_build_cchar(&cc, WACS_URCORNER, tl_pair, 0);
            mvwadd_wch(widget->canvas, 0, right_col, &cc);

            _vk_input_build_cchar(&cc, WACS_VLINE, tl_pair, 0);
            for(i = 1; i < bottom_row; i++)
                mvwadd_wch(widget->canvas, i, 0, &cc);

            _vk_input_build_cchar(&cc, WACS_VLINE, br_pair, 0);
            for(i = 1; i < bottom_row; i++)
                mvwadd_wch(widget->canvas, i, right_col, &cc);

            _vk_input_build_cchar(&cc, WACS_LLCORNER, br_pair, 0);
            mvwadd_wch(widget->canvas, bottom_row, 0, &cc);

            _vk_input_build_cchar(&cc, WACS_HLINE, br_pair, 0);
            for(i = 1; i < right_col; i++)
                mvwadd_wch(widget->canvas, bottom_row, i, &cc);

            _vk_input_build_cchar(&cc, WACS_LRCORNER, br_pair, 0);
            mvwadd_wch(widget->canvas, bottom_row, right_col, &cc);

            break;
        }
    }

    field_width = _vk_input_field_width(input);

    if(field_width < 1) return 0;

    if(input->cursor < input->scroll)
        input->scroll = input->cursor;

    if(input->cursor > input->scroll + field_width - 1)
        input->scroll = input->cursor - field_width + 1;

    if(input->scroll < 0)
        input->scroll = 0;

    cursor_col = input->cursor - input->scroll;

    for(i = 0; i < field_width; i++)
    {
        int     text_idx = input->scroll + i;
        char    ch;
        int     attrs;

        ch = (text_idx < input->text_len) ? input->text[text_idx] : ' ';
        attrs = face_colors;

        if(i == cursor_col)
            attrs |= A_REVERSE;

        wattron(widget->canvas, attrs);
        mvwaddch(widget->canvas, text_row, field_start + i, ch);
        wattroff(widget->canvas, attrs);
    }

    return 0;
}
