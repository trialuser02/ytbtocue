#!/bin/sh

MAJOR=`cat src/utils.h | grep "#define YTBTOCUE_VERSION_MAJOR" | cut -d " " -f3`
MINOR=`cat src/utils.h | grep "#define YTBTOCUE_VERSION_MINOR" | cut -d " " -f3`

VERSION=$MAJOR.$MINOR

NAME=ytbtocue

TARBALL=$NAME-$VERSION

mkdir -p extras/package/sources
git archive --format=tar --prefix=$TARBALL/ $VERSION | xz > extras/package/sources/$TARBALL.tar.xz
