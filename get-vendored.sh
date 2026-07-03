#!/bin/sh

GITPATH=$(which git)
LIBSDL_GH="https://github.com/libsdl-org/"

mkdir vendored

"$GITPATH" clone "$LIBSDL_GH/SDL" vendored/SDL
"$GITPATH" clone "$LIBSDL_GH/SDL_mixer" vendored/SDL_mixer
"$GITPATH" clone "$LIBSDL_GH/SDL_image" vendored/SDL_image
"$GITPATH" clone "$LIBSDL_GH/SDL_ttf" vendored/SDL_ttf
"$GITPATH" clone "https://github.com/curl/curl" vendored/curl

