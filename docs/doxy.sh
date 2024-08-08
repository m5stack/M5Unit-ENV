#!/bin/bash

# Please execute on repositry root

## Get version from library.properties
## Get git rev of HEAD
LIB_VERSION="$(pcregrep -o1 "^\s*version\s*=\s*(\*|\d+(\.\d+){0,3}(\.\*)?)" library.properties)"
#echo ${DOXYGEN_PROJECT_NUMBER}
DOXYGEN_PROJECT_NUMBER="${LIB_VERSION} git rev:$(git rev-parse --short HEAD)" doxygen docs/Doxyfile


