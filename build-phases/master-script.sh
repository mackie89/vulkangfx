#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

bash $DIR/compile-shaders.sh
bash $DIR/copy-shaders.sh
bash $DIR/copy-textures.sh
bash $DIR/copy-models.sh
