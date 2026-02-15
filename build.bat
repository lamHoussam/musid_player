@ECHO OFF

mkdir build
pushd build

REM -Zi Debug info
cl /LD ..\src\lh_crenderer.cpp user32.lib gdi32.lib /Fe:lh_crenderer.dll
cl -Zi ..\src\main.cpp ..\src\audio_engine.cpp /I..\src\lh_crenderer user32.lib gdi32.lib ole32.lib ws2_32.lib lh_crenderer.lib

popd

