#!/bin/sh

rm -rf clangd
mkdir -p clangd

make --always-make --dry-run |
  grep -wE 'gcc|g\+\+' |
  grep -w '\-c' |
  jq -nR '[inputs|{directory:".", command:., file: match(" [^ ]+$").string[1:]}]' \
    >clangd/compile_commands.json
