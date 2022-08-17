#!/usr/bin/env bash

set -ex

# using docker volumes to store data, fix permissions

if [ ! -d "/home/vscode/.conan" ]; then
    sudo mkdir /home/vscode/.conan && chown -R vscode.vscode /home/vscode/.conan
elif [ $(stat --format '%U' "/home/vscode/.conan") = "root" ]; then
    echo "WARNING: fix 'root' permissions for conan"
    sudo chown -R vscode.vscode /home/vscode/.conan
fi

if [ ! -d "/home/vscode/.cache" ]; then
    sudo mkdir /home/vscode/.cache && chown -R vscode.vscode /home/vscode/.cache
elif [ $(stat --format '%U' "/home/vscode/.cache") = "root" ]; then
    echo "WARNING: fix 'root' permissions for ccache"
    sudo chown -R vscode.vscode /home/vscode/.cache
fi

if [ $(stat --format '%U' "/workspaces") = "root" ]; then
    echo "WARNING: fix 'root' permissions for workspaces folder"
    sudo chown -R vscode.vscode /workspaces
fi

export CONAN_V2_MODE=1

pip3 --disable-pip-version-check --no-cache-dir install wheel conan

# g++ is using new C++11 ABI (libstdc++11) with in Conan v2 mode
# check: https://github.com/conan-io/conan/issues/11881
CXX=clang++ conan profile new --detect --force clang
CXX=g++ conan profile new --detect --force gcc
