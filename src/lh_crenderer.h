#ifndef LH_CONSOLE_RENDERER
#define LH_CONSOLE_RENDERER

#include <stdio.h>

#include "types.h"

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


// =============================
// Colors
// =============================
#ifndef FOREGROUND_BLUE
  #define FOREGROUND_BLUE      0x0001 // text color contains blue.
  #define FOREGROUND_GREEN     0x0002 // text color contains green.
  #define FOREGROUND_RED       0x0004 // text color contains red.
  #define FOREGROUND_INTENSITY 0x0008 // text color is intensified.
  #define BACKGROUND_BLUE      0x0010 // background color contains blue.
  #define BACKGROUND_RED       0x0040 // background color contains red.
  #define VK_BACK           0x08
  #define VK_ESCAPE         0x1B
#endif

#define isalpha(x) (((x) <= 'Z' && (x) >= 'A') || ((x) <= 'z' && (x) >= 'a'))

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif



#ifdef __cplusplus
extern "C" {
#endif

#ifdef MYLIB_EXPORTS
#define MYLIB_API __declspec(dllexport)
#else
#define MYLIB_API __declspec(dllimport)
#endif

struct ConsoleRect {
    i32 top;
    i32 bottom;
    i32 left;
    i32 right;
};

struct InputState {
    i32         keys[LH_KEYS_COUNT];
    i32         mouseX, mouseY;
    i32         mouseDX, mouseDY;
};

struct ConsoleRenderer;

MYLIB_API bool console_init(ConsoleRenderer** r);
MYLIB_API void console_clear(ConsoleRenderer* console, wchar_t ch, u16 attr);
MYLIB_API void console_put(ConsoleRenderer* console, i32 x, i32 y, wchar_t ch, u16 attr);
MYLIB_API void render_bounds(ConsoleRenderer* console);
MYLIB_API void console_render(ConsoleRenderer* console);

MYLIB_API void console_draw_rectangle(ConsoleRenderer* console, const ConsoleRect rc);
MYLIB_API void console_write_text(ConsoleRenderer* console, i32 x, i32 y, const wchar_t* ch, i32 size);
MYLIB_API void console_process_input(ConsoleRenderer* console);

MYLIB_API InputState* console_get_input_state(ConsoleRenderer* console);

MYLIB_API b8    input_get_key(const InputState* renderer, const i32 key);
// @NOTE: Bad
MYLIB_API void  input_invalidate_key(InputState* renderer, const i32 key);




#ifdef __cplusplus
}
#endif


#endif