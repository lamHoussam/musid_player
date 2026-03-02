#ifndef LH_CONSOLE_RENDERER
#define LH_CONSOLE_RENDERER

#include <windows.h>
#include <stdio.h>

#define HEIGHT  50
#define WIDTH   200
#define LH_KEYS_COUNT   0x1 << 16


// =============================
// Box Drawing
// =============================

#define UI_HLINE        L'\u2500' // ─
#define UI_VLINE        L'\u2502' // │

#define UI_CORNER_TL    L'\u250C' // ┌
#define UI_CORNER_TR    L'\u2510' // ┐
#define UI_CORNER_BL    L'\u2514' // └
#define UI_CORNER_BR    L'\u2518' // ┘

// Optional intersections (useful later)
#define UI_T_DOWN       L'\u252C' // ┬
#define UI_T_UP         L'\u2534' // ┴
#define UI_T_RIGHT      L'\u251C' // ├
#define UI_T_LEFT       L'\u2524' // ┤
#define UI_CROSS        L'\u253C' // ┼


// =============================
// Block / Progress Elements
// =============================

#define UI_BLOCK_FULL   L'\u2588' // █
#define UI_BLOCK_DARK   L'\u2593' // ▓
#define UI_BLOCK_MED    L'\u2592' // ▒
#define UI_BLOCK_LIGHT  L'\u2591' // ░


// =============================
// Icons / Symbols
// =============================

#define UI_HEART        L'\u2665' // '♥'
#define UI_HEART_STR    L"\u2665" // "♥"


#define UI_PLAY         L'\u25B6' // ▶
#define UI_TRI_RIGHT    L'\u25B8' // ▸ (alternative small arrow)


// =============================
// Simple Arrow Symbols (Safe)
// =============================

#define UI_ARROW_LEFT   L'\u2190' // ←
#define UI_ARROW_RIGHT  L'\u2192' // →
#define UI_ARROW_UP     L'\u2191' // ↑
#define UI_ARROW_DOWN   L'\u2193' // ↓

// =============================
// Media Control Symbols
// =============================

#define UI_PREVIOUS   L'\u23EE' // ⏮  (BLACK LEFT-POINTING DOUBLE TRIANGLE WITH VERTICAL BAR)
#define UI_PLAY_PAUSE L'\u23EF' // ⏯  (BLACK RIGHT-POINTING TRIANGLE WITH DOUBLE VERTICAL BAR)
#define UI_NEXT       L'\u23ED' // ⏭  (BLACK RIGHT-POINTING DOUBLE TRIANGLE WITH VERTICAL BAR)


// =============================
// Loop / Rotation (Safer than emoji)
// =============================

#define UI_REPEAT_CW    L'\u21BB' // ↻
#define UI_REPEAT_CCW   L'\u21BA' // ↺


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