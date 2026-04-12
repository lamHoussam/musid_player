#pragma once

#define MYLIB_EXPORTS
#include <windows.h>

#include "lh_crenderer.h"



struct Cell {
    wchar_t ch;
    u16     attr;
};

struct ConsoleRenderer {
    Cell        backBuffer[HEIGHT*WIDTH];
    CHAR_INFO   winBuffer[HEIGHT*WIDTH];
    InputState  input;
    HANDLE      hBuffer;
    HANDLE      hInput;
};


void input_state_init(InputState* input) {
    memset(input->keys, 0, sizeof(input->keys));
}

bool console_init(ConsoleRenderer** r) {
    *r = new ConsoleRenderer();

    (*r)->hBuffer = CreateConsoleScreenBuffer(
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        CONSOLE_TEXTMODE_BUFFER,
        nullptr
    );

    if ((*r)->hBuffer == INVALID_HANDLE_VALUE)
        return false;

    (*r)->hInput = GetStdHandle(STD_INPUT_HANDLE);

    DWORD mode = 0;
    GetConsoleMode((*r)->hInput, &mode);

    mode |= ENABLE_EXTENDED_FLAGS;
    mode |= ENABLE_WINDOW_INPUT;
    mode |= ENABLE_MOUSE_INPUT;
    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;

    SetConsoleMode((*r)->hInput, mode);
    COORD size = { (SHORT)WIDTH, (SHORT)HEIGHT };

    SetConsoleScreenBufferSize((*r)->hBuffer, size);

    SMALL_RECT rect = { 0, 0, (SHORT)(WIDTH - 1), (SHORT)(HEIGHT - 1) };
    SetConsoleWindowInfo((*r)->hBuffer, TRUE, &rect);

    SetConsoleActiveScreenBuffer((*r)->hBuffer);

    CONSOLE_CURSOR_INFO ci;
    GetConsoleCursorInfo((*r)->hBuffer, &ci);
    ci.bVisible = FALSE;
    SetConsoleCursorInfo((*r)->hBuffer, &ci);

    return true;
}

// @NOTE: Rewrite
void console_process_input(ConsoleRenderer* r) {
    input_state_init(&r->input);
    DWORD numEvents = 0;
    GetNumberOfConsoleInputEvents(r->hInput, &numEvents);

    if (numEvents > 0)
    {
        INPUT_RECORD record;
        DWORD numRead;

        PeekConsoleInput(r->hInput, &record, 1, &numRead);

        if (numRead > 0)
        {
            ReadConsoleInput(r->hInput, &record, 1, &numRead);
            if (record.EventType == KEY_EVENT) {
                KEY_EVENT_RECORD key = record.Event.KeyEvent;

                if (key.bKeyDown) {
                    switch (key.wVirtualKeyCode) {
                        case VK_UP:    // handle up
                        case VK_DOWN:  // handle down
                        case VK_LEFT:
                        case VK_RIGHT:
                        case VK_ESCAPE:
                            break;
                    }

                    wchar_t ch = key.uChar.UnicodeChar;
                    r->input.keys[ch] = 1;
                }
            } else if (record.EventType == MOUSE_EVENT) {
                MOUSE_EVENT_RECORD mouse = record.Event.MouseEvent;

                if (mouse.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) {
                    i32 x = mouse.dwMousePosition.X;
                    i32 y = mouse.dwMousePosition.Y;

                    r->input.mouseDX = r->input.mouseX - x;
                    r->input.mouseDY = r->input.mouseY - y;

                    r->input.mouseX = x;
                    r->input.mouseY = y;
                    // handle click
                }
            }

            // Handle event
        }
    }
}


void console_clear(ConsoleRenderer* console, wchar_t ch, u16 attr) {
    for (i32 i = 0; i < WIDTH*HEIGHT; ++i) {
        console->backBuffer[i].ch   = ch;
        console->backBuffer[i].attr = attr;
    }
}

void _console_put(ConsoleRenderer* console, i32 x, i32 y, wchar_t ch, u16 attr) {
    i32 idx = y * WIDTH + x;
    console->backBuffer[idx].ch   = ch;
    console->backBuffer[idx].attr = attr;
}

void console_put(ConsoleRenderer* console, i32 x, i32 y, wchar_t ch, u16 attr) {
    if (x < 0 || y < 0 || x >= WIDTH || y >= HEIGHT)
        return;
    _console_put(console, x, y, ch, attr);
}

void render_bounds(ConsoleRenderer* console) {
    for (i32 x = 0; x < WIDTH; ++x) {
        _console_put(console, x, HEIGHT - 1, '-', FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    }

    for (i32 y = 0; y < HEIGHT; ++y) {
        _console_put(console, WIDTH - 1, y, '|', FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    }
}

void console_render(ConsoleRenderer* console) {
    i32 count = WIDTH*HEIGHT;

    for (i32 i = 0; i < count; ++i) {
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

    for (i32 x = rc.left; x < rc.right; ++x) {
        _console_put(console, x, rc.top, L'\u2500', FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        _console_put(console, x, rc.bottom, L'\u2500', FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    }

    for (i32 y = rc.top; y < rc.bottom; ++y) {
        _console_put(console, rc.left, y, L'\u2502', FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        _console_put(console, rc.right, y, L'\u2502', FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    }
}

void console_write_text(ConsoleRenderer* console, i32 x, i32 y, const wchar_t* ch, i32 size) {
    if (x < 0 || y < 0 || x + size >= WIDTH || y >= HEIGHT)
        return;
    for (i32 i = 0; i < size; ++i) {
        _console_put(console, x+i, y, ch[i], FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    }
}


InputState* console_get_input_state(ConsoleRenderer* console) { return &console->input; }

b8 input_get_key(const InputState* input, const i32 key) { return input->keys[key]; }

void input_invalidate_key(InputState* renderer, const i32 key) { renderer->keys[key] = false; }

