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
    // AudioEngineLoadSongs(AudioEngine);

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

u8 AudioEngineStartSong(audio_engine* AudioEngine, u64 SongIndex) {
    if (SongIndex < AudioEngine->SongsCount) {
        song_data* ChosenSong = AudioEngine->Songs+SongIndex;
        AudioEngineLoadSong(AudioEngine, ChosenSong->FilePath);
        AudioEngine->xAudio2SourceVoice->Start(0);
        return 0;
    }
    return 1;
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

wchar_t* GetFileName(wchar_t* path)
{
    wchar_t* lastSlash = wcsrchr(path, L'\\');
    wchar_t* lastForwardSlash = wcsrchr(path, L'/');

    wchar_t* lastSeparator = lastSlash > lastForwardSlash 
        ? lastSlash 
        : lastForwardSlash;

    return lastSeparator ? lastSeparator + 1 : path;
}

u8 AudioEngineLoadSongsFromFolder(audio_engine* AudioEngine, const wchar_t* Folder) {
    wchar_t searchPath[MAX_PATH];
    swprintf_s(searchPath, L"%s\\*.wav", Folder);

    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW(searchPath, &findData);

    if (hFind == INVALID_HANDLE_VALUE)
    {
        wprintf(L"Failed to open folder.\n");
        return 1;
    }

    do
    {
        if (wcscmp(findData.cFileName, L".") == 0 ||
            wcscmp(findData.cFileName, L"..") == 0)
        {
            continue;
        }
        if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            wchar_t FilePath[MAX_PATH];
            swprintf(FilePath, MAX_PATH, L"%ls\\%ls", Folder, findData.cFileName);
            // @NOTE: Load the file into song_data
            AudioEngineLoadSong(AudioEngine, FilePath);
            wprintf(L"File: %s\n", FilePath);
        }

    } while (FindNextFileW(hFind, &findData));

    FindClose(hFind);
    return 0;
}

u8 LoadSongDataFromFile(const wchar_t* File, song_data* OutSongData) {
    HANDLE hFile = CreateFileW(
        File,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);

    if( INVALID_HANDLE_VALUE == hFile )
        return HRESULT_FROM_WIN32( GetLastError() );

    if( INVALID_SET_FILE_POINTER == SetFilePointer( hFile, 0, NULL, FILE_BEGIN ) )
        return HRESULT_FROM_WIN32( GetLastError() );

    ParseSongDataFromWavFile(hFile, OutSongData);

    // @NOTE: Add this to file metadata
    wchar_t* FileName = GetFileName(const_cast<wchar_t*>(File));
    wchar_t FileNameCopy[32];
    size_t Size = min(wcslen(FileName), 32)*sizeof(wchar_t);

    memcpy_s(FileNameCopy, Size, FileName, Size);
    printf("%ls\n", FileNameCopy);
    wchar_t* Name      = wcstok(FileNameCopy, L"_");
    wchar_t* Artist    = wcstok(NULL, L"_");

    printf("%ls\n", Name);
    printf("%ls\n", Artist);

    memcpy_s(OutSongData->SongName, sizeof(wchar_t)*32, Name, sizeof(wchar_t)*wcslen(Name));
    memcpy_s(OutSongData->Artist, sizeof(wchar_t)*32, Artist, sizeof(wchar_t)*wcslen(Artist));
    memcpy_s(OutSongData->FilePath, sizeof(wchar_t)*32, File, sizeof(wchar_t)*wcslen(File));
    // memcpy_s(OutSongData->Album, sizeof(wchar_t)*32, L"IDK", sizeof(wchar_t)*wcslen(A));

    return 0;
}


u8 AudioEngineLoadSongs(audio_engine* AudioEngine) {
    // AudioEngine->SongsCount = 0;

    AudioEngine->SongsCount = 4;
    AudioEngine->Songs = (song_data*)malloc(sizeof(song_data)*AudioEngine->SongsCount);

    AudioEngine->CurrentSongIndex = 3;

    LoadDummySong(L"Interstellar Theme", L"Hand Zimmer", L"Interstellar OST", AudioEngine->Songs);
    LoadDummySong(L"Vermilion Pt.2", L"Slipknot", L"Vermilion", AudioEngine->Songs+1);
    LoadDummySong(L"Driven Under", L"Seether", L"Seether", AudioEngine->Songs+2);
    LoadSongDataFromFile(L"data\\FadeToBlack_Metallica.wav", AudioEngine->Songs+3);

    return 0;
}

u8 AudioEngineLoadSong(audio_engine* AudioEngine, const wchar_t* SongName) {
    song_data SongData;
    LoadSongDataFromFile(SongName, &SongData);
    // @NOTE
    AudioEngine->xAudio2->CreateSourceVoice(&AudioEngine->xAudio2SourceVoice, (WAVEFORMATEX*)&SongData.Wfx);

    AudioEngine->xAudio2SourceVoice->SubmitSourceBuffer(&SongData.AudioBuffer);

    return 0;
}

