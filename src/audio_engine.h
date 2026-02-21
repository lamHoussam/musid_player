#pragma once

#include "types.h"
#include <stdio.h>
#include <windows.h>
#include <XAUDIO2.h>

#define MAX_BUFFER_COUNT    3
// @NOTE: More
#define BUFFER_SIZE         1024

struct voice_callback : public IXAudio2VoiceCallback {
public: 
    HANDLE hBufferEndEvent;
    voice_callback(): hBufferEndEvent(CreateEvent(NULL, FALSE, FALSE, NULL)) {}
    ~voice_callback() { CloseHandle(hBufferEndEvent); }

    void OnStreamEnd() { 
        printf("On Stream End\n");
    }

    void OnVoiceProcessingPassEnd() { }
    void OnVoiceProcessingPassStart(UINT32 SamplesRequired) {    }
    void OnBufferEnd(void * pBufferContext) override { 
        printf("On Buffer End\n");
        SetEvent(hBufferEndEvent);
    }
    void OnBufferStart(void * pBufferContext) {}
    void OnLoopEnd(void * pBufferContext) {}
    void OnVoiceError(void * pBufferContext, HRESULT Error) {
        printf("Error on voice\n");
    }
};

struct song_data {
    WAVEFORMATEX    Wfx;
    XAUDIO2_BUFFER  AudioBuffer;
    wchar_t         SongName[32];
    wchar_t         Artist[32];
    wchar_t         Album[32];
    wchar_t         FilePath[MAX_PATH];
};

struct audio_engine {
    voice_callback* VoiceCallback;
    XAUDIO2_BUFFER  AudioBuffer;
    b8              IsPlaying;

    IXAudio2*               xAudio2{};
    IXAudio2SourceVoice*    xAudio2SourceVoice{};
    IXAudio2MasteringVoice* xAudioMasteringVoice{};

    song_data*              Songs;
    u64                     SongsCount;
    u64                     SongsCapacity;
    u64                     CurrentSongIndex;
};

u8 AudioEngineInit(audio_engine* AudioEngine);
u8 AudioEngineLoadSong(audio_engine* AudioEngine, const wchar_t* Song);
u8 AudioEngineLoadSongsFromFolder(audio_engine* AudioEngine, const wchar_t* Folder);

u8 AudioEnginePause(audio_engine* AudioEngine);
u8 AudioEnginePlay(audio_engine* AudioEngine);
u8 AudioEngineTogglePlayPause(audio_engine* AudioEngine);

u8 AudioEnginePlayNext(audio_engine* AudioEngine);
u8 AudioEnginePlayPrev(audio_engine* AudioEngine);
