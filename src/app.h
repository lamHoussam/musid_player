#pragma once

#include "audio_engine.h"
#include "lh_crenderer.h"


#define HEADER_TOP      0
#define HEADER_BOTTOM   4

#define BODY_TOP        5
#define BODY_BOTTOM     34

#define PROGRESS_TOP    35
#define PROGRESS_BOTTOM 39

#define FOOTER_TOP      40
#define FOOTER_BOTTOM   49

#define PLAYLIST_LEFT   0
#define PLAYLIST_RIGHT  59

#define INFO_LEFT       60
#define INFO_RIGHT      199


#define COLOR_DEFAULT   7
#define COLOR_HEADER    15
#define COLOR_ACCENT    10
#define COLOR_SELECTED  112

enum ui_mode {
    UI_MODE_NORMAL   = 0,
    UI_MODE_SEARCH   = 1,
};

enum ui_layout {
    UI_LAYOUT_LIBRARY   = 0,
    UI_LAYOUT_PLAYLIST  = 1,
};


struct ui_state {
    i64             UIVisualStartIndex;
    i64             UICurrentSelectedSongIndex;
    song_metadata*  MetadataToRenderBuffer;
    i64             MetadataCount;
    ui_mode         CurrentMode;
    wchar_t         SearchString[16];
    i8              SearchStringCurrentIndex;
    ui_layout       CurrentLayout;
};

struct playlist {
    wchar_t Name[32];
    i64     SongsCount;
    i64     Capacity;
    i64*    SongIndexList;
};

struct app
{
    audio_engine    AudioEngine;
    ConsoleRenderer Renderer;
    ui_state        UIState;
    b8              IsRunning;
    playlist        CurrentPlaylist; // Should have multiple playlists later
};

void    draw_header(ConsoleRenderer* c);
void    draw_playlist(app* App);
void    draw_now_playing(app* App);
void    draw_progress(app* App);
void    draw_footer(ConsoleRenderer* c);

u8      playlist_init(playlist* Playlist, i64 Capacity, const wchar_t* Name);
u8      playlist_push(playlist* Playlist, i64 SongIndex);
u8      playlist_removeAt(playlist* Playlist, i64 Index);

u8      app_init(app* App);
void    app_run(app* App);
void    app_update(app* App);

