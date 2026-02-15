
#include "audio_engine.h"
#include "lh_crenderer.h"

#define fourccRIFF 'FFIR'
#define fourccDATA 'atad'
#define fourccFMT  ' tmf'
#define fourccWAVE 'EVAW'
#define fourccXWMA 'AMWX'
#define fourccDPDS 'sdpd'


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


// LRESULT WINAPI Win32MainWindowCallback (
//   HWND hWnd,
//   UINT Message,
//   WPARAM wParam,
//   LPARAM lParam
// )
// {
//     LRESULT Result = 0;
//     switch(Message) {
//         case WM_SIZE:
//         {} break;
//         case WM_DESTROY: 
//         {
//             GlobalRunning = false;
//         } break;
//         case WM_PAINT:
//         {
//             PAINTSTRUCT ps;
//             RECT ClientRect;
//             GetClientRect(hWnd, &ClientRect);
//             int ClientWidth     = ClientRect.right - ClientRect.left;
//             int ClientHeight    = ClientRect.bottom - ClientRect.top;
//         } break;
//         default:
//         {
//             Result = DefWindowProc(hWnd, Message, wParam, lParam);
//         } break;
//     }

//     return Result;
// }

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

    while (GlobalRunning) {
        console_clear(&console, L' ', 0);
        // console_draw_rectangle(&console, {1, 3, WIDTH / 2 - 8, WIDTH / 2 + 8});
        // console_write_text(&console, WIDTH / 2 - 4, 2, L"M Player", 8);

        render_bounds(&console);
        console_render(&console);

        Sleep(1600);
        AudioEngineTogglePlayPause(&AudioEngine);
    }

    printf("End of program\n");

    // AudioEngineUninit(NetworkAudioEngine.AudioEngine);
    return 0;
}



