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


struct app
{
    audio_engine    AudioEngine;
    ConsoleRenderer Renderer;
    b8              IsRunning;
};

void    draw_header(ConsoleRenderer* c);
void    draw_playlist(app* App);
void    draw_now_playing(app* App);
void    draw_progress(app* App);
void    draw_footer(ConsoleRenderer* c);
u8      app_init(app* App);
void    app_run(app* App);
void    app_update(app* App);

