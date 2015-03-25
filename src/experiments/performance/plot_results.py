#!/usr/bin/python

import json
from matplotlib import pyplot

def select_metric_per_element(data, metric):
  return [point['metrics'][metric] / point['size'] for point in data]

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

figsize = (12, 12)
CACHED_DATA = None

def load_data(**filters):
  global CACHED_DATA
  if not CACHED_DATA:
    with open('results.json') as results_file:
      CACHED_DATA = json.load(results_file)
  def point_is_good(point):
    return all([point[key] == filters[key] for key in filters])
  return list(filter(point_is_good, CACHED_DATA))

def plot_all_experiments():
  for experiment, title in [
      ('word_frequency', 'Word frequency'),
      ('serial-both', 'Random insert+find'),
      ('serial-findonly', 'Random find'),
      ('ltr_scan', 'Left-to-right scan')]:
    plot_graph(data=load_data(experiment=experiment), title=title)
    pyplot.savefig(experiment + '.png', figsize=figsize)
    pyplot.clf()

  plot_graph(data=load_data(working_set_size=1000, experiment='workingset'),
             title='Random finds in 1k working set')
  pyplot.savefig('ws-1k-find.png', figsize=figsize)
  pyplot.clf()

  plot_graph(data=load_data(working_set_size=100000, experiment='workingset'),
             title='Random finds in 100k working set')
  pyplot.savefig('ws-100k-find.png', figsize=figsize)
  pyplot.clf()

def plot_ksplay_ltr_counters():
  data = load_data(implementation='dict_ksplay', experiment='ltr_scan')
  data = list(filter(lambda point: point['size'] > 1000, data))
  figure = pyplot.figure(1)
  pyplot.xscale('log')
  pyplot.ylabel('K-splay steps per element')
  pyplot.plot([point['size'] for point in data],
              [point['ksplay_steps'] / point['size'] for point in data], 'r-')
  pyplot.savefig('ltr-ksplays.png', figsize=figsize)
  pyplot.clf()

  figure = pyplot.figure(1)
  pyplot.xscale('log')
  pyplot.ylabel('K-splay composed keys per element')
  pyplot.plot([point['size'] for point in data],
              [point['ksplay_composed_keys'] / point['size'] for point in data],
              'r-')
  pyplot.savefig('ltr-composes.png', figsize=figsize)
  pyplot.clf()

def plot_pma_counters():
  data = load_data(implementation='dict_cobt', experiment='serial-both')
  data = list(filter(lambda point: point['size'] > 1000, data))
  figure = pyplot.figure(1)
  # figure.set_title('PMA reorganizations in random inserts')
  # figure.set_xscale('log')
  pyplot.xscale('log')
  pyplot.ylabel('Reorganizations per element')
  pyplot.plot([point['size'] for point in data],
              [point['pma_reorganized'] / point['size'] for point in data],
              'r-')
  pyplot.savefig('pma-reorg.png', figsize=figsize)
  pyplot.clf()

plot_mispredicts(data=load_data(experiment='serial-findonly'),
                 title='Mispredicts (random find)')
pyplot.savefig('random-mispredict.png', figsize=figsize)
pyplot.clf()

plot_pma_counters()
plot_ksplay_ltr_counters()

data = load_data(implementation='dict_cobt', experiment='serial-both')
data = list(filter(lambda point: point['size'] > 1000, data))
figure = pyplot.figure(1)
pyplot.xscale('log')
pyplot.ylabel('Cache misses per element')
for metric, color, label in [
    ('l1d_read_misses', 'r-', 'L1D read misses'),
    ('l1d_write_misses', 'r-', 'L1D write misses'),
    ('l1i_read_misses', 'b-', 'L1D read misses'),
    ('dtlb_read_misses', 'g-', 'dTLB read misses'),
    ('dtlb_write_misses', 'g-', 'dTLB write misses')]:
  pyplot.plot([point['size'] for point in data],
              select_metric_per_element(data, metric), color, label=label)
pyplot.legend(loc='upper left')
pyplot.grid(True)
pyplot.savefig('cobt-cache.png', figsize=figsize)
pyplot.clf()
