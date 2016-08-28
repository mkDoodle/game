@echo off

IF NOT EXIST ..\..\build mkdir ..\..\build

pushd ..\..\build

REM -subsystem:windows,5.1 for compatability winth windows XP *can be used when compiling in 32bit mode ONLY*

cl -MT -nologo -EHa- -GR- -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -DGAME_SLOW=1 -DGAME_INTERNAL=1 -Zi -FC -Fmwin32_game.map ..\game\code\win32_game.cpp /link -opt:ref  gdi32.lib user32.lib

popd