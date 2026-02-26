#pragma once

#include "types.h"
#include <stdio.h>
#include <windows.h>
#include <XAUDIO2.h>

#define MAX_BUFFER_COUNT    3
// @NOTE: More
#define BUFFER_SIZE                     1024
#define AUDIO_ENGINE_MAX_LOADED_SONGS   1

#define AUDIO_ENGINE_MAX_VOLUME         10
#define AUDIO_ENGINE_MIN_VOLUME         0

struct voice_callback;

struct song_data {
    XAUDIO2_BUFFER  AudioBuffer;
    WAVEFORMATEX    Wfx;
    u64             SongBufferSize;
    void*           SongBuffer;
    wchar_t         Album[32];
    wchar_t         SongName[MAX_PATH];
    wchar_t         Artist[MAX_PATH];
    wchar_t         FilePath[MAX_PATH];
    b8              AudioBufferIsLoaded;
};

struct audio_engine {
    voice_callback* VoiceCallback;
    b8              IsPlaying;

    IXAudio2*               xAudio2{};
    IXAudio2SourceVoice*    xAudio2SourceVoice{};
    IXAudio2MasteringVoice* xAudioMasteringVoice{};

    song_data*              Songs;
    u64                     SongsCount;
    u64                     SongsCapacity;
    u64                     CurrentSongIndex;
    u64                     LoadedSongsCount;
    i32                     CurrentVolume;
};

u8      AudioEngineInit(audio_engine* AudioEngine);
void    AudioEngineUpdate(audio_engine* AudioEngine);
u8      AudioEngineLoadSong(audio_engine* AudioEngine, const wchar_t* Song);
u8      AudioEngineUnloadSongAudioBuffer(audio_engine* AudioEngine, u64 SongIndex);
u8      AudioEngineLoadSongsFromFolder(audio_engine* AudioEngine, const wchar_t* Folder);

u8 AudioEnginePause(audio_engine* AudioEngine);
u8 AudioEnginePlay(audio_engine* AudioEngine);
u8 AudioEngineTogglePlayPause(audio_engine* AudioEngine);

u8 AudioEnginePlayNext(audio_engine* AudioEngine);
u8 AudioEnginePlayPrev(audio_engine* AudioEngine);
u8 AudioEnginePlaySongAtIndex(audio_engine* AudioEngine, u64 Index);

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

