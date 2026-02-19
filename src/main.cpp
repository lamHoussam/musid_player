
#include "audio_engine.h"
#include "lh_crenderer.h"

#define fourccRIFF 'FFIR'
#define fourccDATA 'atad'
#define fourccFMT  ' tmf'
#define fourccWAVE 'EVAW'
#define fourccXWMA 'AMWX'
#define fourccDPDS 'sdpd'




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


void draw_header(ConsoleRenderer* c) {
    ConsoleRect rc = {HEADER_TOP, HEADER_BOTTOM, 0, WIDTH - 1};
    console_draw_rectangle(c, rc);

    console_write_text(c, WIDTH/2 - 6, 2, L"TERMINAL PLAYER", 20);
}


void draw_playlist(ConsoleRenderer* c, int selected) {
    ConsoleRect rc = {BODY_TOP, BODY_BOTTOM, PLAYLIST_LEFT, PLAYLIST_RIGHT};
    console_draw_rectangle(c, rc);

    console_write_text(c, 2, BODY_TOP, L" PLAYLIST ", 10);

    int y = BODY_TOP + 2;

    for (int i = 0; i < 20; i++) {
        WORD attr = (i == selected) ? COLOR_SELECTED : COLOR_DEFAULT;

        wchar_t buffer[64];
        swprintf(buffer, 64, L"%02d. Track Name %d", i+1, i+1);

        console_write_text(c, 2, y++, buffer, wcslen(buffer));
    }
}

void draw_now_playing(ConsoleRenderer* c) {
    ConsoleRect rc = {BODY_TOP, BODY_BOTTOM, INFO_LEFT, INFO_RIGHT};
    console_draw_rectangle(c, rc);

    console_write_text(c, INFO_LEFT + 2, BODY_TOP, L" NOW PLAYING ", 12);

    int x = INFO_LEFT + 4;
    int y = BODY_TOP + 3;

    console_write_text(c, x, y,     L"Title : Interstellar Theme", 27);
    console_write_text(c, x, y + 2, L"Artist: Hans Zimmer", 20);
    console_write_text(c, x, y + 4, L"Album : Interstellar OST", 25);
    console_write_text(c, x, y + 6, L"Year  : 2014", 13);
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









static bool GlobalRunning = true;

HRESULT WINAPI FindChunk(HANDLE hFile, DWORD fourcc, DWORD& dwChunkSize, DWORD& dwChunkDataPosition)
{
    HRESULT hr = S_OK;
    if(INVALID_SET_FILE_POINTER == SetFilePointer(hFile, 0, NULL, FILE_BEGIN))
        return HRESULT_FROM_WIN32(GetLastError());

    DWORD dwChunkType;
    DWORD dwChunkDataSize;
    DWORD dwRIFFDataSize = 0;
    DWORD dwFileType;
    DWORD bytesRead = 0;
    DWORD dwOffset = 0;

    while (hr == S_OK)
    {
        DWORD dwRead;
        if( 0 == ReadFile( hFile, &dwChunkType, sizeof(DWORD), &dwRead, NULL ) )
            hr = HRESULT_FROM_WIN32( GetLastError() );

        if( 0 == ReadFile( hFile, &dwChunkDataSize, sizeof(DWORD), &dwRead, NULL ) )
            hr = HRESULT_FROM_WIN32( GetLastError() );

        switch (dwChunkType)
        {
        case fourccRIFF:
            dwRIFFDataSize = dwChunkDataSize;
            dwChunkDataSize = 4;
            if( 0 == ReadFile( hFile, &dwFileType, sizeof(DWORD), &dwRead, NULL ) )
                hr = HRESULT_FROM_WIN32( GetLastError() );
            break;

        default:
            if( INVALID_SET_FILE_POINTER == SetFilePointer( hFile, dwChunkDataSize, NULL, FILE_CURRENT ) )
            return HRESULT_FROM_WIN32( GetLastError() );            
        }

        dwOffset += sizeof(DWORD) * 2;

        if (dwChunkType == fourcc)
        {
            dwChunkSize = dwChunkDataSize;
            dwChunkDataPosition = dwOffset;
            return S_OK;
        }

        dwOffset += dwChunkDataSize;
        if (bytesRead >= dwRIFFDataSize) return S_FALSE;
    }

    return S_OK;
}

HRESULT ReadChunkData(HANDLE hFile, void * buffer, DWORD buffersize, DWORD bufferoffset)
{
    HRESULT hr = S_OK;
    if( INVALID_SET_FILE_POINTER == SetFilePointer( hFile, bufferoffset, NULL, FILE_BEGIN ) )
        return HRESULT_FROM_WIN32( GetLastError() );
    DWORD dwRead;
    if( 0 == ReadFile( hFile, buffer, buffersize, &dwRead, NULL ) )
        hr = HRESULT_FROM_WIN32( GetLastError() );
    return hr;
}

static int CreateAudioBufferFromWavFile(HANDLE FileHandle, WAVEFORMATEXTENSIBLE* Wfx, XAUDIO2_BUFFER* AudioBuffer) {
    if (INVALID_SET_FILE_POINTER == SetFilePointer(FileHandle, 0, 0, FILE_BEGIN)) {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    DWORD dwChunkSize;
    DWORD dwChunkPosition;

    // 'RIFF' chunk
    FindChunk(FileHandle, fourccRIFF, dwChunkSize, dwChunkPosition);
    DWORD Filetype;
    ReadChunkData(FileHandle, &Filetype, sizeof(DWORD), dwChunkPosition);
    if (Filetype != fourccWAVE) { return S_FALSE; }

    // 'fmt ' chunk
    FindChunk(FileHandle, fourccFMT, dwChunkSize, dwChunkPosition);
    ReadChunkData(FileHandle, Wfx, dwChunkSize, dwChunkPosition);

    // 'data' chunk
    FindChunk(FileHandle, fourccDATA, dwChunkSize, dwChunkPosition);
    BYTE* pDataBuffer = new BYTE[dwChunkSize];
    ReadChunkData(FileHandle, pDataBuffer, dwChunkSize, dwChunkPosition);

    AudioBuffer->AudioBytes = dwChunkSize;
    AudioBuffer->pAudioData = pDataBuffer;
    AudioBuffer->Flags = XAUDIO2_END_OF_STREAM;

    return 0;
}

int main() {
    printf("Start of program\n");
    ConsoleRenderer console;
    console_init(&console);
    console_clear(&console, L' ', 0);

    // @NOTE: Data init, needs to be somewhere else
    WAVEFORMATEX Wfx = {0};
    // Wfx.wFormatTag = WAVE_FORMAT_PCM;
    // Wfx.nChannels = 2;
    // Wfx.nSamplesPerSec = 41000;
    // Wfx.wBitsPerSample = 16; // @NOTE: Change if needed
    // Wfx.nBlockAlign = (Wfx.nChannels*Wfx.wBitsPerSample)/8;
    // Wfx.nAvgBytesPerSec = 41000*Wfx.nBlockAlign;

    XAUDIO2_BUFFER AudioBuffer = {0};

    TCHAR* strFileName = __TEXT("data\\song.wav");
    HANDLE hFile = CreateFile(
        strFileName,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        0,
        NULL );

    if( INVALID_HANDLE_VALUE == hFile )
        return HRESULT_FROM_WIN32( GetLastError() );

    if( INVALID_SET_FILE_POINTER == SetFilePointer( hFile, 0, NULL, FILE_BEGIN ) )
        return HRESULT_FROM_WIN32( GetLastError() );

    CreateAudioBufferFromWavFile(hFile, (WAVEFORMATEXTENSIBLE*)&Wfx, &AudioBuffer);


    audio_engine AudioEngine;
    AudioEngineInit(&AudioEngine);

    // @NOTE
    AudioEngine.xAudio2->CreateSourceVoice(&AudioEngine.xAudio2SourceVoice, (WAVEFORMATEX*)&Wfx);

    AudioEngine.xAudio2SourceVoice->SubmitSourceBuffer(&AudioBuffer);
    AudioEngine.xAudio2SourceVoice->Start(0);

    // AudioEngineStartSong(&NetworkAudioEngine, WindowHandle, &SongHeader);

    b8 Switch = false;

    while (GlobalRunning) {

        console_process_input(&console);
        console_clear(&console, L' ', COLOR_DEFAULT);

        draw_header(&console);
        draw_playlist(&console, 0);
        draw_now_playing(&console);
        draw_progress(&console, 40);
        draw_footer(&console);

        render_bounds(&console);
        console_render(&console);
        if (console.input.keys[L'w']) {
            AudioEngineTogglePlayPause(&AudioEngine);
        }
    }


    printf("End of program\n");

    // AudioEngineUninit(NetworkAudioEngine.AudioEngine);
    return 0;
}



