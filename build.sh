#!/usr/bin/env bash

DIR=$(dirname $(realpath "$0"))
cd $DIR
set -ex

USE_PGXS=1 make
USE_PGXS=1 make install
