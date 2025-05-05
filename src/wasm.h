#ifndef WASM_H
#define WASM_H

typedef struct ivec2 {
    int x, y;
} ivec2;

#define CLITERAL(type)      (type)

#define WHITE      CLITERAL(Color){ 255, 255, 255, 255 }
#define RAYWHITE   CLITERAL(Color){ 245, 245, 245, 255 }

// Mouse buttons
typedef enum {
    MOUSE_BUTTON_LEFT    = 0,       // Mouse button left
    MOUSE_BUTTON_RIGHT   = 1,       // Mouse button right
    MOUSE_BUTTON_MIDDLE  = 2,       // Mouse button middle (pressed wheel)
    MOUSE_BUTTON_SIDE    = 3,       // Mouse button side (advanced mouse device)
    MOUSE_BUTTON_EXTRA   = 4,       // Mouse button extra (advanced mouse device)
    MOUSE_BUTTON_FORWARD = 5,       // Mouse button forward (advanced mouse device)
    MOUSE_BUTTON_BACK    = 6,       // Mouse button back (advanced mouse device)
} MouseButton;

// Vector2, 2 components
typedef struct Vector2 {
    float x;                // Vector x component
    float y;                // Vector y component
} Vector2;

// Texture, tex data stored in GPU memory (VRAM)
typedef struct Texture {
    unsigned int id;        // OpenGL texture id
    int width;              // Texture base width
    int height;             // Texture base height
    int mipmaps;            // Mipmap levels, 1 by default
    int format;             // Data format (PixelFormat type)
} Texture;

// Texture2D, same as Texture
typedef Texture Texture2D;

// Color, 4 components, R8G8B8A8 (32bit)
typedef struct Color {
    unsigned char r;        // Color red value
    unsigned char g;        // Color green value
    unsigned char b;        // Color blue value
    unsigned char a;        // Color alpha value
} Color;

extern void InitWindow(int width, int height, const char *title);

extern void ClearBackground(Color color);
extern void BeginDrawing(void);
extern void EndDrawing(void);

extern void DrawRectangle(int posX, int posY, int width, int height, Color color);
extern void DrawCircle(int centerX, int centerY, float radius, Color color);

extern void LoadTextureInternal(const char *fileName, int *id, int *width, int *height, int *mipmaps, int *format);
#define LoadTexture(fileName)                                                  \
    ({                                                                         \
        int __id = 0, __width = 0, __height = 0, __mipmaps = 0, __format = 0;  \
        LoadTextureInternal(fileName, &__id, &__width, &__height, &__mipmaps, &__format); \
        CLITERAL(Texture2D){ __id, __width, __height, __mipmaps, __format }; \
    })

extern void DrawTextureEx(Texture2D texture, Vector2 position, float rotation, float scale, Color tint);

extern int IsMouseButtonPressed(int button);
extern void GetMousePositionInternal(int *x, int *y);
#define GetMousePosition()                                                     \
    ({                                                                         \
        int __x = 0, __y = 0;                                                  \
        GetMousePositionInternal(&__x, &__y);                                  \
        CLITERAL(Vector2){ __x, __y };                                         \
    })

extern int StringFormat(char *buffer, const char *format, ...);
extern void ConsoleLog(const char *format, ...);

#undef DS_LOG_INFO
#define DS_LOG_INFO(format, ...) ConsoleLog(format, ##__VA_ARGS__)

#endif // WASM_H
