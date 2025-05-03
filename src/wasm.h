#ifndef WASM_H
#define WASM_H

typedef struct ivec2 {
    int x, y;
} ivec2;

extern int js_width();
extern int js_height();
extern int js_random(int a, int b);

extern void js_clear_canvas();
extern void js_fill_rect(int x, int y, int w, int h, int color);
extern void js_fill_circle(int x, int y, int r, int color);
extern void js_draw_outline(int x, int y, int w, int h, int color);
extern void js_draw_texture(int x, int y, int w, int h, const char *path);

extern int js_format(char *buffer, const char *format, ...);
extern void js_log_cstr(const char *format, ...);

extern void js_hover_px(ivec2 *point);
extern int js_mouse_down();
extern int js_mouse_up();

#endif // WASM_H

#ifdef WASM_IMPLEMENTATION

#define DS_NO_STDLIB
#define DS_LIST_ALLOCATOR_IMPLEMENTATION
#define DS_DA_INIT_CAPACITY 8
#define DS_DA_IMPLEMENTATION
#define DS_SB_IMPLEMENTATION
#define DS_NO_TERMINAL_COLORS
#include "ds.h"

#define ds_string_builder_append(sb, format, ...)                              \
    do {                                                                       \
        int needed = js_format(NULL, format, __VA_ARGS__);                     \
        char *buffer = DS_MALLOC((sb)->items.allocator, needed + 1);           \
        js_format(buffer, format, __VA_ARGS__);                                \
        buffer[needed] = '\0';                                                 \
        ds_dynamic_array_append_many(&(sb)->items, (void **)buffer, needed);   \
        DS_FREE((sb)->items.allocator, buffer);                                \
    } while (0)

#endif // WASM_IMPLEMENTATION
