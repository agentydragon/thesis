#!/usr/bin/python3
# This script scanned the raw Firefox recording for hash tables with many
# operations and selected the top 10 hash tables with most operations
# into separate recordings.

import re
import collections

line_re = re.compile('^(Get|Put|Remove)Entry\(this=(0x[0-9a-f]*), key=([0-9a-f]*)\)$')
command_counts = collections.defaultdict(int)

CLEANED_FILE = '/mnt/data/prvak/hash-ops.cleaned'

# cnt=0
print('Looking for hash tables with most operations.')
for line in open(CLEANED_FILE, 'r'):
  result = line_re.match(line)
  thisptr = result.group(2)
  command_counts[thisptr] += 1
  # cnt += 1
  # if cnt == 100000:
  #   break

by_size = sorted(command_counts.keys(),
                 key=lambda thisptr: command_counts[thisptr])
largest = by_size[-10:]
for thisptr in largest:
  print(thisptr.rjust(20), command_counts[thisptr])

print('Selecting operations on these hashtables.')
files = {thisptr: open('/mnt/data/prvak/hash-ops.%s' % thisptr, 'w')
         for thisptr in largest}
# TODO: clean keys to have the same size.
for line in open(CLEANED_FILE, 'r'):
  result = line_re.match(line)
  operation = result.group(1)
  thisptr = result.group(2)
  key = result.group(3)
  operation = {'Get': 'find', 'Put': 'insert', 'Remove': 'delete'}[operation]

  if thisptr in largest:
    files[thisptr].write("%s %s\n" % (operation, key))
