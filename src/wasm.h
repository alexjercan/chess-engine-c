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

extern int js_format(char *buffer, const char *format, void *args);
extern void js_log_cstr(const char *message);

extern void js_fill_piece(int x, int y, int w, int h, char piece);

extern void js_canvas_hover_px(point_t *point);
extern int js_canvas_clicked();

#endif // WASM_H

/*
#ifdef DS_SB_IMPLEMENTATION
// Append a formatted string to the string builder using wasm js functions
//
// Returns 0 if the string was appended successfully.
DSHDEF ds_result ds_string_builder_append(ds_string_builder *sb,
                                          const char *format, ...) {
    ds_result result = DS_OK;

    va_list args;
    va_start(args, format);
    int needed = vsnprintf(NULL, 0, format, args);
    va_end(args);

    char *buffer = DS_MALLOC(sb->items.allocator, needed + 1);
    if (buffer == NULL) {
        DS_LOG_ERROR("Failed to allocate string");
        return_defer(DS_ERR);
    }

    va_start(args, format);
    vsnprintf(buffer, needed + 1, format, args);
    va_end(args);

    if (ds_dynamic_array_append_many(&sb->items, (void **)buffer, needed) !=
        DS_OK) {
        return_defer(DS_ERR);
    }

defer:
    if (buffer != NULL) {
        DS_FREE(sb->items.allocator, buffer);
    }
    return result;
}
#endif
*/
