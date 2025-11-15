#!/usr/bin/env bash
# Скрипт дважды запускает child_script.sh с разными аргументами.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

"$SCRIPT_DIR/child_script.sh" "первый_запуск"
"$SCRIPT_DIR/child_script.sh" "второй_запуск"