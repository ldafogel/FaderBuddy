#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../../.."
mkdir -p build
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined \
  -Ifirmware/src firmware/test/haptics/test.cpp -o build/test-haptics
build/test-haptics
