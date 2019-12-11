#!/bin/bash

for answer in answers/*.txt; do
  puzzle="puzzles/$(basename "$answer")"
  solver="bin/$(basename -s '.txt' "$answer")"
  printf "$solver..."
  if "$solver" "$puzzle" | colordiff --color=yes "$answer" - > /tmp/diff; then
    printf " \x1b[32mPASSED\x1b[0m\n"
  else
    printf " \x1b[31mFAILED\x1b[0m: (\x1b[31mwant\x1b[0m, \x1b[32mgot\x1b[0m)\n"
    cat /tmp/diff
  fi
done
