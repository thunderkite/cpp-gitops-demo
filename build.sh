#!/usr/bin/env bash
set -euo pipefail

VERSION="${1:?использование: ./build.sh v1}"
cd "$(dirname "$0")"

g++ -std=c++17 -O2 -static -DAPP_VERSION="\"${VERSION}\"" -o app/cpp-demo app/main.cpp

# docker доступен без sudo (пользователь в группе docker)
docker build -t "cpp-demo:${VERSION}" app/

# поток в containerd k3s — без временного файла и без sudo у docker
docker save "cpp-demo:${VERSION}" | sudo k3s ctr images import -

rm -f app/cpp-demo
