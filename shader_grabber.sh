#!/bin/bash

SHADER_DIR="data/shader"

vert=(`find $SHADER_DIR -name "*.vert"`)
frag=(`find $SHADER_DIR -name "*.frag"`)
comp=(`find $SHADER_DIR -name "*.comp"`)

shaders=( "${vert[@]}" "${frag[@]}" "${comp[@]}" )
for i in "${shaders[@]}"; do
    echo "$i";
done