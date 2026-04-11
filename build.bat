@ECHO OFF

mkdir build
pushd build

REM -Zi Debug info
cl /LD ..\src\lh_crenderer.cpp user32.lib gdi32.lib /Fe:lh_crenderer.dll
cl -Zi -DWIN32=1 ..\src\main.cpp ..\src\win32_audio_engine.cpp ..\src\app.cpp /I..\src\lh_crenderer user32.lib gdi32.lib ole32.lib ws2_32.lib lh_crenderer.lib

popd

