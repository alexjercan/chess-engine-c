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
extern void js_fill_piece(int x, int y, int w, int h, char piece);
extern void js_log_cstr(const char *message);
extern void js_canvas_hover_px(point_t *point);
extern int js_canvas_clicked();

#endif // WASM_H
