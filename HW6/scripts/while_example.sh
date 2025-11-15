#!/usr/bin/env bash
# Скрипт выводит числа от 1 до N, используя цикл while.

N="${1:-5}"
COUNTER=1

while [[ $COUNTER -le $N ]]; do
    echo "Счётчик: $COUNTER"
    ((COUNTER++))
done
