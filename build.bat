@echo off

mkdir ..\..\build
pushd ..\..\build

cl -MT -nologo -EHa- -GR- -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -DGAME_SLOW=1 -DGAME_INTERNAL=1 -Zi -FC ..\game\code\win32_game.cpp /link -subsystem:windows,5.1 gdi32.lib user32.lib

popd