#!/bin/sh

set -e

export MRUBY_CONFIG=../build_config.rb
cd mruby
mkdir -p test
touch test/mrbtest.rake
rake
cp bin/d3probe ..
