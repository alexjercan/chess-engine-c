#ifndef WASM_H
#define WASM_H

typedef struct point_t {
    int x, y;
} point_t;

extern int js_width();
extern int js_height();
extern int js_random(int a, int b);

extern void js_clear_canvas();
extern void js_fill_rect(int x, int y, int w, int h, int color);
extern void js_draw_outline(int x, int y, int w, int h, int color);

extern int js_format(char *buffer, const char *format, ...);
extern void js_log_cstr(const char *message);

extern void js_fill_piece(int x, int y, int w, int h, char piece);

extern void js_canvas_hover_px(point_t *point);
extern int js_canvas_clicked();

#endif // WASM_H

#ifdef DS_SB_IMPLEMENTATION
#define ds_string_builder_append(sb, format, ...)                              \
    do {                                                                       \
        int needed = js_format(NULL, format, __VA_ARGS__);                     \
        char *buffer = DS_MALLOC((sb)->items.allocator, needed + 1);           \
        js_format(buffer, format, __VA_ARGS__);                                \
        buffer[needed] = '\0';                                                 \
        ds_dynamic_array_append_many(&(sb)->items, (void **)buffer, needed);   \
        DS_FREE((sb)->items.allocator, buffer);                                \
    } while (0)
#endif
