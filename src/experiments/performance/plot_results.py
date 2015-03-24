#!/usr/bin/python

import json
from matplotlib import pyplot

def load_results():
  with open('results.json') as results_file:
    return json.load(results_file)

def plot_graph(data, title):
  data = list(filter(lambda point: point['size'] > 1000, data))
  apis = sorted(set(point['implementation'] for point in data))
  colors = ['r-', 'g-', 'b-', 'c-', 'm-', 'y-', 'k-']

  def sizes(api):
    return [point['size'] for point in data
                          if point['implementation'] == api]
  def times(api):
    return [point['time_ns'] / point['size']
            for point in data if point['implementation'] == api]
  def misses(api):
    return [point['metrics']['cache_misses'] / point['size']
            for point in data if point['implementation'] == api]

  figure = pyplot.figure(1)
  subplot = pyplot.subplot(211)
  subplot.set_title(title)
  subplot.set_xscale('log')
  for api, color in zip(apis, colors):
    pyplot.plot(sizes(api), times(api), color, linewidth=2.0, label=api)
  pyplot.legend(loc='upper left')
  pyplot.ylabel('Time per key [ns]')
  # pyplot.xlabel('Inserted items')
  pyplot.grid(True)

  subplot = pyplot.subplot(212)
  # subplot.set_title(title)
  subplot.set_xscale('log')
  for api, color in zip(apis, colors):
    pyplot.plot(sizes(api), misses(api), color, linewidth=1.0, label=api)
  pyplot.legend(loc='upper left')
  pyplot.ylabel('Cache misses per key')
  pyplot.xlabel('Inserted items')
  pyplot.grid(True)

def plot_mispredicts(data, title):
  data = list(filter(lambda point: point['size'] > 1000, data))
  apis = sorted(set(point['implementation'] for point in data))
  colors = ['r-', 'g-', 'b-', 'c-', 'm-', 'y-', 'k-']

  def sizes(api):
    return [point['size'] for point in data
                          if point['implementation'] == api and
                             point['metrics']['branches']]
  def mispredicts(api):
    result = []
    for point in data:
      if point['implementation'] == api and point['metrics']['branches']:
        result.append(point['metrics']['branch_mispredicts'] /
                      point['metrics']['branches'])
    return result

  figure = pyplot.figure(1)
  subplot = pyplot.subplot(211)
  subplot.set_title(title)
  subplot.set_xscale('log')
  for api, color in zip(apis, colors):
    pyplot.plot(sizes(api), mispredicts(api), color, linewidth=2.0, label=api)
  pyplot.legend(loc='upper left')
  pyplot.ylabel('Branch mispredict ratio')
  # pyplot.xlabel('Inserted items')
  pyplot.grid(True)

def filter_experiment(data, experiment):
  return list(filter(lambda point: point['experiment'] == experiment, data))

data = load_results()
figsize = (12, 12)

plot_graph(data=filter_experiment(data, experiment='word_frequency'),
            title='Word frequency')
pyplot.savefig('word-frequency.png', figsize=figsize)
pyplot.clf()

plot_graph(data=filter_experiment(data, experiment='serial-both'),
            title='Random insert+find')
pyplot.savefig('random-insert-find.png', figsize=figsize)
pyplot.clf()

plot_graph(data=filter_experiment(data, experiment='serial-findonly'),
            title='Random find')
pyplot.savefig('random-find.png', figsize=figsize)
pyplot.clf()

plot_graph(data=filter_experiment(data, experiment='ltr_scan'),
            title='Left-to-right scan')
pyplot.savefig('ltr-scan.png', figsize=figsize)
pyplot.clf()

plot_graph(list(filter(lambda point: point['working_set_size'] == 1000,
                        filter_experiment(data, experiment='workingset'))),
            title='Random finds in 1k working set')
pyplot.savefig('ws-1k-find.png', figsize=figsize)
pyplot.clf()

plot_graph(list(filter(lambda point: point['working_set_size'] == 100000,
                        filter_experiment(data, experiment='workingset'))),
            title='Random finds in 100k working set')
pyplot.savefig('ws-100k-find.png', figsize=figsize)
pyplot.clf()

plot_mispredicts(data=filter_experiment(data, experiment='serial-findonly'),
                 title='Mispredicts (random find)')
pyplot.savefig('random-mispredict.png', figsize=figsize)
pyplot.clf()

data = load_results()
data = list(filter(lambda point: point['size'] > 1000 and
                                 point['implementation'] == 'dict_cobt' and
                                 point['experiment'] == 'serial-both',
                   data))
figure = pyplot.figure(1)
# figure.set_title('PMA reorganizations in random inserts')
# figure.set_xscale('log')
pyplot.xscale('log')
pyplot.ylabel('Reorganizations per element')
pyplot.plot([point['size'] for point in data],
            [point['pma_reorganized'] / point['size'] for point in data], 'r-')
pyplot.savefig('pma-reorg.png', figsize=figsize)
pyplot.clf()

data = load_results()
data = list(filter(lambda point: point['size'] > 1000 and
                                 point['implementation'] == 'dict_ksplay' and
                                 point['experiment'] == 'ltr_scan',
                   data))
figure = pyplot.figure(1)
# figure.set_title('PMA reorganizations in random inserts')
# figure.set_xscale('log')
pyplot.xscale('log')
pyplot.ylabel('K-splay steps per element')
pyplot.plot([point['size'] for point in data],
            [point['ksplay_steps'] / point['size'] for point in data], 'r-')
pyplot.savefig('ltr-ksplays.png', figsize=figsize)
pyplot.clf()


data = load_results()
data = list(filter(lambda point: point['size'] > 1000 and
                                 point['implementation'] == 'dict_ksplay' and
                                 point['experiment'] == 'ltr_scan',
                   data))
figure = pyplot.figure(1)
# figure.set_title('PMA reorganizations in random inserts')
# figure.set_xscale('log')
pyplot.xscale('log')
pyplot.ylabel('K-splay composed keys per element')
pyplot.plot([point['size'] for point in data],
            [point['ksplay_composed_keys'] / point['size'] for point in data], 'r-')
pyplot.savefig('ltr-composes.png', figsize=figsize)
pyplot.clf()
