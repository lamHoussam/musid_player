#include "app.h"

#define SONGS_FOLDER L"data"














// @NOTE: Start
#define UI_BG              0
#define UI_TEXT            (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)
#define UI_HEADER          (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY)
#define UI_ACCENT          (FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY)
#define UI_PLAYING         (FOREGROUND_GREEN | FOREGROUND_INTENSITY)
#define UI_SELECTED_BG     (BACKGROUND_BLUE)
#define UI_SELECTED        (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | BACKGROUND_BLUE)
#define UI_DIM             (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)

// ------------------------------------------------------------

static void draw_hline(ConsoleRenderer* r, i32 y, i32 x0, i32 x1, WORD attr)
{
    for (i32 x = x0; x <= x1; ++x) { console_put(r, x, y, UI_HLINE, attr); }
}

static void draw_vline(ConsoleRenderer* r, i32 x, i32 y0, i32 y1, WORD attr)
{
    for (i32 y = y0; y <= y1; ++y) { console_put(r, x, y, UI_VLINE, attr); }
}

static void draw_box(ConsoleRenderer* r, i32 left, i32 top, i32 right, i32 bottom, WORD attr) {
    console_put(r, left,  top,    UI_CORNER_TL, attr);
    console_put(r, right, top,    UI_CORNER_TR, attr);
    console_put(r, left,  bottom, UI_CORNER_BL, attr);
    console_put(r, right, bottom, UI_CORNER_BR, attr);

    draw_hline(r, top,    left + 1, right - 1, attr);
    draw_hline(r, bottom, left + 1, right - 1, attr);
    draw_vline(r, left,   top + 1,  bottom - 1, attr);
    draw_vline(r, right,  top + 1,  bottom - 1, attr);
}

static void write_padded(ConsoleRenderer* r, i32 x, i32 y, const wchar_t* text, i32 width, WORD attr) {
    i32 len = (int)wcslen(text);
    i32 copy = len > width ? width : len;

    for (i32 i = 0; i < copy; ++i) { console_put(r, x + i, y, text[i], attr); }
    for (i32 i = copy; i < width; ++i) { console_put(r, x + i, y, L' ', attr); }
}


void render_miniplayer(app* App, i32 BottomTop, i32 BottomBottom) {
    draw_box(&App->Renderer, 0, BottomTop, WIDTH - 1, BottomBottom, UI_ACCENT);

    u64 SamplesPlayed = 0;
    if (App->AudioEngine.xAudio2SourceVoice != nullptr) {
        XAUDIO2_VOICE_STATE State;
        App->AudioEngine.xAudio2SourceVoice->GetState(&State);
        SamplesPlayed = State.SamplesPlayed;
    }

    song_data* CurrentSong  = App->AudioEngine.Songs+App->AudioEngine.CurrentSongIndex;
    i32 DurationInSec       = GetSongBufferDurationInSec(&CurrentSong->SongBufferData);
    u64 CurrentTimeInSec    = SamplesPlayed / CurrentSong->SongBufferData.SamplesPerSec;
    i32 CurrentFloorMinDuration = CurrentTimeInSec / 60;
    i32 CurrentRemainingSecs    = CurrentTimeInSec % 60;
    i32 FloorMinDuration        = DurationInSec / 60;
    i32 RemainingSecs           = DurationInSec % 60;

    wchar_t TimeText[32];
    swprintf(TimeText, 32, L"%02d:%02d", CurrentFloorMinDuration, CurrentRemainingSecs);

    write_padded(&App->Renderer, 2, BottomTop + 1,
        TimeText,
        6,
        UI_TEXT);

    // Progress bar
    i32 barStart = 10;
    i32 barWidth = 60;

    i32 filled = (i32)((f32)barWidth * (1.0f*CurrentTimeInSec/DurationInSec));
    for (i32 i = 0; i < barWidth; ++i) {
        wchar_t ch = (i < filled) ? UI_BLOCK_FULL : UI_BLOCK_LIGHT;
        console_put(&App->Renderer, barStart + i, BottomTop + 1, ch, UI_ACCENT);
    }

    swprintf(TimeText, 32, L"%02d:%02d", FloorMinDuration, RemainingSecs);
    write_padded(&App->Renderer, barStart + barWidth + 2, BottomTop + 1,
        TimeText,
        6,
        UI_TEXT);


}


// ------------------------------------------------------------
// LAYOUT 1: Library
// ------------------------------------------------------------
void RenderLayout_Library(app* App) {

    ConsoleRenderer* r = &App->Renderer;
    audio_engine* AudioEngine = &App->AudioEngine;

    console_clear(r, L' ', UI_BG);

    // ========================================================
    // TOP BAR (0–2)
    // ========================================================

    draw_box(r, 0, 0, WIDTH - 1, 2, UI_ACCENT);

    wchar_t LibraryTitle[40];
    swprintf(LibraryTitle, 40, L"LIBRARY - %lld Tracks", AudioEngine->SongsCount);

    write_padded(r, 2, 1,
        LibraryTitle,
        40,
        UI_HEADER);

    wchar_t SearchUI[28];
    swprintf(SearchUI, 28, L"Search: [%s]", App->UIState.SearchString);

    write_padded(r, WIDTH - 60, 1,
        SearchUI,
        28,
        UI_TEXT);

    // ========================================================
    // TABLE AREA (3–45)
    // ========================================================

    const i32 tableTop = 3;
    const i32 tableBottom = 45;

    const i32 MaxSongsToRenderCount = tableBottom - tableTop - 3;
    const i32 SongsToRenderCount    = min(App->UIState.MetadataCount - App->UIState.UIVisualStartIndex, MaxSongsToRenderCount);

    draw_box(r, 0, tableTop, WIDTH - 1, tableBottom, UI_ACCENT);

    // Column positions
    const i32 colNum    = 2;
    const i32 colTitle  = 8;
    const i32 colArtist = 70;
    const i32 colAlbum  = 110;
    const i32 colTime   = 145;
    const i32 colFav    = 155;
    const i32 colPlays  = 160;

    i32 headerY = tableTop + 1;

    write_padded(r, colNum,    headerY, L"#",           4,  UI_HEADER);
    write_padded(r, colTitle,  headerY, L"Title",       60, UI_HEADER);
    write_padded(r, colArtist, headerY, L"Artist",      35, UI_HEADER);
    write_padded(r, colAlbum,  headerY, L"Album",       30, UI_HEADER);
    write_padded(r, colTime,   headerY, L"Time",        6,  UI_HEADER);
    write_padded(r, colFav,    headerY, UI_HEART_STR,   2,  UI_HEADER);
    // write_padded(r, colPlays,  headerY, L"Plays",       6,  UI_HEADER);

    draw_hline(r, headerY + 1, 1, WIDTH - 2, UI_ACCENT);

    // ========================================================
    // SAMPLE DATA ROWS
    // ========================================================

    i32 startY = headerY + 2;
    for (i32 i = 0; i < SongsToRenderCount; ++i) {
        i32 y = startY + i;

        i32 SongIndex = App->UIState.UIVisualStartIndex+i;

        WORD attr = UI_TEXT;

        b32 isPlaying = (SongIndex == AudioEngine->CurrentSongIndex);
        b32 isSelected = (SongIndex == App->UIState.UICurrentSelectedSongIndex);

        if (isPlaying) { attr = UI_PLAYING; }
        if (isSelected) { attr = UI_SELECTED; }

        wchar_t num[8];
        if (isPlaying) { swprintf(num, 9, L"%lc%03d", UI_PLAY, SongIndex + 1); } 
        else { swprintf(num, 9, L"%03d", SongIndex + 1); }

        // @NOTE: Maybe calculate once
        song_metadata* SongData = App->UIState.MetadataToRenderBuffer+SongIndex;

        i32 FloorMinDuration    = SongData->DurationInSec / 60;
        i32 RemainingSecs       = SongData->DurationInSec % 60;

        wchar_t TimeText[32];
        swprintf(TimeText, 32, L"%02d:%02d", FloorMinDuration, RemainingSecs);

        write_padded(r, colNum,    y, num, 4,  attr);
        write_padded(r, colTitle,  y, SongData->SongName, 60, attr);
        write_padded(r, colArtist, y, SongData->Artist, 35, attr);
        write_padded(r, colAlbum,  y, SongData->Album, 30, attr);
        write_padded(r, colTime,   y, TimeText, 6, attr);
        write_padded(r, colFav,    y, (i % 2 == 0) ? UI_HEART_STR : L"", 2, attr);
        // write_padded(r, colPlays,  y, L"213",                               6,  attr);
    }

    // ========================================================
    // HINTS
    // ========================================================

    write_padded(r, colPlays + 5, headerY + 2,  L"[j]       Down", 16, UI_ACCENT);
    write_padded(r, colPlays + 5, headerY + 3,  L"[k]       Up", 14, UI_ACCENT);
    write_padded(r, colPlays + 5, headerY + 4,  L"[SPACE]   Play/Pause", 22, UI_ACCENT);
    write_padded(r, colPlays + 5, headerY + 5,  L"[/]       Search Mode", 23, UI_ACCENT);
    write_padded(r, colPlays + 5, headerY + 6,  L"[Esc]     Normal Mode", 23, UI_ACCENT);
    write_padded(r, colPlays + 5, headerY + 7,  L"[n]       Next", 16, UI_ACCENT);
    write_padded(r, colPlays + 5, headerY + 8,  L"[p]       Previous", 20, UI_ACCENT);
    write_padded(r, colPlays + 5, headerY + 9,  L"[ENTER]   Select", 17, UI_ACCENT);
    write_padded(r, colPlays + 5, headerY + 10, L"[u]       Volume up", 21, UI_ACCENT);
    write_padded(r, colPlays + 5, headerY + 11, L"[d]       Volume down", 23, UI_ACCENT);
    write_padded(r, colPlays + 5, headerY + 12, L"[l]       Loop", 15, UI_ACCENT);
    write_padded(r, colPlays + 5, headerY + 13, L"[a]       Add to playlist", 26, UI_ACCENT);
    write_padded(r, colPlays + 5, headerY + 14, L"[s]       Switch Layout", 24, UI_ACCENT);
    write_padded(r, colPlays + 5, headerY + 15, L"[q]       Quit", 16, UI_ACCENT);

    // ========================================================
    // MINI PLAYER (46–49)
    // ========================================================

    const i32 bottomTop = 46;
    const i32 bottomBottom = HEIGHT - 1;

    render_miniplayer(App, bottomTop, bottomBottom);

    wchar_t ControlText[15];
    swprintf(ControlText, 15, L"%lc  %lc  %lc", UI_PREVIOUS, UI_PLAY_PAUSE, UI_NEXT);

    write_padded(r, 90, bottomTop + 1,
        ControlText,
        15,
        UI_TEXT);

    write_padded(r, 130, bottomTop + 1,
        L"Vol  ",
        6,
        UI_TEXT);

    i32 barStart = 136;
    i32 barWidth = 10;
    for (i32 i = 0; i < barWidth; ++i) {
        wchar_t ch = (i < AudioEngine->CurrentVolume) ? UI_BLOCK_FULL : UI_BLOCK_LIGHT;
        console_put(r, barStart + i, bottomTop + 1, ch, UI_ACCENT);
    }

    wchar_t VolumeText[5];
    swprintf(VolumeText, 5, L"%d%%", AudioEngine->CurrentVolume*10);
    write_padded(r, 148, bottomTop + 1,
        VolumeText,
        5,
        UI_TEXT);



    // UPDATE
    if (App->UIState.UICurrentSelectedSongIndex < App->UIState.UIVisualStartIndex) {
        if (App->UIState.UIVisualStartIndex == 0) { 
            App->UIState.UIVisualStartIndex = AudioEngine->SongsCount - MaxSongsToRenderCount + 1;
        }
        else { App->UIState.UIVisualStartIndex -= 1; }
    } else if (App->UIState.UICurrentSelectedSongIndex >= App->UIState.UIVisualStartIndex + MaxSongsToRenderCount) {
        App->UIState.UIVisualStartIndex += 1;
        App->UIState.UIVisualStartIndex %= AudioEngine->SongsCount;
    }
}


// ------------------------------------------------------------
// LAYOUT 2: Playlist
// ------------------------------------------------------------

void RenderLayout_Playlist(app* App) {
    ConsoleRenderer* r = &App->Renderer;
    playlist* CurrentPlaylist = &App->CurrentPlaylist;

    console_clear(r, L' ', UI_BG);

    // ========================================================
    // TOP BAR (0–2)
    // ========================================================

    draw_box(r, 0, 0, WIDTH - 1, 2, UI_ACCENT);

    wchar_t Title[64];
    swprintf(Title, 64, L"PLAYLIST: %ls (%lld Tracks)", CurrentPlaylist->Name, CurrentPlaylist->SongsCount);

    write_padded(r, 2, 1, Title, 64, UI_HEADER);

    // @NOTE
    // write_padded(r, WIDTH - 50, 1,
    //     L"Shuffle ON | Total 2h 31m",
    //     48,
    //     UI_TEXT);

    // ========================================================
    // MAIN AREA (3–42)
    // ========================================================

    const int mainTop    = 3;
    const int mainBottom = 42;

    draw_box(r, 0, mainTop, WIDTH - 1, mainBottom, UI_ACCENT);

    // Split positions
    const int splitX = 130;

    // Vertical separator
    draw_vline(r, splitX, mainTop + 1, mainBottom - 1, UI_ACCENT);

    // --------------------------------------------------------
    // LEFT: TRACK LIST
    // --------------------------------------------------------

    int listLeft   = 2;
    int listRight  = splitX - 2;
    int listTop    = mainTop + 1;

    write_padded(r, listLeft, listTop,
        L"TRACK LIST",
        20,
        UI_HEADER);



    // @NOTE: Use previous layout code
    int selectedIndex = 1;
    int playingIndex  = 0;

    for (int i = 0; i < CurrentPlaylist->SongsCount; ++i)
    {
        int y = listTop + 2 + i;

        WORD attr = UI_TEXT;

        if (i == playingIndex) { attr = UI_PLAYING; }
        if (i == selectedIndex) { attr = UI_SELECTED; }

        wchar_t line[128];

        song_metadata* SongMetadata = &(App->AudioEngine.Songs+CurrentPlaylist->SongIndexList[i])->SongMetadata;

        if (i == playingIndex) {
            swprintf(line, 128, L"%lc %02d  %ls", UI_PLAY, i + 1, SongMetadata->SongName);
        }
        else {
            swprintf(line, 128, L"  %02d  %ls", i + 1, SongMetadata->SongName);
        }

        write_padded(r, listLeft, y, line, listRight - listLeft, attr);
    }

    // --------------------------------------------------------
    // RIGHT: NOW PLAYING PANEL
    // --------------------------------------------------------

    int panelLeft = splitX + 2;
    int panelTop  = mainTop + 1;

    // @BACK
    // write_padded(r, panelLeft, panelTop,
    //     L"NOW PLAYING",
    //     20,
    //     UI_HEADER);

    // write_padded(r, panelLeft, panelTop + 2,
    //     L"Slipknot",
    //     30,
    //     UI_PLAYING);

    // write_padded(r, panelLeft, panelTop + 3,
    //     L"Vermilion, Pt. 2",
    //     40,
    //     UI_TEXT);

    draw_hline(r, panelTop + 5, panelLeft, WIDTH - 3, UI_DIM);

    // write_padded(r, panelLeft, panelTop + 7,  L"Album: Vol. 3",       40, UI_TEXT);
    // write_padded(r, panelLeft, panelTop + 8,  L"Year: 2004",          40, UI_TEXT);
    // write_padded(r, panelLeft, panelTop + 9,  L"Genre: Metal",        40, UI_TEXT);
    // write_padded(r, panelLeft, panelTop + 10, L"Bitrate: 320kbps",    40, UI_TEXT);
    // write_padded(r, panelLeft, panelTop + 11, L"Sample Rate: 44.1kHz",40, UI_TEXT);
    // write_padded(r, panelLeft, panelTop + 12, L"Plays: 213",          40, UI_TEXT);

    // ========================================================
    // BOTTOM CONTROLS (43–49)
    // ========================================================

    const int bottomTop    = 43;
    const int bottomBottom = HEIGHT - 1;

    draw_box(r, 0, bottomTop, WIDTH - 1, bottomBottom, UI_ACCENT);

    // Large progress bar
    write_padded(r, 2, bottomTop + 1, L"03:12", 6, UI_TEXT);

    int barStart = 10;
    int barWidth = 120;
    int filled   = 60;

    for (int i = 0; i < barWidth; ++i)
    {
        wchar_t ch = (i < filled) ? UI_BLOCK_FULL : UI_BLOCK_LIGHT;
        console_put(r, barStart + i, bottomTop + 1, ch, UI_ACCENT);
    }

    write_padded(r, barStart + barWidth + 2, bottomTop + 1, L"05:44", 6, UI_TEXT);

    // Media controls
    write_padded(r, 60, bottomTop + 3,
        L"⏮   ⏯   ⏭      ↻ Shuffle      ↺ Repeat      Vol ███████░░░ 65%",
        90,
        UI_TEXT);

    console_render(r);
}


// @NOTE: End



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
        swprintf(buffer, 64, L"%02d. %s", i+1, App->AudioEngine.Songs[i].SongMetadata.SongName);

        console_write_text(&App->Renderer, 2, y++, buffer, wcslen(buffer));
    }
}

void draw_now_playing(app* App) {
    ConsoleRect rc = {BODY_TOP, BODY_BOTTOM, INFO_LEFT, INFO_RIGHT};
    console_draw_rectangle(&App->Renderer, rc);

    console_write_text(&App->Renderer, INFO_LEFT + 2, BODY_TOP, L" NOW PLAYING ", 12);

    i32 x = INFO_LEFT + 4;
    i32 y = BODY_TOP + 3;

    if (App->AudioEngine.SongsCount > 0) {
        song_data SongData = App->AudioEngine.Songs[App->AudioEngine.CurrentSongIndex];

        console_write_text(&App->Renderer, x, y, SongData.SongMetadata.SongName, wcslen(SongData.SongMetadata.SongName));
        console_write_text(&App->Renderer, x, y + 2, SongData.SongMetadata.Artist, wcslen(SongData.SongMetadata.Artist));
        console_write_text(&App->Renderer, x, y + 4, SongData.SongMetadata.Album, wcslen(SongData.SongMetadata.Album));
    }
}

void draw_progress(app* App) {
    ConsoleRect rc = {PROGRESS_TOP, PROGRESS_BOTTOM, 0, WIDTH - 1};
    console_draw_rectangle(&App->Renderer, rc);

    u64 SamplesPlayed = 0;
    if (App->AudioEngine.xAudio2SourceVoice != nullptr) {
        XAUDIO2_VOICE_STATE State;
        App->AudioEngine.xAudio2SourceVoice->GetState(&State);
        SamplesPlayed = State.SamplesPlayed;
    }

    i32 barWidth = WIDTH - 40;
    song_data* CurrentSong = App->AudioEngine.Songs+App->AudioEngine.CurrentSongIndex;
    
    u64 CurrentTimeInSec = SamplesPlayed / CurrentSong->SongBufferData.SamplesPerSec;

    i32 y = PROGRESS_TOP + 2;
    i32 x = 5;

    i32 DurationInSec       = GetSongBufferDurationInSec(&CurrentSong->SongBufferData);
    i32 FloorMinDuration    = DurationInSec / 60;
    i32 RemainingSecs       = DurationInSec % 60;

    i32 filled = (i32)((f32)barWidth * (1.0f*CurrentTimeInSec/DurationInSec));
    i32 CurrentFloorMinDuration = CurrentTimeInSec / 60;
    i32 CurrentRemainingSecs    = CurrentTimeInSec % 60;

    for (i32 i = 0; i < barWidth; i++) {
        wchar_t ch = (i < filled) ? L'x' : L' ';
        console_put(&App->Renderer, x + i, y, ch, COLOR_ACCENT);
    }

    wchar_t timeText[32];
    swprintf(timeText, 32, L"%02d:%02d / %02d:%02d", CurrentFloorMinDuration, CurrentRemainingSecs, FloorMinDuration, RemainingSecs);

    console_write_text(&App->Renderer, WIDTH - 20, y, timeText, wcslen(timeText));
}

void draw_footer(ConsoleRenderer* c) {
    ConsoleRect rc = {FOOTER_TOP, FOOTER_BOTTOM, 0, WIDTH - 1};
    console_draw_rectangle(c, rc);

    console_write_text(c, 5, FOOTER_TOP + 2,
        L"[SPACE] Play/Pause  [N] Next  [P] Prev  [S] Stop  [Q] Quit",
        65);
}

void draw_volume(app* App) {
    wchar_t VolumeText[32];

    swprintf(VolumeText, 32, L"%02d / %02d / %02d", 
        AUDIO_ENGINE_MIN_VOLUME, 
        App->AudioEngine.CurrentVolume, 
        AUDIO_ENGINE_MAX_VOLUME);

    i32 y = PROGRESS_TOP + 8;

    console_write_text(&App->Renderer, WIDTH - 20, y, VolumeText, wcslen(VolumeText));
}

void draw_original_layout(app* App) {
    draw_header(&App->Renderer);
    draw_playlist(App);
    draw_now_playing(App);
    draw_progress(App);
    draw_footer(&App->Renderer);
    draw_volume(App);

}

/////////////////////////////////
//  PLAYLIST
/////////////////////////////////
u8 playlist_init(playlist* Playlist, i64 Capacity, const wchar_t* Name) {
    wcsncpy(Playlist->Name, Name, 32);
    Playlist->SongIndexList = (i64*)malloc(sizeof(i64)*Capacity);
    Playlist->Capacity      = Capacity;
    Playlist->SongsCount    = 0;

    return 0;
}

u8 playlist_push(playlist* Playlist, i64 SongIndex) {
    // Allocate more
    if (Playlist->SongsCount >= Playlist->Capacity) { return 1; }
    Playlist->SongIndexList[Playlist->SongsCount++] = SongIndex;
    return 0;
}

u8 playlist_removeAt(playlist* Playlist, i64 Index) {
    if (Playlist->SongsCount == 0) { return 1; }
    Playlist->SongIndexList[Index] = -1;
    for (i64 i = Index + 1; i < Playlist->SongsCount; ++i) {
        Playlist->SongIndexList[i-1] = Playlist->SongIndexList[i];
    }
    Playlist->SongsCount--;
    return 0;
}






u8 app_init(app* App) {
    console_init(&App->Renderer);

    AudioEngineInit(&App->AudioEngine);
    AudioEngineLoadSongsFromFolder(&App->AudioEngine, SONGS_FOLDER);
    if (App->AudioEngine.SongsCount != 0) { AudioEnginePlaySongAtIndex(&App->AudioEngine, 0); }

    App->UIState.UIVisualStartIndex = 0;
    App->UIState.UICurrentSelectedSongIndex = 0;

    App->UIState.MetadataToRenderBuffer = (song_metadata*)malloc(sizeof(song_metadata)*App->AudioEngine.SongsCount);
    App->UIState.MetadataCount          = App->AudioEngine.SongsCount;
    App->UIState.CurrentMode            = UI_MODE_NORMAL;
    App->UIState.CurrentLayout          = UI_LAYOUT_LIBRARY;
    wcsncpy(App->UIState.SearchString, L"                ", 16);
    App->UIState.SearchStringCurrentIndex = 0;

    playlist_init(&App->CurrentPlaylist, App->AudioEngine.SongsCount, L"TestPlaylist");

    playlist_push(&App->CurrentPlaylist, 0);
    playlist_push(&App->CurrentPlaylist, 5);
    playlist_push(&App->CurrentPlaylist, 10);
    playlist_push(&App->CurrentPlaylist, 3);

    for (i64 i = 0; i < App->UIState.MetadataCount; ++i) {
        memcpy(App->UIState.MetadataToRenderBuffer+i, &(App->AudioEngine.Songs+i)->SongMetadata, sizeof(song_metadata));
    }

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

    console_clear(&App->Renderer, L' ', COLOR_DEFAULT);
    console_process_input(&App->Renderer);

    if (App->Renderer.input.keys[VK_ESCAPE]) {
        App->UIState.CurrentMode = UI_MODE_NORMAL;
        App->Renderer.input.keys[VK_ESCAPE] = false;
    }

    if (App->Renderer.input.keys[L'/']) {
        App->UIState.CurrentMode = UI_MODE_SEARCH;
        App->Renderer.input.keys[L'/'] = false;
    }

    switch (App->UIState.CurrentMode)
    {
    case UI_MODE_NORMAL: {

        if (App->Renderer.input.keys[L'n']) { AudioEnginePlayNext(&App->AudioEngine); }
        if (App->Renderer.input.keys[L'p']) { AudioEnginePlayPrev(&App->AudioEngine); }
        if (App->Renderer.input.keys[L' ']) { AudioEngineTogglePlayPause(&App->AudioEngine); }
        if (App->Renderer.input.keys[L'q']) { App->IsRunning = false; }
        if (App->Renderer.input.keys[L's']) {
            App->UIState.CurrentLayout = (ui_layout)((App->UIState.CurrentLayout+1)%2);
        }
        if (App->Renderer.input.keys[L'u']) { App->AudioEngine.CurrentVolume++; }
        if (App->Renderer.input.keys[L'd']) { App->AudioEngine.CurrentVolume--; }
        if (App->Renderer.input.keys[L'l']) { App->AudioEngine.IsLooping = !App->AudioEngine.IsLooping; }
        if (App->Renderer.input.keys[L'a']) { 
            playlist_push(&App->CurrentPlaylist, App->UIState.UICurrentSelectedSongIndex);
        }

        if (App->Renderer.input.keys[L'j']) { 
            App->UIState.UICurrentSelectedSongIndex += 1; 
            App->UIState.UICurrentSelectedSongIndex %= App->AudioEngine.SongsCount;
            App->Renderer.input.keys[L'j'] = false;
        }
        if (App->Renderer.input.keys[L'k']) { 
            App->UIState.UICurrentSelectedSongIndex -= 1; 
            App->UIState.UICurrentSelectedSongIndex %= App->AudioEngine.SongsCount;
            App->Renderer.input.keys[L'k'] = false;
        }
        if (App->Renderer.input.keys[VK_RETURN]) {
            AudioEnginePlaySongAtIndex(&App->AudioEngine, App->UIState.UICurrentSelectedSongIndex);
            App->Renderer.input.keys[VK_RETURN] = false;
        }

    } break;
    case UI_MODE_SEARCH: {
        if (App->UIState.SearchStringCurrentIndex < 16) {
            for (i32 i = 0; i < LH_KEYS_COUNT; ++i) {
                if (isalpha(i) && App->Renderer.input.keys[i]) {
                    App->UIState.SearchString[App->UIState.SearchStringCurrentIndex] = (wchar_t)((char)i);
                    App->UIState.SearchStringCurrentIndex++;
                    App->Renderer.input.keys[i] = false;
                    break;
                }
            }
        }

        if (App->UIState.SearchStringCurrentIndex > 0 && App->Renderer.input.keys[VK_BACK]) {
            App->UIState.SearchString[App->UIState.SearchStringCurrentIndex-1] = L' ';
            App->UIState.SearchStringCurrentIndex--;
            App->Renderer.input.keys[VK_BACK] = false;
        }

    } break;
    }


    switch (App->UIState.CurrentLayout)
    {
    case UI_LAYOUT_LIBRARY: { RenderLayout_Library(App); } break;
    case UI_LAYOUT_PLAYLIST: { RenderLayout_Playlist(App); } break;
    }

    AudioEngineUpdate(&App->AudioEngine);
    console_render(&App->Renderer);
}


