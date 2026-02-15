#ifndef LH_CONSOLE_RENDERER
#define LH_CONSOLE_RENDERER

#include <windows.h>

#define HEIGHT  50
#define WIDTH   200

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

struct ConsoleRenderer {
    HANDLE hBuffer;
    Cell backBuffer[HEIGHT*WIDTH];
    CHAR_INFO winBuffer[HEIGHT*WIDTH];
};

MYLIB_API bool console_init(ConsoleRenderer* r);
MYLIB_API void console_clear(ConsoleRenderer* console, wchar_t ch, WORD attr);
MYLIB_API void console_put(ConsoleRenderer* console, int x, int y, wchar_t ch, WORD attr);
MYLIB_API void render_bounds(ConsoleRenderer* console);
MYLIB_API void console_render(ConsoleRenderer* console);

MYLIB_API void console_draw_rectangle(ConsoleRenderer* console, const ConsoleRect rc);
MYLIB_API void console_write_text(ConsoleRenderer* console, int x, int y, const wchar_t* ch, int size);

#ifdef __cplusplus
}
#endif


#endif