#!/bin/bash

make -q opt || make opt
for answer in answers/*.txt; do
  puzzle="puzzles/$(basename "$answer")"
  solver="bin/opt/$(basename -s '.txt' "$answer")"
  begin="$(date +'%s%N')"
  "$solver" "$puzzle" > /tmp/out
  end="$(date +'%s%N')"
  runtime_millis=$(((end - begin) / 1000000))
  printf "$solver..."
  if colordiff --color=yes "$answer" /tmp/out > /tmp/diff; then
    printf " \x1b[32mPASSED\x1b[0m"
  else
    printf " \x1b[31mFAILED\x1b[0m: (\x1b[31mwant\x1b[0m, \x1b[32mgot\x1b[0m)"
    cat /tmp/diff
  fi
  printf " in \x1b[33m%4dms\x1b[0m\n" "$runtime_millis"
done
