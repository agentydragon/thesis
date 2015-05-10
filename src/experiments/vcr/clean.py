#!/usr/bin/python3
# This small script was used to clean up unparseable lines from
# the raw recording. (There was no synchronization in the Firefox patch,
# so the lines frequently overlapped.)

import re

line_re = re.compile('^(Get|Put|Remove)Entry\(this=0x[0-9a-f]*, key=[0-9a-f]*\)$')
output = open('/mnt/data/prvak/hash-ops.cleaned', 'w')
broken = open('/mnt/data/prvak/hash-ops.broken', 'w')

skipped = 0
okay = 0

with open('/mnt/data/prvak/hash-ops', 'r') as ops_file:
  for line in ops_file:
    result = line_re.match(line)
    if result:
      output.write(line)
      okay += 1
    else:
      broken.write(line)
      skipped += 1

print("OK:", okay, "skipped:", skipped)
