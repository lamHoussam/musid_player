#define MYLIB_EXPORTS

#include "lh_crenderer.h"


bool console_init(ConsoleRenderer* r) {
    r->hBuffer = CreateConsoleScreenBuffer(
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        CONSOLE_TEXTMODE_BUFFER,
        nullptr
    );

    if (r->hBuffer == INVALID_HANDLE_VALUE)
        return false;

    COORD size = { (SHORT)WIDTH, (SHORT)HEIGHT };
    SetConsoleScreenBufferSize(r->hBuffer, size);

    SMALL_RECT rect = { 0, 0, (SHORT)(WIDTH - 1), (SHORT)(HEIGHT - 1) };
    SetConsoleWindowInfo(r->hBuffer, TRUE, &rect);

    SetConsoleActiveScreenBuffer(r->hBuffer);

    CONSOLE_CURSOR_INFO ci;
    GetConsoleCursorInfo(r->hBuffer, &ci);
    ci.bVisible = FALSE;
    SetConsoleCursorInfo(r->hBuffer, &ci);

    return true;
}

void console_clear(ConsoleRenderer* console, wchar_t ch, WORD attr) {
    for (int i = 0; i < WIDTH*HEIGHT; ++i) {
        console->backBuffer[i].ch   = ch;
        console->backBuffer[i].attr = attr;
    }
}

void _console_put(ConsoleRenderer* console, int x, int y, wchar_t ch, WORD attr) {
    int idx = y * WIDTH + x;
    console->backBuffer[idx].ch   = ch;
    console->backBuffer[idx].attr = attr;
}

void console_put(ConsoleRenderer* console, int x, int y, wchar_t ch, WORD attr) {
    if (x < 0 || y < 0 || x >= WIDTH || y >= HEIGHT)
        return;
    _console_put(console, x, y, ch, attr);
}

void render_bounds(ConsoleRenderer* console) {
    for (int x = 0; x < WIDTH; ++x) {
        _console_put(console, x, HEIGHT - 1, '-', FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    }

    for (int y = 0; y < HEIGHT; ++y) {
        _console_put(console, WIDTH - 1, y, '|', FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    }
}

void console_render(ConsoleRenderer* console) {
    int count = WIDTH*HEIGHT;

    for (int i = 0; i < count; ++i) {
        console->winBuffer[i].Char.UnicodeChar = console->backBuffer[i].ch;
        console->winBuffer[i].Attributes       = console->backBuffer[i].attr;
    }

    SMALL_RECT rect = {
        0, 0,
        (SHORT)(WIDTH  - 1),
        (SHORT)(HEIGHT - 1)
    };

    COORD size  = { (SHORT)WIDTH, (SHORT)HEIGHT };
    COORD zero  = { 0, 0 };

    WriteConsoleOutputW(
        console->hBuffer,
        console->winBuffer,
        size,
        zero,
        &rect
    );
}

void console_draw_rectangle(ConsoleRenderer* console, const ConsoleRect rc) {
    if (rc.left < 0 || rc.top < 0 || rc.right >= WIDTH || rc.bottom >= HEIGHT)
        return;

    for (int x = rc.left; x < rc.right; ++x) {
        _console_put(console, x, rc.top, '\u2500', FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        _console_put(console, x, rc.bottom, '\u2500', FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    }

    for (int y = rc.top; y < rc.bottom; ++y) {
        _console_put(console, rc.left, y, '\u2502', FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        _console_put(console, rc.right, y, '\u2502', FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    }
}

void console_write_text(ConsoleRenderer* console, int x, int y, const wchar_t* ch, int size) {
    if (x < 0 || y < 0 || x + size >= WIDTH || y >= HEIGHT)
        return;
    for (int i = 0; i < size; ++i) {
        _console_put(console, x+i, y, ch[i], FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    }
}

