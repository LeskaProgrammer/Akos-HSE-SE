#!/usr/bin/env bash
# Скрипт демонстрирует функцию, которая вызывает другую функцию.

square() {
    local x="$1"
    echo $((x * x))
}

process_number() {
    local num="$1"
    local sq
    sq=$(square "$num")
    echo "Число: $num, его квадрат: $sq"
}

NUM="${1:-3}"
process_number "$NUM"
