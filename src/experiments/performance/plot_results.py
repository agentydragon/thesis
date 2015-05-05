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
             ['r-', 'g-', 'b-', 'c-', 'm-', 'y-', 'k-', 'rx', 'gx', 'bx'])

FIGURE_SIZE = (20,20)

def new_figure():
  return pyplot.figure(1, figsize=FIGURE_SIZE)

def plot_graph(data, title=None):
  def sizes(api_data):
    return [point['size'] for point in api_data]
  def times(api_data):
    return [point['time_ns'] / point['size'] for point in api_data]
  def misses(api_data):
    return [point['metrics']['cache_misses'] / point['size'] for point in api_data]

  new_figure()
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

  new_figure()
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
  pyplot.savefig('output/' + filename)

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

  new_figure()
  pyplot.xscale('log')
  pyplot.ylabel('K-splay steps per element')
  pyplot.plot([point['size'] for point in data],
              [point['ksplay_steps'] / point['size'] for point in data], 'r-')
  save_to('ltr-ksplays.png')
  pyplot.clf()

  new_figure()
  pyplot.xscale('log')
  pyplot.ylabel('K-splay composed keys per element')
  pyplot.plot([point['size'] for point in data],
              [point['ksplay_composed_keys'] / point['size'] for point in data],
              'r-')
  save_to('ltr-composes.png')
  pyplot.clf()

def plot_cuckoo_counters():
  data = load_data(implementation='dict_htcuckoo', experiment='serial-both')
  figure = new_figure()
  pyplot.xscale('log')
  # pyplot.ylabel('Full rehashes per insert')
  pyplot.ylabel('Cost of full rehashes per insert')
  pyplot.plot([point['size'] for point in data],
              [point['cuckoo_full_rehashes'] for point in data],
  #            [point['cuckoo_full_rehashes'] / point['size'] for point in data],
              'r-')
  save_to('cuckoo-full-rehashes.png')
  pyplot.clf()

  figure = new_figure()
  pyplot.xscale('log')
  pyplot.ylabel('Traversed edges per insert')
  pyplot.plot([point['size'] for point in data],
              [point['cuckoo_traversed_edges'] / point['size'] for point in data],
              'r-')
  save_to('cuckoo-traversed-edges.png')
  pyplot.clf()

def plot_pma_counters():
  data = load_data(implementation='dict_cobt', experiment='serial-both')
  figure = new_figure()
  # figure.set_title('PMA reorganizations in random inserts')
  # figure.set_xscale('log')
  pyplot.xscale('log')
  pyplot.ylabel('Reorganizations per element')
  pyplot.plot([point['size'] for point in data],
              [point['pma_reorganized'] / point['size'] for point in data],
              'r-')
  save_to('pma-reorg.png')
  pyplot.clf()

def plot_cache_events(implementation, experiment):
  data = load_data(implementation=implementation, experiment=experiment)
  new_figure()
  pyplot.xscale('log')
  pyplot.ylabel('Cache misses per element')
  for metric, color, label in [
      ('l1d_read_misses', 'r-', 'L1D read misses'),
      ('l1d_write_misses', 'r-', 'L1D write misses'),
      ('l1i_read_misses', 'b-', 'L1I read misses'),
      ('dtlb_read_misses', 'g-', 'dTLB read misses'),
      ('dtlb_write_misses', 'g-', 'dTLB write misses')]:
    pyplot.plot([point['size'] for point in data],
                select_metric_per_element(data, metric), color, label=label)
  pyplot.legend(loc='upper left')
  pyplot.grid(True)

def select_time_per_op(data):
  return [point['time_ns'] / point['size'] for point in data]

#HASH_LABEL = 'Hash table with linear probing (for reference)'
HASH_LABEL = 'Hash table with linear probing'
EXPORT_FIGSIZE = (6,6)

def plot_cob_find_speed_figures():
  def find_speed_fig_internal(data):
    # Random FINDs: cache-oblivious B-tree vs. B-tree vs. linear probing HT
    pyplot.xscale('log')
    pyplot.xlabel('Dictionary size')
    pyplot.ylabel('Time per operation [ns]')
    pyplot.grid(True)

    api_data = [point for point in data if point['implementation'] == 'dict_btree']
    pyplot.plot([point['size'] for point in api_data],
                select_time_per_op(api_data),
                'r', linewidth=2.0, label='B-tree')

    api_data = [point for point in data if point['implementation'] == 'dict_cobt']
    pyplot.plot([point['size'] for point in api_data],
                select_time_per_op(api_data),
                'g', linewidth=2.0, label='Cache-oblivious B-tree')

    api_data = [point for point in data if point['implementation'] == 'dict_htlp']
    if api_data:
      pyplot.plot([point['size'] for point in api_data],
                  select_time_per_op(api_data),
                  '#666666', linewidth=1.0, label=HASH_LABEL,
                  linestyle='dashed')

    pyplot.legend(loc='upper left')
  def find_speed_fig(**kwargs):
    data = load_data(**kwargs)
    data = [point for point in data
            if point['implementation'] in ['dict_btree', 'dict_cobt',
                                           'dict_htlp']]
    find_speed_fig_internal(data)

  pyplot.figure(1, figsize=EXPORT_FIGSIZE)
  find_speed_fig(experiment='serial-findonly')
  save_to('export/cob-performance-1.png')
  pyplot.clf()

  pyplot.figure(1, figsize=EXPORT_FIGSIZE)
  find_speed_fig(experiment='workingset', working_set_size=1000)
  save_to('export/cob-performance-3.png')
  pyplot.clf()

  pyplot.figure(1, figsize=EXPORT_FIGSIZE)
  find_speed_fig(experiment='workingset', working_set_size=100000)
  save_to('export/cob-performance-4.png')
  pyplot.clf()

  pyplot.figure(1, figsize=EXPORT_FIGSIZE)
  find_speed_fig(experiment='ltr_scan')
  pyplot.ylabel('Time per scanned key [ns]')
  save_to('export/cob-performance-5.png')
  pyplot.clf()

  pyplot.figure(1, figsize=EXPORT_FIGSIZE)
  data = load_data(experiment='word_frequency')
  data = [point for point in data
          if point['implementation'] in ['dict_btree', 'dict_cobt',
                                         'dict_htlp']]
  data = [point for point in data if point['size'] <= 800 * 1000]
  find_speed_fig_internal(data)
  pyplot.ylabel('Time per indexed word')
  pyplot.xlabel('Indexed words')
  save_to('export/cob-performance-6.png')
  pyplot.clf()

def get_difference(api):
  find_pts = load_data(implementation=api, experiment='serial-findonly')
  both_pts = load_data(implementation=api, experiment='serial-both')
  find_pts = {point['size']: point['time_ns'] for point in find_pts}
  both_pts = {point['size']: point['time_ns'] for point in both_pts}
  difference = [(size, both_pts[size] - find_pts[size]) for size in find_pts]

  # hack against mismeasurements
  difference = [(size, diff) for size, diff in difference if diff > 0]

  return sorted(difference, key=lambda x: x[0])

def plot_cob_performance():
  plot_cob_find_speed_figures()

  # Random INSERTs: cache-oblivious B-tree vs. B-tree vs. linear probing HT
  # (Derived.)
  pyplot.figure(1, figsize=EXPORT_FIGSIZE)
  pyplot.xscale('log')
  pyplot.xlabel('Dictionary size')
  pyplot.ylabel('Time per operation [ns]')
  pyplot.grid(True)

  difference = get_difference('dict_btree')
  pyplot.plot([size for size, _ in difference],
              [time / size for size, time in difference],
              'r', linewidth=2.0, label='B-tree')
  difference = get_difference('dict_cobt')
  pyplot.plot([size for size, _ in difference],
              [time / size for size, time in difference],
              'g', linewidth=2.0, label='Cache-oblivious B-tree')
  difference = get_difference('dict_htlp')
  pyplot.plot([size for size, _ in difference],
              [time / size for size, time in difference],
              '#666666', linewidth=1.0, label=HASH_LABEL,
              linestyle='dashed')

  pyplot.legend(loc='upper left')
  save_to('export/cob-performance-2.png')
  pyplot.clf()

  pyplot.figure(1, figsize=(10,10))
  def sizes(api_data):
    return [point['size'] for point in api_data]
  def times(api_data):
    return [point['time_ns'] / point['size'] for point in api_data]

  data = load_data(experiment='serial-findonly')
  new_figure()
  for api, color, label in [
      ('dict_btree', 'r-', 'B-tree'),
      ('dict_cobt', 'g-', 'Cache-oblivious B-tree'),
      ('dict_htcuckoo', 'b-', 'Cuckoo hash table'),
      ('dict_htlp', 'c-', 'Hash table with linear probing'),
      ('dict_kforest', 'k-', 'K-forest'),
      ('dict_ksplay', 'y-', 'K-splay tree'),
      ('dict_splay', 'm-', 'Splay tree')]:
    api_data = select_api(data, api)
    pyplot.plot(sizes(api_data), times(api_data), color, linewidth=2.0,
                label=label)
  pyplot.xscale('log')
  pyplot.legend(loc='upper left')
  pyplot.ylabel('Time per operation [ns]')
  pyplot.grid(True)
  save_to('export/overall-random-find.png')
  pyplot.clf()

def plot_self_adjusting_performance():
  def sizes(api_data):
    return [point['size'] for point in api_data]
  def times(api_data):
    return [point['time_ns'] / point['size'] for point in api_data]
  def plot_self_adj(**kwargs):
    pyplot.figure(1, figsize=(10,10))
    data = load_data(**kwargs)
    new_figure()
    for api, color, label in [
        ('dict_splay', 'r-', 'Splay tree'),
        ('dict_ksplay', 'g-', 'K-splay tree'),
        ('dict_kforest', 'b-', 'K-forest')]:
      api_data = select_api(data, api)
      pyplot.plot(sizes(api_data), times(api_data), color, linewidth=2.0,
                  label=label)

    api_data = select_api(data, 'dict_btree')
    pyplot.plot(sizes(api_data), times(api_data), '#666666', linewidth=1.0,
                linestyle='dashed', label='B-tree')

    pyplot.xscale('log')
    pyplot.legend(loc='upper left')
    pyplot.ylabel('Time per operation [ns]')
    pyplot.grid(True)

  plot_self_adj(experiment='serial-findonly')
  save_to('export/self-adj-random-find.png')
  pyplot.clf()

  plot_self_adj(experiment='serial-insertonly')
  save_to('export/self-adj-random-insert.png')
  pyplot.clf()

  plot_self_adj(experiment='workingset', working_set_size=1000)
  save_to('export/self-adj-ws-1k.png')
  pyplot.clf()

  plot_self_adj(experiment='workingset', working_set_size=100000)
  save_to('export/self-adj-ws-100k.png')
  pyplot.clf()

def plot_hashing_performance():
  def hashing_figure(**kwargs):
    data = load_data(**kwargs)
    data = [point for point in data
            if point['implementation'] in ['dict_htlp', 'dict_htcuckoo', 'dict_btree']]
    pyplot.figure(1, figsize=EXPORT_FIGSIZE)
    pyplot.xscale('log')
    pyplot.xlabel('Dictionary size')
    pyplot.ylabel('Time per operation [ns]')
    pyplot.grid(True)

    api_data = [point for point in data if point['implementation'] == 'dict_htlp']
    pyplot.plot([point['size'] for point in api_data],
                select_time_per_op(api_data),
                'r', linewidth=2.0, label='Linear probing')

    api_data = [point for point in data if point['implementation'] == 'dict_htcuckoo']
    pyplot.plot([point['size'] for point in api_data],
                select_time_per_op(api_data),
                'g', linewidth=2.0, label='Cuckoo hashing')

    api_data = [point for point in data if point['implementation'] == 'dict_btree'
                                        and point['time_ns'] / point['size'] < 500]
    pyplot.plot([point['size'] for point in api_data],
                select_time_per_op(api_data),
                '#666666', linewidth=1.0, label='B-tree', linestyle='dashed')
    pyplot.legend(loc='upper left')

  hashing_figure(experiment='serial-findonly')
  save_to('export/hashing-1.png')
  pyplot.clf()

  # Random INSERTs -- linear probing vs. cuckoo hashing vs. B-tree for reference
  # (Derived.)
  pyplot.figure(1, figsize=EXPORT_FIGSIZE)
  pyplot.xscale('log')
  pyplot.xlabel('Dictionary size')
  pyplot.ylabel('Time per operation [ns]')
  pyplot.grid(True)

  difference = get_difference('dict_htlp')
  pyplot.plot([size for size, _ in difference],
              [time / size for size, time in difference],
              'r', linewidth=2.0, label='Linear probing')
  difference = get_difference('dict_htcuckoo')
  pyplot.plot([size for size, _ in difference],
              [time / size for size, time in difference],
              'g', linewidth=2.0, label='Cuckoo hashing')
  difference = get_difference('dict_btree')
  difference = [(size, time) for size, time in difference if time / size < 1200]
  pyplot.plot([size for size, _ in difference],
              [time / size for size, time in difference],
              '#666666', linewidth=1.0, label='B-tree',
              linestyle='dashed')
  pyplot.legend(loc='upper left')
  save_to('export/hashing-2.png')
  pyplot.clf()

  hashing_figure(experiment='workingset', working_set_size=1000)
  save_to('export/hashing-3.png')
  pyplot.clf()

  hashing_figure(experiment='workingset', working_set_size=100000)
  save_to('export/hashing-4.png')
  pyplot.clf()

def plot_exported_figures():
  plot_cob_performance()
  plot_self_adjusting_performance()
  plot_hashing_performance()

def main():
  plot_exported_figures()

  plot_all_experiments()

  plot_mispredicts(data=load_data(experiment='serial-findonly'),
                   title='Mispredicts (random find)')
  save_to('random-mispredict.png')
  pyplot.clf()

  plot_pma_counters()
  plot_cuckoo_counters()
  plot_ksplay_ltr_counters()

  plot_cache_events(implementation='dict_cobt', experiment='serial-both')
  save_to('cobt-cache.png')
  pyplot.clf()

  plot_cache_events(implementation='dict_htable', experiment='serial-findonly')
  save_to('htable-cache-findonly.png')
  pyplot.clf()

  plot_cache_events(implementation='dict_htable', experiment='serial-both')
  save_to('htable-cache-both.png')
  pyplot.clf()

main()
