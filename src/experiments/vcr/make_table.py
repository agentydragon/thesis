#!/usr/bin/python
import json
import os
import subprocess

implementations = ['dict_array', 'dict_rbtree', 'dict_btree', 'dict_cobt',
                   'dict_splay', 'dict_kforest', 'dict_ksplay', 'dict_htlp',
                   'dict_htcuckoo']

os.chdir('../../')

lines = []
identifiers = ['0x7f4791631c60', '0x7f47a6e31058', '0x7f566e3cb0e0',
               '0x7f567b64a830', '0x7fff84eb8f98', '0x7f47930300e0',
               '0x7f565ca63148', '0x7f5671faab80', '0x7fff1d9c7738',
               '0x7fff84eb9148']
for identifier in identifiers:
  recording_path = '../data/firefox-htable/hash-ops.' + identifier
  subprocess.check_call(['bin/experiments/vcr', '-p', recording_path,
                         '-a', ','.join(implementations),
                         '-r', '5'])
  with open('experiments/vcr/results.json') as results_file:
    results = json.load(results_file)

  cr = []
  for implementation in implementations:
    for point in results:
      if point['implementation'] == implementation:
        cr.append(point['time_ns'])
  m = max(cr)
  cr = [(x / m) * 100 for x in cr]
  lines.append(cr)

averages = [0 for point in lines[0]]
for line in lines:
  for i, point in enumerate(line):
    averages[i] += point
print(averages)
averages = [impl_sum / len(lines) for impl_sum in averages]

for i, line in enumerate(lines):
  l = ("\\texttt{%s}" % identifiers[i])
  minimum = min(line)

  for r in line:
    if r == minimum:
      l += (" & \\textbf{%d}" % r)
    else:
      l += (" & %d" % r)
  l += " \\\\\n\\hline"
  print(l)

l = '\\hline\nAverage'
for pt in averages:
  l += (" & %d" % pt)
print(l + ' \\')
