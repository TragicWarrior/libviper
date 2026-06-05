#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "vk_object.h"
#include "vk_widget.h"
#include "vk_scroller.h"
#include "vk_textbox.h"

static int
_vk_textbox_ctor(vk_object_t *object, va_list *argp, ...);

static int
_vk_textbox_dtor(vk_object_t *object);

static int
_vk_textbox_update(vk_textbox_t *textbox);

static int
_vk_textbox_on_resize(vk_widget_t *widget);

static int
_vk_textbox_on_recreate(vk_widget_t *widget);

static void
_vk_textbox_reflow(vk_textbox_t *textbox);

static void
_vk_textbox_free_lines(vk_textbox_t *textbox);


require_klass(VK_WIDGET_KLASS);

declare_klass(VK_TEXTBOX_KLASS)
{
    .size = KLASS_SIZE(vk_textbox_t),
    .name = KLASS_NAME(vk_textbox_t),
    .ctor = _vk_textbox_ctor,
    .dtor = _vk_textbox_dtor,
};


inline vk_textbox_t*
vk_textbox_create(int width, int height)
{
    vk_textbox_t    *textbox;

    if(width < 1 || height < 1) return NULL;

    textbox = (vk_textbox_t*)vk_object_create(VK_TEXTBOX_KLASS,
        width, height);

    return textbox;
}

inline int
vk_textbox_set_text(vk_textbox_t *textbox, const char *text)
{
    if(textbox == NULL) return -1;

    if(!vk_object_assert(textbox, vk_textbox_t)) return -1;

    if(textbox->text != NULL) free(textbox->text);

    textbox->text = (text != NULL) ? strdup(text) : NULL;
    textbox->scroll_top = 0;

    _vk_textbox_reflow(textbox);

    return 0;
}

inline const char*
vk_textbox_get_text(vk_textbox_t *textbox)
{
    if(textbox == NULL) return NULL;

    if(!vk_object_assert(textbox, vk_textbox_t)) return NULL;

    return textbox->text;
}

inline int
vk_textbox_set_word_wrap(vk_textbox_t *textbox, bool enabled)
{
    if(textbox == NULL) return -1;

    if(!vk_object_assert(textbox, vk_textbox_t)) return -1;

    if(textbox->word_wrap == enabled) return 0;

    textbox->word_wrap = enabled;
    textbox->scroll_top = 0;

    _vk_textbox_reflow(textbox);

    return 0;
}

inline int
vk_textbox_get_line_count(vk_textbox_t *textbox)
{
    if(textbox == NULL) return -1;

    if(!vk_object_assert(textbox, vk_textbox_t)) return -1;

    return textbox->line_count;
}

inline int
vk_textbox_update(vk_textbox_t *textbox)
{
    if(textbox == NULL) return -1;

    if(!vk_object_assert(textbox, vk_textbox_t)) return -1;

    return textbox->_update(textbox);
}

inline void
vk_textbox_destroy(vk_textbox_t *textbox)
{
    if(textbox == NULL) return;

    if(!vk_object_assert(textbox, vk_textbox_t)) return;

    textbox->dtor(VK_OBJECT(textbox));
}


static int
_vk_textbox_ctor(vk_object_t *object, va_list *argp, ...)
{
    vk_textbox_t    *textbox;
    va_list         args;

    if(object == NULL) return -1;

    if(argp == NULL)
    {
        va_start(args, argp);
        argp = &args;
    }

    VK_WIDGET_KLASS->ctor(object, argp);

    va_end(args);

    textbox = VK_TEXTBOX(object);

    textbox->text = NULL;
    textbox->lines = NULL;
    textbox->line_count = 0;
    textbox->lines_alloc = 0;
    textbox->scroll_top = 0;
    textbox->word_wrap = TRUE;

    textbox->ctor = _vk_textbox_ctor;
    textbox->dtor = _vk_textbox_dtor;
    textbox->_update = _vk_textbox_update;

    VK_WIDGET(textbox)->_on_resize = _vk_textbox_on_resize;
    VK_WIDGET(textbox)->_on_recreate = _vk_textbox_on_recreate;

    return 0;
}

static int
_vk_textbox_dtor(vk_object_t *object)
{
    vk_textbox_t    *textbox;

    if(object == NULL) return -1;

    if(!vk_object_assert(object, vk_textbox_t)) return -1;

    textbox = VK_TEXTBOX(object);

    if(textbox->text != NULL)
    {
        free(textbox->text);
        textbox->text = NULL;
    }

    _vk_textbox_free_lines(textbox);

    vk_object_demote(object, vk_widget_t);
    vk_widget_destroy(VK_WIDGET(object));

    return 0;
}

static int
_vk_textbox_update(vk_textbox_t *textbox)
{
    vk_widget_t     *widget;
    int             paint_width;
    int             paint_height;
    int             paint_colors;
    int             max_top;
    int             y;
    int             i;

    if(textbox == NULL) return -1;

    widget = VK_WIDGET(textbox);

    widget->_erase(widget);

    paint_width = widget->width;
    paint_height = widget->height;

    if(widget->vscroller != NULL) paint_width--;
    if(widget->hscroller != NULL) paint_height--;

    paint_colors = COLOR_PAIR(vdk_color_pair(widget->fg, widget->bg)) | widget->attrs;
    vk_widget_fill(widget, ' ' | paint_colors);
    wattron(widget->canvas, paint_colors);

    if(textbox->line_count <= paint_height)
        textbox->scroll_top = 0;

    max_top = textbox->line_count - paint_height;
    if(max_top < 0) max_top = 0;
    if(textbox->scroll_top > max_top) textbox->scroll_top = max_top;

    y = 0;

    for(i = textbox->scroll_top; i < textbox->line_count && y < paint_height; i++)
    {
        mvwprintw(widget->canvas, y, 0, "%-*.*s",
            paint_width, paint_width, textbox->lines[i]);
        y++;
    }

    while(y < paint_height)
    {
        mvwprintw(widget->canvas, y, 0, "%-*c", paint_width, ' ');
        y++;
    }

    wattroff(widget->canvas, paint_colors);

    if(widget->vscroller != NULL)
    {
        if(vk_scroller_update(widget->vscroller) > 0)
            vk_widget_draw(VK_WIDGET(widget->vscroller));
    }

    if(widget->hscroller != NULL)
    {
        if(vk_scroller_update(widget->hscroller) > 0)
            vk_widget_draw(VK_WIDGET(widget->hscroller));
    }

    return 0;
}

static int
_vk_textbox_on_resize(vk_widget_t *widget)
{
    if(widget->vscroller != NULL)
    {
        vk_widget_resize(VK_WIDGET(widget->vscroller), 1, widget->height);
        vk_widget_move(VK_WIDGET(widget->vscroller), widget->width - 1, 0);
    }

    if(widget->hscroller != NULL)
    {
        vk_widget_resize(VK_WIDGET(widget->hscroller), widget->width, 1);
        vk_widget_move(VK_WIDGET(widget->hscroller), 0, widget->height - 1);
    }

    _vk_textbox_reflow(VK_TEXTBOX(widget));

    return _vk_textbox_update(VK_TEXTBOX(widget));
}

static int
_vk_textbox_on_recreate(vk_widget_t *widget)
{
    if(widget->vscroller != NULL)
    {
        VK_WIDGET(widget->vscroller)->surface = widget->canvas;
        vk_widget_recreate(VK_WIDGET(widget->vscroller));
    }

    if(widget->hscroller != NULL)
    {
        VK_WIDGET(widget->hscroller)->surface = widget->canvas;
        vk_widget_recreate(VK_WIDGET(widget->hscroller));
    }

    return _vk_textbox_update(VK_TEXTBOX(widget));
}

static void
_vk_textbox_free_lines(vk_textbox_t *textbox)
{
    int i;

    if(textbox->lines != NULL)
    {
        for(i = 0; i < textbox->line_count; i++)
            free(textbox->lines[i]);

        free(textbox->lines);
        textbox->lines = NULL;
    }

    textbox->line_count = 0;
    textbox->lines_alloc = 0;
}

static void
_vk_textbox_append_line(vk_textbox_t *textbox, const char *str, int len)
{
    if(textbox->line_count >= textbox->lines_alloc)
    {
        int new_alloc = (textbox->lines_alloc == 0) ? 32 :
            textbox->lines_alloc * 2;

        textbox->lines = realloc(textbox->lines,
            new_alloc * sizeof(char *));
        textbox->lines_alloc = new_alloc;
    }

    textbox->lines[textbox->line_count] = strndup(str, len);
    textbox->line_count++;
}

static void
_vk_textbox_reflow(vk_textbox_t *textbox)
{
    vk_widget_t     *widget;
    int             paint_width;
    const char      *p;
    const char      *line_start;
    int             col;

    _vk_textbox_free_lines(textbox);

    if(textbox->text == NULL) return;

    widget = VK_WIDGET(textbox);
    paint_width = widget->width;
    if(widget->vscroller != NULL) paint_width--;

    if(paint_width < 1) return;

    p = textbox->text;
    line_start = p;
    col = 0;

    while(*p != '\0')
    {
        if(*p == '\n')
        {
            _vk_textbox_append_line(textbox, line_start, p - line_start);
            p++;
            line_start = p;
            col = 0;
            continue;
        }

        col++;

        if(col >= paint_width)
        {
            if(textbox->word_wrap)
            {
                const char *brk = p;

                while(brk > line_start && *brk != ' ' && *brk != '\t')
                    brk--;

                if(brk > line_start)
                {
                    _vk_textbox_append_line(textbox,
                        line_start, brk - line_start);
                    p = brk + 1;
                }
                else
                {
                    _vk_textbox_append_line(textbox,
                        line_start, p - line_start + 1);
                    p++;
                }
            }
            else
            {
                _vk_textbox_append_line(textbox,
                    line_start, p - line_start + 1);
                p++;
            }

            line_start = p;
            col = 0;
            continue;
        }

        p++;
    }

    if(p > line_start || (p > textbox->text && *(p - 1) == '\n'))
    {
        _vk_textbox_append_line(textbox, line_start, p - line_start);
    }
}
