#include "audio_engine.h"

#define fourccRIFF 'FFIR'
#define fourccDATA 'atad'
#define fourccFMT  ' tmf'
#define fourccWAVE 'EVAW'
#define fourccXWMA 'AMWX'
#define fourccDPDS 'sdpd'


u8 AudioEngineInit(audio_engine* AudioEngine) {
    ::CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    ::XAudio2Create(&AudioEngine->xAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
    AudioEngine->xAudio2->CreateMasteringVoice(&AudioEngine->xAudioMasteringVoice);

    // @NOTE: Not true
    AudioEngine->IsPlaying = true;
    AudioEngineLoadSongs(AudioEngine);

    return 0;
}

u8 AudioEnginePause(audio_engine* AudioEngine) {
    AudioEngine->xAudio2->StopEngine();
    AudioEngine->IsPlaying = false;
    return 0;
}

u8 AudioEnginePlay(audio_engine* AudioEngine) {
    AudioEngine->xAudio2->StartEngine();
    AudioEngine->IsPlaying = true;
    return 0;
}

u8 AudioEngineTogglePlayPause(audio_engine* AudioEngine) {
    if (AudioEngine->IsPlaying) { return AudioEnginePause(AudioEngine); }
    else { return AudioEnginePlay(AudioEngine); }
}


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


static int ParseSongDataFromWavFile(HANDLE FileHandle, song_data* OutSongData) {
    if (INVALID_SET_FILE_POINTER == SetFilePointer(FileHandle, 0, 0, FILE_BEGIN)) {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    OutSongData->AudioBuffer    = {0};
    OutSongData->Wfx            = {0};

    DWORD dwChunkSize;
    DWORD dwChunkPosition;

    // 'RIFF' chunk
    FindChunk(FileHandle, fourccRIFF, dwChunkSize, dwChunkPosition);
    DWORD Filetype;
    ReadChunkData(FileHandle, &Filetype, sizeof(DWORD), dwChunkPosition);
    if (Filetype != fourccWAVE) { return S_FALSE; }

    // 'fmt ' chunk
    FindChunk(FileHandle, fourccFMT, dwChunkSize, dwChunkPosition);
    ReadChunkData(FileHandle, (WAVEFORMATEXTENSIBLE*)&OutSongData->Wfx, dwChunkSize, dwChunkPosition);

    // 'data' chunk
    FindChunk(FileHandle, fourccDATA, dwChunkSize, dwChunkPosition);
    BYTE* pDataBuffer = new BYTE[dwChunkSize];
    ReadChunkData(FileHandle, pDataBuffer, dwChunkSize, dwChunkPosition);

    OutSongData->AudioBuffer.AudioBytes = dwChunkSize;
    OutSongData->AudioBuffer.pAudioData = pDataBuffer;
    OutSongData->AudioBuffer.Flags = XAUDIO2_END_OF_STREAM;

    return 0;
}

TCHAR* CharStr2WChar(const char* data, u64 size) {
    TCHAR* res = (TCHAR*)malloc(size*2);
    for (int i = 0; i < size; ++i) { res[i] = (wchar_t)data[i]; }
    return res;
}

void LoadDummySong(const wchar_t* Name, const wchar_t* Artist, const wchar_t* Album, song_data* OutSong) {
    memcpy_s(OutSong->SongName, sizeof(wchar_t)*32, Name, sizeof(wchar_t)*32);
    memcpy_s(OutSong->Artist, sizeof(wchar_t)*32, Artist, sizeof(wchar_t)*32);
    memcpy_s(OutSong->Album, sizeof(wchar_t)*32, Album, sizeof(wchar_t)*32);
}

u8 AudioEngineLoadSongs(audio_engine* AudioEngine) {
    // AudioEngine->SongsCount = 0;

    AudioEngine->SongsCount = 3;
    AudioEngine->Songs = (song_data*)malloc(sizeof(song_data)*AudioEngine->SongsCount);

    AudioEngine->CurrentSongIndex = 1;

    LoadDummySong(L"Interstellar Theme", L"Hand Zimmer", L"Interstellar OST", AudioEngine->Songs);
    LoadDummySong(L"Vermilion Pt.2", L"Slipknot", L"Vermilion", AudioEngine->Songs+1);
    LoadDummySong(L"Driven Under", L"Seether", L"Seether", AudioEngine->Songs+2);

    return 0;
}

u8 LoadSongDataFromFile(const char* File, song_data* OutSongData) {
    TCHAR* strFileName = CharStr2WChar(File, (u64)strlen(File));
    HANDLE hFile = CreateFile(
        strFileName,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);
    free(strFileName);

    if( INVALID_HANDLE_VALUE == hFile )
        return HRESULT_FROM_WIN32( GetLastError() );

    if( INVALID_SET_FILE_POINTER == SetFilePointer( hFile, 0, NULL, FILE_BEGIN ) )
        return HRESULT_FROM_WIN32( GetLastError() );

    ParseSongDataFromWavFile(hFile, OutSongData);
    strncpy((char*)OutSongData->SongName, "Interstellar Theme", 32);
    strncpy((char*)OutSongData->Artist, "Hans Zimmer", 32);
    strncpy((char*)OutSongData->Album, "Interstellar OST", 32);

    return 0;
}

u8 AudioEngineInitSong(audio_engine* AudioEngine, const char* SongName) {
    song_data SongData;
    LoadSongDataFromFile(SongName, &SongData);
    // @NOTE
    AudioEngine->xAudio2->CreateSourceVoice(&AudioEngine->xAudio2SourceVoice, (WAVEFORMATEX*)&SongData.Wfx);

    AudioEngine->xAudio2SourceVoice->SubmitSourceBuffer(&SongData.AudioBuffer);

    return 0;
}

