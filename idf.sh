#!/usr/bin/env bash
# Runs idf.py with ESP-IDF v6.0.2 loaded, from any shell: ./idf.sh build, ./idf.sh flash monitor, ...
set -e
export IDF_PYTHON_ENV_PATH="${IDF_PYTHON_ENV_PATH:-$HOME/.espressif/python_env/idf6.0_py3.14_env}"
source "${IDF_PATH:-$HOME/esp/esp-idf}/export.sh" > /dev/null
exec idf.py "$@"
