#pragma once

#include "types.h"
#include <stdio.h>

#define MAX_BUFFER_COUNT    3
// @NOTE: More
#define BUFFER_SIZE                     1024
#define AUDIO_ENGINE_MAX_LOADED_SONGS   1

#define AUDIO_ENGINE_MAX_VOLUME         10
#define AUDIO_ENGINE_MIN_VOLUME         0

#define AUDIO_ENGINE_FILE_MAX_PATH 256


struct song_metadata {
    wchar_t Album[32];
    wchar_t SongName[AUDIO_ENGINE_FILE_MAX_PATH];
    wchar_t Artist[AUDIO_ENGINE_FILE_MAX_PATH];
    wchar_t FilePath[AUDIO_ENGINE_FILE_MAX_PATH];
    i32     DurationInSec;
};

struct song_buffer {
    u8* Data;
    u64 BufferSize;

    u16 Channels;
    u32 SamplesPerSec;
    u32 NumAvgBytesPerSec;
    u16 BlockAlign;
    u16 BitsPerSample;
};

struct song_data {
    song_buffer     SongBufferData;
    song_metadata   SongMetadata;
    b8              AudioBufferIsLoaded;
};

struct platform_audio_engine;
struct audio_engine_state {
    song_data*  Songs;
    u32         SongsCount;
    u32         SongsCapacity;
    u32         CurrentSongIndex;
    u32         LoadedSongsCount;
    i32         CurrentVolume;
    b8          IsPlaying;
    b8          IsLooping;
};

u8      AudioEngineInit(audio_engine_state* AudioEngine, platform_audio_engine** PlatformAudioEngine);
void    AudioEngineUpdate(audio_engine_state* AudioEngine, platform_audio_engine* PlatformAudioEngine);
u8      AudioEngineLoadSong(audio_engine_state* AudioEngine, const wchar_t* Song);
u8      AudioEngineUnloadSongAudioBuffer(audio_engine_state* AudioEngine, u32 SongIndex);
u8      AudioEngineLoadSongsFromFolder(audio_engine_state* AudioEngine, const wchar_t* Folder);

u8 AudioEnginePause(audio_engine_state* AudioEngine, platform_audio_engine* PlatformAudioEngine);
u8 AudioEnginePlay(audio_engine_state* AudioEngine, platform_audio_engine* PlatformAudioEngine);
u8 AudioEngineTogglePlayPause(audio_engine_state* AudioEngine, platform_audio_engine* PlatformAudioEngine);

u8 AudioEnginePlayNext(audio_engine_state* AudioEngine, platform_audio_engine* PlatformAudioEngine);
u8 AudioEnginePlayPrev(audio_engine_state* AudioEngine, platform_audio_engine* PlatformAudioEngine);
u8 AudioEnginePlaySongAtIndex(audio_engine_state* AudioEngine, platform_audio_engine* PlatformAudioEngine, u32 Index);
// @NOTE: Uninit platform audio engine

u64 GetSamplesPlayed(platform_audio_engine* PlatformAudioEngine);

i32 GetSongBufferDurationInSec(const song_buffer* SongBuffer);
