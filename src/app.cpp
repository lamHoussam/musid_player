#include "app.h"

void draw_header(ConsoleRenderer* c) {
    ConsoleRect rc = {HEADER_TOP, HEADER_BOTTOM, 0, WIDTH - 1};
    console_draw_rectangle(c, rc);
    console_write_text(c, WIDTH/2 - 6, 2, L"TERMINAL PLAYER", 16);
}


void draw_playlist(app* App) {
    ConsoleRect rc = {BODY_TOP, BODY_BOTTOM, PLAYLIST_LEFT, PLAYLIST_RIGHT};
    console_draw_rectangle(&App->Renderer, rc);

    wchar_t TitleBuffer[64];
    swprintf(TitleBuffer, 64, L"PLAYLIST %lld", App->AudioEngine.SongsCount);
    console_write_text(&App->Renderer, 2, BODY_TOP, TitleBuffer, wcslen(TitleBuffer));

    i32 y = BODY_TOP + 2;

    // @NOTE: Check
    for (i32 i = 0; i < App->AudioEngine.SongsCount; i++) {
        WORD attr = (i == App->AudioEngine.CurrentSongIndex) ? COLOR_SELECTED : COLOR_DEFAULT;

        wchar_t buffer[64];
        swprintf(buffer, 64, L"%02d. %s", i+1, App->AudioEngine.Songs[i].SongName);

        console_write_text(&App->Renderer, 2, y++, buffer, wcslen(buffer));
    }
}

void draw_now_playing(app* App) {
    ConsoleRect rc = {BODY_TOP, BODY_BOTTOM, INFO_LEFT, INFO_RIGHT};
    console_draw_rectangle(&App->Renderer, rc);

    console_write_text(&App->Renderer, INFO_LEFT + 2, BODY_TOP, L" NOW PLAYING ", 12);

    int x = INFO_LEFT + 4;
    int y = BODY_TOP + 3;

    if (App->AudioEngine.SongsCount > 0) {
        song_data SongData = App->AudioEngine.Songs[App->AudioEngine.CurrentSongIndex];

        console_write_text(&App->Renderer, x, y, SongData.SongName, wcslen(SongData.SongName));
        console_write_text(&App->Renderer, x, y + 2, SongData.Artist, wcslen(SongData.Artist));
        console_write_text(&App->Renderer, x, y + 4, SongData.Album, wcslen(SongData.Album));
    }
    // console_write_text(&App->Renderer, x, y, L"Title : Interstellar Theme", 27);
    // console_write_text(&App->Renderer, x, y + 2, L"Artist: Hans Zimmer", 20);
    // console_write_text(&App->Renderer, x, y + 4, L"Album : Interstellar OST", 25);
    // console_write_text(c, x, y + 6, L"Year  : 2014", 13);
}

void draw_progress(ConsoleRenderer* c, float percent) {
    ConsoleRect rc = {PROGRESS_TOP, PROGRESS_BOTTOM, 0, WIDTH - 1};
    console_draw_rectangle(c, rc);

    int barWidth = WIDTH - 10;
    int filled = (int)(barWidth * percent);

    int y = PROGRESS_TOP + 2;
    int x = 5;

    for (int i = 0; i < barWidth; i++) {
        wchar_t ch = (i < filled) ? L'█' : L'░';
        console_put(c, x + i, y, ch, COLOR_ACCENT);
    }

    wchar_t timeText[32];
    swprintf(timeText, 32, L"01:23 / 04:56");

    console_write_text(c, WIDTH - 20, y, timeText, wcslen(timeText));
}

void draw_footer(ConsoleRenderer* c) {
    ConsoleRect rc = {FOOTER_TOP, FOOTER_BOTTOM, 0, WIDTH - 1};
    console_draw_rectangle(c, rc);

    console_write_text(c, 5, FOOTER_TOP + 2,
        L"[SPACE] Play/Pause  [N] Next  [P] Prev  [S] Stop  [Q] Quit",
        65);
}

u8 app_init(app* App) {
    console_init(&App->Renderer);
    console_clear(&App->Renderer, L' ', 0);

    AudioEngineInit(&App->AudioEngine);
    AudioEngineLoadSongsFromFolder(&App->AudioEngine, L"data");
    return 0;
}

void app_run(app* App) {
    printf("Start of program\n");
    App->IsRunning = true;
    while (App->IsRunning) { app_update(App); }
    printf("End of program\n");
}

void app_update(app* App) {
    ConsoleRenderer* console = &App->Renderer;

    console_process_input(&App->Renderer);
    console_clear(&App->Renderer, L' ', COLOR_DEFAULT);

    draw_header(&App->Renderer);
    draw_playlist(App);
    draw_now_playing(App);
    draw_progress(&App->Renderer, 40);
    draw_footer(&App->Renderer);

    render_bounds(&App->Renderer);
    console_render(&App->Renderer);

    if (App->Renderer.input.keys[L'n']) { AudioEnginePlayNext(&App->AudioEngine); }
    if (App->Renderer.input.keys[L'p']) { AudioEnginePlayPrev(&App->AudioEngine); }
    if (App->Renderer.input.keys[L' ']) { AudioEngineTogglePlayPause(&App->AudioEngine); }
    if (App->Renderer.input.keys[L'q']) { App->IsRunning = false; }
    if (App->Renderer.input.keys[L's']) { AudioEnginePause(&App->AudioEngine); }
}


