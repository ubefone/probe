#!/bin/sh

set -e

export MRUBY_CONFIG=../build_config.rb
cd mruby
mkdir -p test
touch test/mrbtest.rake
# rake clean
rm -rf build/host
rm -rf build/main
rm -rf bin/*
rake
cp bin/d3probe ..
