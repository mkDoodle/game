@echo off

mkdir ..\..\build
pushd ..\..\build

cl  -WX -W4 -wd4201 -DGAME_SLOW=1 -DGAME_INTERNAL=1 -Zi -FC ..\game\code\win32_game.cpp gdi32.lib user32.lib

popd