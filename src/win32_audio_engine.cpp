#include <windows.h>
#include <XAUDIO2.h>

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

struct voice_callback : public IXAudio2VoiceCallback {
public: 
    HANDLE hBufferEndEvent;
    voice_callback(): hBufferEndEvent(CreateEvent(NULL, TRUE, FALSE, NULL)) {}
    ~voice_callback() { CloseHandle(hBufferEndEvent); }

    void OnStreamEnd() { 
        printf("On Stream End\n");
        SetEvent(hBufferEndEvent);
    }

    void OnVoiceProcessingPassEnd() { }
    void OnVoiceProcessingPassStart(UINT32 SamplesRequired) {    }
    void OnBufferEnd(void * pBufferContext) override { 
        printf("On Buffer End\n");
    }
    void OnBufferStart(void * pBufferContext) {}
    void OnLoopEnd(void * pBufferContext) {}
    void OnVoiceError(void * pBufferContext, HRESULT Error) {
        printf("Error on voice\n");
    }
};


struct platform_audio_engine {
    XAUDIO2_BUFFER          AudioBuffer;
    voice_callback*         VoiceCallback;
    IXAudio2*               xAudio2{};
    IXAudio2SourceVoice*    xAudio2SourceVoice{};
    IXAudio2MasteringVoice* xAudioMasteringVoice{};
};

static void CharStr2WCharStr(const char* Str, wchar_t* wStr, u32 Size) {
    mbstowcs(wStr, Str, Size);
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

static i32 _LoadSongAudioBufferFromFileHandle(HANDLE FileHandle, song_data* OutSongData) {
    DWORD dwChunkSize;
    DWORD dwChunkPosition;
    // 'data' chunk
    FindChunk(FileHandle, fourccDATA, dwChunkSize, dwChunkPosition);
    OutSongData->SongBufferData.Data = (u8*)malloc(sizeof(u8)*dwChunkSize);
    ReadChunkData(FileHandle, OutSongData->SongBufferData.Data, dwChunkSize, dwChunkPosition);

    OutSongData->SongBufferData.BufferSize = dwChunkSize;
    OutSongData->AudioBufferIsLoaded    = true;
    return 0;
}

static i32 ParseSongDataFromWavFile(HANDLE FileHandle, song_data* OutSongData, b32 LoadAudioBuffer) {
    if (INVALID_SET_FILE_POINTER == SetFilePointer(FileHandle, 0, 0, FILE_BEGIN)) {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    WAVEFORMATEX Wfx            = {0};
    OutSongData->SongBufferData = {0};

    DWORD dwChunkSize;
    DWORD dwChunkPosition;

    // 'RIFF' chunk
    FindChunk(FileHandle, fourccRIFF, dwChunkSize, dwChunkPosition);
    DWORD Filetype;
    ReadChunkData(FileHandle, &Filetype, sizeof(DWORD), dwChunkPosition);
    if (Filetype != fourccWAVE) { return S_FALSE; }

    // 'fmt ' chunk
    FindChunk(FileHandle, fourccFMT, dwChunkSize, dwChunkPosition);
    ReadChunkData(FileHandle, (WAVEFORMATEXTENSIBLE*)&Wfx, dwChunkSize, dwChunkPosition);

    OutSongData->SongBufferData.Channels            = Wfx.nChannels;
    OutSongData->SongBufferData.SamplesPerSec       = Wfx.nSamplesPerSec;
    OutSongData->SongBufferData.NumAvgBytesPerSec   = Wfx.nAvgBytesPerSec;
    OutSongData->SongBufferData.BlockAlign          = Wfx.nBlockAlign;
    OutSongData->SongBufferData.BitsPerSample       = Wfx.wBitsPerSample;

    if (LoadAudioBuffer) {
        _LoadSongAudioBufferFromFileHandle(FileHandle, OutSongData);
    } 

    if (FindChunk(FileHandle, fourccLIST, dwChunkSize, dwChunkPosition) == S_OK)
    {
        DWORD listType;
        ReadChunkData(FileHandle, &listType, sizeof(DWORD), dwChunkPosition);

        u8 FoundMetaData = 0;

        if (listType == fourccINFO) {
            DWORD bytesRead = sizeof(DWORD);
            DWORD currentPos = dwChunkPosition + sizeof(DWORD);

            while (bytesRead < dwChunkSize) {
                DWORD chunkId;
                DWORD chunkSize;

                ReadChunkData(FileHandle, &chunkId, sizeof(DWORD), currentPos);
                currentPos += sizeof(DWORD);

                ReadChunkData(FileHandle, &chunkSize, sizeof(DWORD), currentPos);
                currentPos += sizeof(DWORD);

                if (chunkId == fourccIART) {
                    char Artist[MAX_PATH];
                    ReadChunkData(FileHandle, Artist, chunkSize, currentPos);
                    u32 Size = min(MAX_PATH, chunkSize);
                    Artist[Size-1] = '\0';
                    CharStr2WCharStr(Artist, OutSongData->SongMetadata.Artist, Size);
                    FoundMetaData |= 0x1;
                } else if (chunkId == fourccINAM) {
                    char Title[MAX_PATH];
                    ReadChunkData(FileHandle, Title, chunkSize, currentPos);
                    u32 Size = min(MAX_PATH, chunkSize);
                    Title[Size-1] = '\0';
                    CharStr2WCharStr(Title, OutSongData->SongMetadata.SongName, Size);
                    FoundMetaData |= 0x1 << 1;
                } else if (chunkId == fourccIPRD) {
                    char Album[MAX_PATH];
                    ReadChunkData(FileHandle, Album, chunkSize, currentPos);
                    u32 Size = min(MAX_PATH, chunkSize);
                    Album[Size-1] = '\0';
                    CharStr2WCharStr(Album, OutSongData->SongMetadata.Album, Size);
                    FoundMetaData |= 0x1 << 2;
                }

                currentPos += chunkSize;

                if (chunkSize % 2 != 0) { currentPos++; }
                bytesRead += sizeof(DWORD) * 2 + chunkSize;
            }
            if ((FoundMetaData & 1) == 0) { wcsncpy(OutSongData->SongMetadata.Artist, L"Unknown Artist", 14); }
            if ((FoundMetaData & 2) == 0) { wcsncpy(OutSongData->SongMetadata.SongName, L"Unknown", 7); }
            if ((FoundMetaData & 4) == 0) { wcsncpy(OutSongData->SongMetadata.Album, L"Unknown Album", 13); }


        }
    }

    return 0;
}

wchar_t* GetFileName(wchar_t* path) {
    wchar_t* lastSlash = wcsrchr(path, L'\\');
    wchar_t* lastForwardSlash = wcsrchr(path, L'/');

    wchar_t* lastSeparator = lastSlash > lastForwardSlash 
        ? lastSlash 
        : lastForwardSlash;

    return lastSeparator ? lastSeparator + 1 : path;
}

u8 AudioEngineInit(audio_engine_state* AudioEngine, platform_audio_engine** PlatformAudioEngine) {
    *PlatformAudioEngine = new platform_audio_engine();

    ::CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    ::XAudio2Create(&(*PlatformAudioEngine)->xAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);

    (*PlatformAudioEngine)->VoiceCallback = new voice_callback();

    (*PlatformAudioEngine)->xAudio2->CreateMasteringVoice(&(*PlatformAudioEngine)->xAudioMasteringVoice);

    // @NOTE: Not true
    AudioEngine->IsPlaying = true;
    AudioEngine->IsLooping = false;
    // @NOTE: Check 
    AudioEngine->CurrentSongIndex   = 0;
    AudioEngine->SongsCapacity      = 100;
    AudioEngine->SongsCount         = 0;
    AudioEngine->Songs = (song_data*)malloc(sizeof(song_data)*AudioEngine->SongsCapacity);
    AudioEngine->CurrentVolume = AUDIO_ENGINE_MAX_VOLUME;

    return 0;
}

// @NOTE: Maybe do it differently
u8 _UnloadFirstLoadedSongThatIsNotPlaying(audio_engine_state* AudioEngine) {
    for (u32 i = 0; i < AudioEngine->SongsCount; ++i) {
        song_data* SongData = AudioEngine->Songs+i;
        if (AudioEngine->CurrentSongIndex != i && SongData->AudioBufferIsLoaded) {
            return AudioEngineUnloadSongAudioBuffer(AudioEngine, i);
        }
    }
    return 1;
}

static i32 _LoadSongAudioBufferFromFile(wchar_t* File, song_data* OutSongData) {
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

XAUDIO2_BUFFER _GetXAudio2Buffer(const song_buffer* SongBuffer) {
    XAUDIO2_BUFFER AudioBuffer = {0};
    AudioBuffer.pAudioData  = SongBuffer->Data;
    AudioBuffer.Flags       = XAUDIO2_END_OF_STREAM;
    AudioBuffer.AudioBytes  = SongBuffer->BufferSize;

    return AudioBuffer;
}

// @NOTE: Aerials or 2nd song not working
WAVEFORMATEX _GetWaveFormatEx(const song_buffer* SongBuffer) {
    WAVEFORMATEX Wfx = {0};

#define WIN32_FORMAT_TAG_DEFAULT 1

    Wfx.wFormatTag      = WIN32_FORMAT_TAG_DEFAULT;
    Wfx.nChannels       = SongBuffer->Channels;
    Wfx.nSamplesPerSec  = SongBuffer->SamplesPerSec;
    Wfx.nAvgBytesPerSec = SongBuffer->NumAvgBytesPerSec;
    Wfx.nBlockAlign     = SongBuffer->BlockAlign;
    Wfx.wBitsPerSample  = SongBuffer->BitsPerSample;
    Wfx.cbSize          = 0;

    return Wfx;
}

u8 _AudioEngineReplaySong(audio_engine_state* AudioEngine, platform_audio_engine* PlatformAudioEngine) {
    song_data* SongData = AudioEngine->Songs+AudioEngine->CurrentSongIndex;

    PlatformAudioEngine->AudioBuffer = _GetXAudio2Buffer(&SongData->SongBufferData);

    PlatformAudioEngine->xAudio2SourceVoice->SubmitSourceBuffer(&PlatformAudioEngine->AudioBuffer);
    PlatformAudioEngine->xAudio2SourceVoice->Start(0);

    return 0;
}

u8 _AudioEnginePlaySong(audio_engine_state* AudioEngine, platform_audio_engine* PlatformAudioEngine) {
    song_data* SongData = AudioEngine->Songs+AudioEngine->CurrentSongIndex;

    if (PlatformAudioEngine->xAudio2SourceVoice != nullptr) {
        PlatformAudioEngine->xAudio2SourceVoice->DestroyVoice();
    }

    if (!SongData->AudioBufferIsLoaded) {
        _LoadSongAudioBufferFromFile(SongData->SongMetadata.FilePath, SongData);
    }

    PlatformAudioEngine->AudioBuffer = _GetXAudio2Buffer(&SongData->SongBufferData);
    WAVEFORMATEX Wfx = _GetWaveFormatEx(&SongData->SongBufferData);

    PlatformAudioEngine->xAudio2->CreateSourceVoice(&PlatformAudioEngine->xAudio2SourceVoice, (WAVEFORMATEX*)&Wfx, 0, XAUDIO2_DEFAULT_FREQ_RATIO, PlatformAudioEngine->VoiceCallback, NULL, NULL);

    PlatformAudioEngine->xAudio2SourceVoice->SubmitSourceBuffer(&PlatformAudioEngine->AudioBuffer);
    PlatformAudioEngine->xAudio2SourceVoice->Start(0);

    _UnloadFirstLoadedSongThatIsNotPlaying(AudioEngine);

    printf("Started playing %d\n", AudioEngine->CurrentSongIndex);
    return 0;
}

void AudioEngineUpdate(audio_engine_state* AudioEngine, platform_audio_engine* PlatformAudioEngine) {
    f32 FloatVolume = 1.0f * (AudioEngine->CurrentVolume - AUDIO_ENGINE_MIN_VOLUME) / (AUDIO_ENGINE_MAX_VOLUME - AUDIO_ENGINE_MIN_VOLUME);

    PlatformAudioEngine->xAudioMasteringVoice->SetVolume(FloatVolume);
    if (WaitForSingleObject(PlatformAudioEngine->VoiceCallback->hBufferEndEvent, 0) == WAIT_OBJECT_0) { 
        printf("I have awaited properly\n");
        if (AudioEngine->IsLooping) { _AudioEngineReplaySong(AudioEngine, PlatformAudioEngine); } 
        else { AudioEnginePlayNext(AudioEngine, PlatformAudioEngine); }
        printf("Started playing %d\n", AudioEngine->CurrentSongIndex);
        ResetEvent(PlatformAudioEngine->VoiceCallback->hBufferEndEvent);
    }
}

u8 AudioEnginePause(audio_engine_state* AudioEngine, platform_audio_engine* PlatformAudioEngine) {
    PlatformAudioEngine->xAudio2->StopEngine();
    AudioEngine->IsPlaying = false;
    return 0;
}

u8 AudioEnginePlay(audio_engine_state* AudioEngine, platform_audio_engine* PlatformAudioEngine) {
    PlatformAudioEngine->xAudio2->StartEngine();
    AudioEngine->IsPlaying = true;
    return 0;
}

u8 AudioEngineTogglePlayPause(audio_engine_state* AudioEngine, platform_audio_engine* PlatformAudioEngine) {
    if (AudioEngine->IsPlaying) { return AudioEnginePause(AudioEngine, PlatformAudioEngine); }
    else { return AudioEnginePlay(AudioEngine, PlatformAudioEngine); }
}



u8 AudioEngineUnloadSongAudioBuffer(audio_engine_state* AudioEngine, u32 SongIndex) {
    song_data* SongData = AudioEngine->Songs+SongIndex;
    if (SongData->AudioBufferIsLoaded) {
        SongData->AudioBufferIsLoaded = false; 
        free(SongData->SongBufferData.Data);
        SongData->SongBufferData = {0};
        AudioEngine->LoadedSongsCount--;
        printf("Unloaded song: %d\n", SongIndex);
    }

    return 0;
}

u8 AudioEngineLoadSongsFromFolder(audio_engine_state* AudioEngine, const wchar_t* Folder) {
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

    OutSongData->SongMetadata.DurationInSec = GetSongBufferDurationInSec(&OutSongData->SongBufferData);

    // @NOTE: Read file metadata
    wchar_t* FileName = GetFileName(const_cast<wchar_t*>(File));
    size_t Size = min(wcslen(FileName), MAX_PATH)*sizeof(wchar_t);
    Size = min(wcslen(File), MAX_PATH)*sizeof(wchar_t);
    memcpy_s(OutSongData->SongMetadata.FilePath, sizeof(wchar_t)*MAX_PATH, File, Size);
    OutSongData->SongMetadata.FilePath[Size-1] = '\0';

    return 0;
}

u8 AudioEngineLoadSong(audio_engine_state* AudioEngine, const wchar_t* SongName) {
    if (AudioEngine->SongsCount < AudioEngine->SongsCapacity) {
        song_data* SongData = &AudioEngine->Songs[AudioEngine->SongsCount++];
        if (AudioEngine->LoadedSongsCount <= AUDIO_ENGINE_MAX_LOADED_SONGS) {
            LoadSongDataFromFile(SongName, SongData, true);
            AudioEngine->LoadedSongsCount++;
        } else {
            LoadSongDataFromFile(SongName, SongData, false);
        }
    }

    return 0;
}

u8 AudioEnginePlaySongAtIndex(audio_engine_state* AudioEngine, platform_audio_engine* PlatformAudioEngine, u32 Index) {
    AudioEngine->CurrentSongIndex = Index;
    return _AudioEnginePlaySong(AudioEngine, PlatformAudioEngine);
}

u8 AudioEnginePlayNext(audio_engine_state* AudioEngine, platform_audio_engine* PlatformAudioEngine) {
    if (AudioEngine->SongsCount == 0) { return 1; }
    // @NOTE: Check
    if (++AudioEngine->CurrentSongIndex >= AudioEngine->SongsCount) {
        AudioEngine->CurrentSongIndex = 0;
    }

    return _AudioEnginePlaySong(AudioEngine, PlatformAudioEngine);
}

u8 AudioEnginePlayPrev(audio_engine_state* AudioEngine, platform_audio_engine* PlatformAudioEngine) {
    if (AudioEngine->SongsCount == 0) { return 1; }
    // @NOTE: Check
    if (AudioEngine->CurrentSongIndex == 0) {
        AudioEngine->CurrentSongIndex = AudioEngine->SongsCount - 1;
    } else { --AudioEngine->CurrentSongIndex; }

    return _AudioEnginePlaySong(AudioEngine, PlatformAudioEngine);
}


u64 GetSamplesPlayed(platform_audio_engine* PlatformAudioEngine) {
    u64 SamplesPlayed = 0;
    if (PlatformAudioEngine->xAudio2SourceVoice != nullptr) {
        XAUDIO2_VOICE_STATE State;
        PlatformAudioEngine->xAudio2SourceVoice->GetState(&State);
        SamplesPlayed = State.SamplesPlayed;
    }

    return SamplesPlayed;
}

i32 GetSongBufferDurationInSec(const song_buffer* SongBuffer) {
    return SongBuffer->BufferSize / SongBuffer->NumAvgBytesPerSec;
}

