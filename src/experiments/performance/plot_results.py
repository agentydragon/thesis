#!/usr/bin/python

import json
from matplotlib import pyplot

def select_api(data, api):
  return [point for point in data if point['implementation'] == api]

def select_metric_per_element(data, metric):
  return [point['metrics'][metric] / point['size'] for point in data]

def mentioned_apis(data):
  return sorted(set(point['implementation'] for point in data))

def apis_with_colors(data):
  return zip(mentioned_apis(data),
             ['r-', 'g-', 'b-', 'c-', 'm-', 'y-', 'k-'])

def plot_graph(data, title):
  def sizes(api_data):
    return [point['size'] for point in api_data]
  def times(api_data):
    return [point['time_ns'] / point['size'] for point in api_data]
  def misses(api_data):
    return [point['metrics']['cache_misses'] / point['size'] for point in api_data]

  pyplot.figure(1)
  subplot = pyplot.subplot(211)
  subplot.set_title(title)
  subplot.set_xscale('log')
  for api, color in apis_with_colors(data):
    api_data = select_api(data, api)
    pyplot.plot(sizes(api_data), times(api_data), color, linewidth=2.0,
                label=api)
  pyplot.legend(loc='upper left')
  pyplot.ylabel('Time per key [ns]')
  pyplot.grid(True)

  subplot = pyplot.subplot(212)
  subplot.set_xscale('log')
  for api, color in apis_with_colors(data):
    api_data = select_api(data, api)
    pyplot.plot(sizes(api_data), misses(api_data), color, linewidth=1.0,
                label=api)
  pyplot.legend(loc='upper left')
  pyplot.ylabel('Cache misses per key')
  pyplot.xlabel('Inserted items')
  pyplot.grid(True)

def plot_mispredicts(data, title):
  def sizes(api_data):
    return [point['size'] for point in api_data if point['metrics']['branches']]
  def mispredicts(api_data):
    result = []
    for point in api_data:
      if point['metrics']['branches']:
        result.append(point['metrics']['branch_mispredicts'] /
                      point['metrics']['branches'])
    return result

  pyplot.figure(1)
  subplot = pyplot.subplot(211)
  subplot.set_title(title)
  subplot.set_xscale('log')
  for api, color in apis_with_colors(data):
    api_data = select_api(data, api)
    pyplot.plot(sizes(api_data), mispredicts(api_data), color, linewidth=2.0,
                label=api)
  pyplot.legend(loc='upper left')
  pyplot.ylabel('Branch mispredict ratio')
  # pyplot.xlabel('Inserted items')
  pyplot.grid(True)

CACHED_DATA_FROM_FILE = None

def save_to(filename):
  pyplot.savefig('output/' + filename, figsize=(12, 12))

def point_fits_filters(point, filters):
  for key in filters:
    if key in point:
      if point[key] != filters[key]:
        return False
    elif key in point['metrics']:
      if point['metrics'][key] != filters[key]:
        return False
    else:
      return False
  return True

def load_data(discard_trivial=True, **filters):
  global CACHED_DATA_FROM_FILE

  if not CACHED_DATA_FROM_FILE:
    with open('output/results.json') as results_file:
      CACHED_DATA_FROM_FILE = json.load(results_file)

  def point_is_good(point):
    if discard_trivial and point['size'] < 1000:
      return False
    return point_fits_filters(point, filters)

  return list(filter(point_is_good, CACHED_DATA_FROM_FILE))

def plot_all_experiments():
  for experiment, title in [
      ('word_frequency', 'Word frequency'),
      ('serial-both', 'Random insert+find'),
      ('serial-findonly', 'Random find'),
      ('ltr_scan', 'Left-to-right scan')]:
    plot_graph(data=load_data(experiment=experiment), title=title)
    save_to(experiment + '.png')
    pyplot.clf()

  plot_graph(data=load_data(working_set_size=1000, experiment='workingset'),
             title='Random finds in 1k working set')
  save_to('ws-1k-find.png')
  pyplot.clf()

  plot_graph(data=load_data(working_set_size=100000, experiment='workingset'),
             title='Random finds in 100k working set')
  save_to('ws-100k-find.png')
  pyplot.clf()

def plot_ksplay_ltr_counters():
  data = load_data(implementation='dict_ksplay', experiment='ltr_scan')

  pyplot.figure(1)
  pyplot.xscale('log')
  pyplot.ylabel('K-splay steps per element')
  pyplot.plot([point['size'] for point in data],
              [point['ksplay_steps'] / point['size'] for point in data], 'r-')
  save_to('ltr-ksplays.png')
  pyplot.clf()

  pyplot.figure(1)
  pyplot.xscale('log')
  pyplot.ylabel('K-splay composed keys per element')
  pyplot.plot([point['size'] for point in data],
              [point['ksplay_composed_keys'] / point['size'] for point in data],
              'r-')
  save_to('ltr-composes.png')
  pyplot.clf()

def plot_pma_counters():
  data = load_data(implementation='dict_cobt', experiment='serial-both')
  figure = pyplot.figure(1)
  # figure.set_title('PMA reorganizations in random inserts')
  # figure.set_xscale('log')
  pyplot.xscale('log')
  pyplot.ylabel('Reorganizations per element')
  pyplot.plot([point['size'] for point in data],
              [point['pma_reorganized'] / point['size'] for point in data],
              'r-')
  save_to('pma-reorg.png')
  pyplot.clf()

def main():
  plot_all_experiments()

  plot_mispredicts(data=load_data(experiment='serial-findonly'),
                   title='Mispredicts (random find)')
  save_to('random-mispredict.png')
  pyplot.clf()

  plot_pma_counters()
  plot_ksplay_ltr_counters()

  data = load_data(implementation='dict_cobt', experiment='serial-both')
  pyplot.figure(1)
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
  save_to('cobt-cache.png')
  pyplot.clf()

main()
