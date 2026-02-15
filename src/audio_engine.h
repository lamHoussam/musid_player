#pragma once

#include "types.h"
#include <stdio.h>
#include <windows.h>
#include <XAUDIO2.h>

#define MAX_BUFFER_COUNT    3
// @NOTE: More
#define BUFFER_SIZE         1024

struct song_header {
    i8  Artist[32];
    i8  Album[32];
    i8  Name[32];
    u32 SampleRate;
    u8  Channels;
};

struct voice_callback : public IXAudio2VoiceCallback {
public: 
    HANDLE hBufferEndEvent;
    voice_callback(): hBufferEndEvent(CreateEvent(NULL, FALSE, FALSE, NULL)) {}
    ~voice_callback() { CloseHandle(hBufferEndEvent); }

    void OnStreamEnd() { 
        OutputDebugStringA("On Stream End\n");
    }

    void OnVoiceProcessingPassEnd() { }
    void OnVoiceProcessingPassStart(UINT32 SamplesRequired) {    }
    void OnBufferEnd(void * pBufferContext) override { 
        OutputDebugStringA("On Buffer End\n");
        SetEvent(hBufferEndEvent);
    }
    void OnBufferStart(void * pBufferContext) {}
    void OnLoopEnd(void * pBufferContext) {}
    void OnVoiceError(void * pBufferContext, HRESULT Error) {
        OutputDebugStringA("Error on voice\n");
    }
};

struct audio_engine {
    voice_callback* VoiceCallback;

    XAUDIO2_BUFFER AudioBuffer;
    b8 IsPlaying;

    IXAudio2*               xAudio2{};
    IXAudio2SourceVoice*    xAudio2SourceVoice{};
    IXAudio2MasteringVoice* xAudioMasteringVoice{};
};

u8 AudioEngineInit(audio_engine* AudioEngine);
u8 AudioEnginePause(audio_engine* AudioEngine);
u8 AudioEnginePlay(audio_engine* AudioEngine);
u8 AudioEngineTogglePlayPause(audio_engine* AudioEngine);

