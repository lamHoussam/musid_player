#include "audio_engine.h"

u8 AudioEngineInit(audio_engine* AudioEngine) {
    ::CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    ::XAudio2Create(&AudioEngine->xAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
    AudioEngine->xAudio2->CreateMasteringVoice(&AudioEngine->xAudioMasteringVoice);

    // @NOTE: Not true
    AudioEngine->IsPlaying = true;

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

