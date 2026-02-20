#include "app.h"

int main() {
    // @NOTE: Data init, needs to be somewhere else
    // AudioEngine.xAudio2SourceVoice->Start(0);
    // AudioEngineStartSong(&NetworkAudioEngine, WindowHandle, &SongHeader);

    app App;
    app_init(&App);
    app_run(&App);
    return 0;
}

