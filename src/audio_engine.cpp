#include "audio_engine.h"

#define fourccRIFF 'FFIR'
#define fourccDATA 'atad'
#define fourccFMT  ' tmf'
#define fourccWAVE 'EVAW'
#define fourccXWMA 'AMWX'
#define fourccDPDS 'sdpd'
#define fourccLIST 'TSIL'
#define fourccINFO 'OFNI'
#define fourccIART 'TRAI'
#define fourccINAM 'MANI'
#define fourccIPRD 'DRPI'


u8 AudioEngineInit(audio_engine* AudioEngine) {
    ::CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    ::XAudio2Create(&AudioEngine->xAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);

    AudioEngine->VoiceCallback = new voice_callback();

    AudioEngine->xAudio2->CreateMasteringVoice(&AudioEngine->xAudioMasteringVoice);

    // @NOTE: Not true
    AudioEngine->IsPlaying = true;
    AudioEngine->CurrentSongIndex       = 0;
    // @NOTE: Check 
    AudioEngine->SongsCapacity      = 100;
    AudioEngine->SongsCount         = 0;
    AudioEngine->Songs = (song_data*)malloc(sizeof(song_data)*AudioEngine->SongsCapacity);
    // AudioEngineLoadSongs(AudioEngine);

    return 0;
}

void AudioEngineUpdate(audio_engine* AudioEngine) {
    if (WaitForSingleObject(AudioEngine->VoiceCallback->hBufferEndEvent, 0) == WAIT_OBJECT_0) { 
        printf("I have awaited properly\n");
        AudioEnginePlayNext(AudioEngine); 
        printf("Started playing %lld\n", AudioEngine->CurrentSongIndex);
        ResetEvent(AudioEngine->VoiceCallback->hBufferEndEvent);
    }
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


static void CharStr2WCharStr(const char* Str, wchar_t* wStr) {
    const size_t cSize = strlen(Str)+1;
    mbstowcs (wStr, Str, cSize);
}

static int _LoadSongAudioBufferFromFileHandle(HANDLE FileHandle, song_data* OutSongData) {
    DWORD dwChunkSize;
    DWORD dwChunkPosition;
    // 'data' chunk
    FindChunk(FileHandle, fourccDATA, dwChunkSize, dwChunkPosition);
    OutSongData->SongBuffer = malloc(sizeof(BYTE)*dwChunkSize);
    ReadChunkData(FileHandle, OutSongData->SongBuffer, dwChunkSize, dwChunkPosition);
    OutSongData->AudioBuffer.AudioBytes = dwChunkSize;
    OutSongData->AudioBuffer.pAudioData = (BYTE*)OutSongData->SongBuffer;
    OutSongData->AudioBuffer.Flags = XAUDIO2_END_OF_STREAM;

    OutSongData->SongBufferSize         = dwChunkSize;
    OutSongData->AudioBufferIsLoaded    = true;
    return 0;
}


static int _LoadSongAudioBufferFromFile(wchar_t* File, song_data* OutSongData) {

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
    
    return _LoadSongAudioBufferFromFileHandle(hFile, OutSongData);
}

static int ParseSongDataFromWavFile(HANDLE FileHandle, song_data* OutSongData, b32 LoadAudioBuffer) {
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

    if (LoadAudioBuffer) {
        _LoadSongAudioBufferFromFileHandle(FileHandle, OutSongData);
    } 

    if (FindChunk(FileHandle, fourccLIST, dwChunkSize, dwChunkPosition) == S_OK)
    {
        DWORD listType;
        ReadChunkData(FileHandle, &listType, sizeof(DWORD), dwChunkPosition);

        if (listType == fourccINFO)
        {
            DWORD bytesRead = sizeof(DWORD);
            DWORD currentPos = dwChunkPosition + sizeof(DWORD);

            while (bytesRead < dwChunkSize)
            {
                DWORD chunkId;
                DWORD chunkSize;

                ReadChunkData(FileHandle, &chunkId, sizeof(DWORD), currentPos);
                currentPos += sizeof(DWORD);

                ReadChunkData(FileHandle, &chunkSize, sizeof(DWORD), currentPos);
                currentPos += sizeof(DWORD);

                if (chunkId == fourccIART) {
                    char Artist[MAX_PATH];
                    ReadChunkData(FileHandle, Artist, chunkSize, currentPos);
                    Artist[chunkSize] = '\0';
                    CharStr2WCharStr(Artist, OutSongData->Artist);
                    break;
                } else if (chunkId == fourccINAM) {
                    char Title[MAX_PATH];
                    ReadChunkData(FileHandle, Title, chunkSize, currentPos);
                    Title[chunkSize] = '\0';
                    CharStr2WCharStr(Title, OutSongData->SongName);
                    break;
                } else if (chunkId == fourccIPRD) {
                    // @NOTE: Need default value for album
                    char Album[MAX_PATH];
                    ReadChunkData(FileHandle, Album, chunkSize, currentPos);
                    Album[chunkSize] = '\0';
                    CharStr2WCharStr(Album, OutSongData->Album);
                    break;
                }
                currentPos += chunkSize;

                if (chunkSize % 2 != 0)
                    currentPos++;

                bytesRead += sizeof(DWORD) * 2 + chunkSize;
            }
        }
    }

    return 0;
}

void LoadDummySong(const wchar_t* Name, const wchar_t* Artist, const wchar_t* Album, song_data* OutSong) {
    memcpy_s(OutSong->SongName, sizeof(wchar_t)*32, Name, sizeof(wchar_t)*32);
    memcpy_s(OutSong->Artist, sizeof(wchar_t)*32, Artist, sizeof(wchar_t)*32);
    memcpy_s(OutSong->Album, sizeof(wchar_t)*32, Album, sizeof(wchar_t)*32);
    OutSong->AudioBufferIsLoaded = false;
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

u8 AudioEngineUnloadSongAudioBuffer(audio_engine* AudioEngine, u64 SongIndex) {
    song_data* SongData = AudioEngine->Songs+SongIndex;
    if (SongData->AudioBufferIsLoaded) {
        SongData->AudioBufferIsLoaded = false; 
        free(SongData->SongBuffer);
        SongData->AudioBuffer.AudioBytes = {0};
        AudioEngine->LoadedSongsCount--;
        printf("Unloaded song: %lld\n", SongIndex);
    }

    return 0;
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

u8 LoadSongDataFromFile(const wchar_t* File, song_data* OutSongData, b32 LoadAudioBuffer) {
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

    ParseSongDataFromWavFile(hFile, OutSongData, LoadAudioBuffer);

    // @NOTE: Read file metadata
    wchar_t* FileName = GetFileName(const_cast<wchar_t*>(File));
    size_t Size = min(wcslen(FileName), MAX_PATH)*sizeof(wchar_t);

    Size = min(wcslen(FileName), MAX_PATH)*sizeof(wchar_t);
    memcpy_s(OutSongData->SongName, sizeof(wchar_t)*MAX_PATH, FileName, Size);
    OutSongData->SongName[Size-1] = '\0';

    Size = min(wcslen(L"Unknown"), MAX_PATH)*sizeof(wchar_t);
    memcpy_s(OutSongData->Artist, sizeof(wchar_t)*MAX_PATH, L"Unknown", Size);
    OutSongData->Artist[Size-1] = '\0';

    Size = min(wcslen(File), MAX_PATH)*sizeof(wchar_t);
    memcpy_s(OutSongData->FilePath, sizeof(wchar_t)*MAX_PATH, File, Size);
    OutSongData->FilePath[Size-1] = '\0';

    return 0;
}

u8 AudioEngineLoadSong(audio_engine* AudioEngine, const wchar_t* SongName) {
    if (AudioEngine->SongsCount < AudioEngine->SongsCapacity) {
        song_data* SongData = &AudioEngine->Songs[AudioEngine->SongsCount++];
        if (AudioEngine->LoadedSongsCount <= AUDIO_ENGINE_MAX_LOADED_SONGS) {
            LoadSongDataFromFile(SongName, SongData, true);
            AudioEngine->LoadedSongsCount++;
        } else {
            LoadSongDataFromFile(SongName, SongData, false);
        }
        // @NOTE
        // AudioEngine->xAudio2->CreateSourceVoice(&AudioEngine->xAudio2SourceVoice, (WAVEFORMATEX*)&SongData->Wfx);

        // AudioEngine->xAudio2SourceVoice->SubmitSourceBuffer(&SongData->AudioBuffer);
    }

    return 0;
}

// @NOTE: Maybe do it differently
u8 _UnloadFirstLoadedSongThatIsNotPlaying(audio_engine* AudioEngine) {
    for (u64 i = 0; i < AudioEngine->SongsCount; ++i) {
        song_data* SongData = AudioEngine->Songs+i;
        if (AudioEngine->CurrentSongIndex != i && SongData->AudioBufferIsLoaded) {
            return AudioEngineUnloadSongAudioBuffer(AudioEngine, i);
        }
    }
    return 1;
}

u8 _AudioEnginePlaySong(audio_engine* AudioEngine) {
    song_data* SongData = AudioEngine->Songs+AudioEngine->CurrentSongIndex;

    if (AudioEngine->xAudio2SourceVoice != nullptr) {
        AudioEngine->xAudio2SourceVoice->DestroyVoice();
        // AudioEngine->xAudio2SourceVoice->Stop(0);
        // @NOTE: Not sure its the right way
        // AudioEngine->xAudio2SourceVoice->FlushSourceBuffers();
    }

    if (!SongData->AudioBufferIsLoaded) {
        _LoadSongAudioBufferFromFile(SongData->FilePath, SongData);
    }

    AudioEngine->xAudio2->CreateSourceVoice(&AudioEngine->xAudio2SourceVoice, (WAVEFORMATEX*)&SongData->Wfx, 0, XAUDIO2_DEFAULT_FREQ_RATIO, AudioEngine->VoiceCallback, NULL, NULL);

    AudioEngine->xAudio2SourceVoice->SubmitSourceBuffer(&SongData->AudioBuffer);
    AudioEngine->xAudio2SourceVoice->Start(0);

    _UnloadFirstLoadedSongThatIsNotPlaying(AudioEngine);

    printf("Started playing %lld\n", AudioEngine->CurrentSongIndex);
    return 0;
}

u8 AudioEnginePlaySongAtIndex(audio_engine* AudioEngine, u64 Index) {
    AudioEngine->CurrentSongIndex = Index;
    return _AudioEnginePlaySong(AudioEngine);
}

u8 AudioEnginePlayNext(audio_engine* AudioEngine) {
    if (AudioEngine->SongsCount == 0) { return 1; }
    // @NOTE: Check
    if (++AudioEngine->CurrentSongIndex >= AudioEngine->SongsCount) {
        AudioEngine->CurrentSongIndex = 0;
    }

    return _AudioEnginePlaySong(AudioEngine);
}

u8 AudioEnginePlayPrev(audio_engine* AudioEngine) {
    if (AudioEngine->SongsCount == 0) { return 1; }
    // @NOTE: Check
    if (AudioEngine->CurrentSongIndex == 0) {
        AudioEngine->CurrentSongIndex = AudioEngine->SongsCount - 1;
    } else { --AudioEngine->CurrentSongIndex; }

    return _AudioEnginePlaySong(AudioEngine);
}
