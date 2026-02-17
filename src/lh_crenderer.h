#ifndef LH_CONSOLE_RENDERER
#define LH_CONSOLE_RENDERER

#include <windows.h>
#include <stdio.h>

#define HEIGHT  50
#define WIDTH   200
#define LH_KEYS_COUNT   0x1 << 16

#ifdef __cplusplus
extern "C" {
#endif

#ifdef MYLIB_EXPORTS
#define MYLIB_API __declspec(dllexport)
#else
#define MYLIB_API __declspec(dllimport)
#endif

struct Cell {
    wchar_t ch;
    WORD attr;
};

struct ConsoleRect {
    int top;
    int bottom;
    int left;
    int right;
};

struct InputState {
    int         keys[LH_KEYS_COUNT];
    int         mouseX, mouseY;
    int         mouseDX, mouseDY;
};

struct ConsoleRenderer {
    Cell        backBuffer[HEIGHT*WIDTH];
    CHAR_INFO   winBuffer[HEIGHT*WIDTH];
    InputState  input;
    HANDLE      hBuffer;
    HANDLE      hInput;
};

MYLIB_API bool console_init(ConsoleRenderer* r);
MYLIB_API void console_clear(ConsoleRenderer* console, wchar_t ch, WORD attr);
MYLIB_API void console_put(ConsoleRenderer* console, int x, int y, wchar_t ch, WORD attr);
MYLIB_API void render_bounds(ConsoleRenderer* console);
MYLIB_API void console_render(ConsoleRenderer* console);

MYLIB_API void console_draw_rectangle(ConsoleRenderer* console, const ConsoleRect rc);
MYLIB_API void console_write_text(ConsoleRenderer* console, int x, int y, const wchar_t* ch, int size);
MYLIB_API void console_process_input(ConsoleRenderer* console);

#ifdef __cplusplus
}
#endif


#endif