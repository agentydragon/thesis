#!/usr/bin/python

import glob
import json
from matplotlib import pyplot

def select_api(data, api):
  return [point for point in data if point['implementation'] == api]

def select_metric_per_element(data, metric):
  return [point['metrics'][metric] / point['size'] for point in data]
def per_element(data, item):
  return [point[item] / point['size'] for point in data]

def mentioned_apis(data):
  return sorted(set(point['implementation'] for point in data))

def apis_with_colors(data):
  return zip(mentioned_apis(data),
             ['r-', 'g-', 'b-', 'c-', 'm-', 'y-', 'k-', 'rx', 'gx', 'bx'])

def new_figure(size=(20, 20), ylabel=None):
  pyplot.clf()
  figure = pyplot.figure(1)
  figure.set_size_inches(*size)
  pyplot.xscale('log')
  pyplot.grid(True)
  if ylabel:
    pyplot.ylabel(ylabel)
  return figure

def sizes(api_data):
  return [point['size'] for point in api_data]
def times(api_data):
  return [point['time_ns'] / point['size'] for point in api_data]

def plot_data(data, *rest, **kwargs):
  if data:
    pyplot.plot(sizes(data), times(data), *rest, **kwargs)

def plot_graph(data, title=None):
  new_figure()
  subplot = pyplot.subplot(211)
  subplot.set_title(title)
  subplot.set_xscale('log')
  for api, color in apis_with_colors(data):
    api_data = select_api(data, api)
    plot_data(api_data, color, linewidth=2.0, label=api)
  pyplot.legend(loc='upper left')
  pyplot.ylabel('Time per key [ns]')
  pyplot.grid(True)

  subplot = pyplot.subplot(212)
  subplot.set_xscale('log')
  for api, color in apis_with_colors(data):
    api_data = select_api(data, api)
    pyplot.plot(sizes(api_data),
                select_metric_per_element(api_data, 'cache_misses'),
                color, linewidth=1.0, label=api)
  pyplot.legend(loc='upper left')
  pyplot.ylabel('Cache misses per key')
  pyplot.xlabel('Inserted items')

def plot_mispredicts(data):
  def mispredicts(api_data):
    return [point['metrics']['branch_mispredicts'] /
            point['metrics']['branches'] for point in api_data
                                         if point['metrics']['branches']]
  new_figure(ylabel='Branch mispredict ratio')
  for api, color in apis_with_colors(data):
    api_data = select_api(data, api)
    api_data = [point for point in api_data if point['metrics']['branches']]
    pyplot.plot(sizes(api_data), mispredicts(api_data), color, linewidth=2.0,
                label=api)
  pyplot.legend(loc='upper left')
  # pyplot.xlabel('Inserted items')

CACHED_DATA = None

def save_to(filename):
  pyplot.savefig('output/' + filename, bbox_inches='tight')

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

def load_data(discard_trivial=1000, **filters):
  global CACHED_DATA

  if not CACHED_DATA:
    CACHED_DATA = []
    for path in glob.glob('output/results*.json'):
      print('Appending results from %s...' % path)
      with open(path) as results_file:
        CACHED_DATA.extend(json.load(results_file))

  def point_is_good(point):
    if discard_trivial and point['size'] < discard_trivial:
      return False
    return point_fits_filters(point, filters)

  return list(filter(point_is_good, CACHED_DATA))

def plot_all_experiments():
  for experiment, title in [
      ('word_frequency', 'Word frequency'),
      ('serial-insertonly', 'Random insert'),
      ('serial-findonly', 'Random find'),
      ('ltr_scan', 'Left-to-right scan')]:
    plot_graph(data=load_data(experiment=experiment), title=title)
    save_to(experiment + '.png')

  for success_rate in [100, 50, 0]:
    new_figure(EXPORT_FIGSIZE_FULLWIDTH)
    plot_graph(data=load_data(experiment='serial-findonly',
                              success_percentage=success_rate),
               title=('Serial, find-only, %d%%' % success_rate))
    save_to('serial-findonly-%d.png' % success_rate)

  new_figure(EXPORT_FIGSIZE)
  plot_graph(data=load_data(working_set_size=1000, experiment='workingset'),
             title='Random finds in 1k working set')
  save_to('ws-1k-find.png')

  plot_graph(data=load_data(working_set_size=100000, experiment='workingset'),
             title='Random finds in 100k working set')
  save_to('ws-100k-find.png')

def plot_ksplay_ltr_counters():
  data = load_data(implementation='dict_ksplay', experiment='ltr_scan')

  new_figure(ylabel='K-splay steps per element')
  pyplot.plot(sizes(data), per_element(data, 'ksplay_steps'), 'r-')
  save_to('ltr-ksplays.png')

  new_figure(ylabel='K-splay composed keys per element')
  pyplot.plot(sizes(data), per_element(data, 'ksplay_composed_keys'), 'r-')
  save_to('ltr-composes.png')

def plot_cuckoo_counters():
  data = load_data(implementation='dict_htcuckoo', experiment='serial-insertonly')
  new_figure(ylabel='Cost of full rehashes per insert')
  pyplot.plot(sizes(data), [point['cuckoo_full_rehashes'] for point in data],
              'r-')
  save_to('cuckoo-full-rehashes.png')

  new_figure(ylabel='Traversed edges per insert')
  pyplot.plot(sizes(data),
              [point['cuckoo_traversed_edges'] / point['size'] for point in data],
              'r-')
  save_to('cuckoo-traversed-edges.png')

def plot_pma_counters():
  data = load_data(implementation='dict_cobt', experiment='serial-insertonly')
  new_figure(ylabel='Reorganizations per element')
  pyplot.plot(sizes(data),
              [point['pma_reorganized'] / point['size'] for point in data],
              'r-')
  save_to('pma-reorg.png')

def plot_cache_events(**kwargs):
  data = load_data(**kwargs)
  new_figure(ylabel='Cache misses per element')
  for metric, color, label in [
      ('l1d_read_misses', 'r-', 'L1D read misses'),
      ('l1d_write_misses', 'r-', 'L1D write misses'),
      ('l1i_read_misses', 'b-', 'L1I read misses'),
      ('dtlb_read_misses', 'g-', 'dTLB read misses'),
      ('dtlb_write_misses', 'g-', 'dTLB write misses')]:
    pyplot.plot(sizes(data), select_metric_per_element(data, metric),
                color, label=label)
  pyplot.legend(loc='upper left')

HASH_LABEL = 'Hash table with linear probing'
EXPORT_FIGSIZE = (6,6)
EXPORT_FIGSIZE_FULLWIDTH = (18,6)

def plot_cob_find_speed_figures():
  def find_speed_fig_internal(data):
    # Random FINDs: cache-oblivious B-tree vs. B-tree vs. linear probing HT
    pyplot.xlabel('Dictionary size')
    pyplot.ylabel('Time per operation [ns]')

    plot_data(select_api(data, 'dict_btree'),
              'r', linewidth=2.0, label='B-tree')
    plot_data(select_api(data, 'dict_cobt'),
              'g', linewidth=2.0, label='Cache-oblivious B-tree')
    plot_data(select_api(data, 'dict_htlp'),
              '#666666', linewidth=1.0, label=HASH_LABEL, linestyle='dashed')

    pyplot.legend(loc='upper left')
  def find_speed_fig(**kwargs):
    find_speed_fig_internal(load_data(**kwargs))

  for success_rate in [100, 50, 0]:
    new_figure(EXPORT_FIGSIZE)
    find_speed_fig(experiment='serial-findonly',
                   success_percentage=success_rate)
    save_to('export/cob-performance-1-%d.png' % success_rate)

  new_figure(EXPORT_FIGSIZE)
  find_speed_fig(experiment='workingset', working_set_size=1000)
  save_to('export/cob-performance-3.png')

  new_figure(EXPORT_FIGSIZE)
  find_speed_fig(experiment='workingset', working_set_size=100000)
  save_to('export/cob-performance-4.png')

  new_figure(EXPORT_FIGSIZE, ylabel='Time per scanned key [ns]')
  find_speed_fig(experiment='ltr_scan')
  save_to('export/cob-performance-5.png')

  new_figure(EXPORT_FIGSIZE, ylabel='Time per indexed word')
  data = load_data(experiment='word_frequency')
  data = [point for point in data if point['size'] <= 800 * 1000]
  find_speed_fig_internal(data)
  pyplot.xlabel('Indexed words')
  save_to('export/cob-performance-6.png')

def plot_cob_performance():
  plot_cob_find_speed_figures()

  # Random INSERTs: cache-oblivious B-tree vs. B-tree vs. linear probing HT
  # (Derived.)
  new_figure(EXPORT_FIGSIZE, ylabel='Time per operation [ns]')
  pyplot.xlabel('Dictionary size')

  for api, color, style_args in [
      ('dict_btree', 'r', {'linewidth': 2.0, 'label': 'B-tree'}),
      ('dict_cobt', 'g', {'linewidth': 2.0, 'label': 'Cache-oblivious B-tree'}),
      ('dict_htlp', '#666666', {'linewidth': 1.0, 'label': HASH_LABEL,
                                'linestyle': 'dashed'})]:
    plot_data(load_data(implementation=api, experiment='serial-insertonly'),
              color, **style_args)

  pyplot.legend(loc='upper left')
  save_to('export/cob-performance-2.png')

  data = load_data(experiment='serial-findonly', success_percentage=100)
  new_figure(ylabel='Time per operation [ns]')
  for api, color, label in [
      ('dict_btree', 'r-', 'B-tree'),
      ('dict_cobt', 'g-', 'Cache-oblivious B-tree'),
      ('dict_htcuckoo', 'b-', 'Cuckoo hash table'),
      ('dict_htlp', 'c-', 'Hash table with linear probing'),
      ('dict_kforest', 'k-', 'K-forest'),
      ('dict_ksplay', 'y-', 'K-splay tree'),
      ('dict_splay', 'm-', 'Splay tree')]:
    api_data = select_api(data, api)
    plot_data(api_data, color, linewidth=2.0, label=label)
  pyplot.legend(loc='upper left')
  save_to('export/overall-random-find.png')

def plot_basic_performance():
  trivial = 1000

  def plot_basic(data):
    for api, color, label in [
        ('dict_rbtree', 'r-', 'Red-black tree'),
        ('dict_btree', 'g-', 'B-tree'),
        ('dict_array', 'b-', 'Array with binary search')]:
      api_data = select_api(data, api)
      api_data = [point for point in api_data
                  if point['time_ns'] / point['size'] < 1400 or
                     point['implementation'] != 'dict_array']
      plot_data(api_data, color, linewidth=2.0, label=label)

    pyplot.legend(loc='upper left')
    pyplot.ylabel('Time per operation [ns]')

  for success_rate in [100, 50, 0]:
    new_figure(EXPORT_FIGSIZE_FULLWIDTH)
    plot_basic(load_data(experiment='serial-findonly',
                         success_percentage=success_rate,
                         discard_trivial=trivial))
    save_to('export/basic-random-find-%d.png' % success_rate)

  new_figure(EXPORT_FIGSIZE)
  plot_basic(load_data(experiment='serial-insertonly', discard_trivial=trivial))
  save_to('export/basic-random-insert.png')

  new_figure(EXPORT_FIGSIZE)
  plot_basic(load_data(experiment='workingset', working_set_size=1000,
             discard_trivial=trivial))
  save_to('export/basic-ws-1k.png')

  working_set_size = 100000
  data = load_data(experiment='workingset', working_set_size=working_set_size,
                   discard_trivial=trivial)
  new_figure(EXPORT_FIGSIZE)
  plot_basic([point for point in data if point['size'] >= working_set_size])
  save_to('export/basic-ws-100k.png')

def plot_self_adjusting_performance():
  def plot_self_adj(data):
    for api, color, style_args in [
        ('dict_splay', 'r-', {'label': 'Splay tree', 'linewidth': 2.0}),
        ('dict_ksplay', 'g-', {'label': 'K-splay tree', 'linewidth': 2.0}),
        ('dict_kforest', 'b-', {'label': 'K-forest', 'linewidth': 2.0}),
        ('dict_rbtree', '#333333', {'label': 'Red-black tree', 'linewidth': 1.0, 'linestyle': 'dashed'}),
        ('dict_btree', '#666666', {'label': 'B-tree', 'linewidth': 1.0, 'linestyle': 'dotted'})]:
      api_data = select_api(data, api)
      plot_data(api_data, color, **style_args)

    pyplot.legend(loc='upper left')
    pyplot.ylabel('Time per operation [ns]')

  for success_rate in [100, 50, 0]:
    new_figure(EXPORT_FIGSIZE_FULLWIDTH)
    plot_self_adj(load_data(experiment='serial-findonly',
                            success_percentage=success_rate))
    save_to('export/self-adj-random-find-%d.png' % success_rate)

  new_figure(EXPORT_FIGSIZE)
  plot_self_adj(load_data(experiment='serial-insertonly'))
  save_to('export/self-adj-random-insert.png')

  new_figure(EXPORT_FIGSIZE)
  plot_self_adj(load_data(experiment='workingset', working_set_size=1000))
  save_to('export/self-adj-ws-1k.png')

  working_set_size = 100000
  data = load_data(experiment='workingset', working_set_size=working_set_size)
  new_figure(EXPORT_FIGSIZE)
  plot_self_adj([point for point in data if point['size'] >= working_set_size])
  save_to('export/self-adj-ws-100k.png')

def plot_hashing_performance():
  API_COLORS_STYLES = [
      ('dict_htlp', 'r', {'linewidth': 2.0, 'label': 'Linear probing'}),
      ('dict_htcuckoo', 'g', {'linewidth': 2.0, 'label': 'Cuckoo hashing'}),
      ('dict_btree', '#666666', {'linewidth': 1.0, 'label': 'B-tree',
                                 'linestyle': 'dashed'})]

  def hashing_figure(data):
    pyplot.xlabel('Dictionary size')
    pyplot.ylabel('Time per operation [ns]')

    for api, color, style_args in API_COLORS_STYLES:
      api_data = select_api(data, api)
      api_data = [point for point in api_data if point['time_ns'] / point['size'] < 500]
      plot_data(api_data, color, **style_args)

    pyplot.legend(loc='upper left')

  for success_rate in [100, 50, 0]:
    new_figure(EXPORT_FIGSIZE_FULLWIDTH)
    hashing_figure(load_data(experiment='serial-findonly',
                             success_percentage=success_rate))
    save_to('export/hashing-1-%d.png' % success_rate)

  # Random INSERTs -- linear probing vs. cuckoo hashing vs. B-tree for reference
  # (Derived.)
  new_figure(EXPORT_FIGSIZE, ylabel='Time per operation [ns]')
  pyplot.xlabel('Dictionary size')

  new_figure(EXPORT_FIGSIZE)
  for api, color, style_args in API_COLORS_STYLES:
    difference = load_data(implementation=api, experiment='serial-insertonly')
    difference = [point for point in difference if point['time_ns'] / point['size'] < 1200]
    plot_data(difference, color, **style_args)
  pyplot.legend(loc='upper left')
  save_to('export/hashing-2.png')

  new_figure(EXPORT_FIGSIZE)
  hashing_figure(load_data(experiment='workingset', working_set_size=1000))
  save_to('export/hashing-3.png')

  new_figure(EXPORT_FIGSIZE)
  working_set_size = 100000
  data = load_data(experiment='workingset', working_set_size=working_set_size)
  hashing_figure([point for point in data if point['size'] >= working_set_size])
  save_to('export/hashing-4.png')

def plot_exported_figures():
  plot_basic_performance()
  plot_cob_performance()
  plot_self_adjusting_performance()
  plot_hashing_performance()

def main():
  plot_exported_figures()
  plot_all_experiments()
  plot_mispredicts(load_data(experiment='serial-findonly',
                             success_percentage=100))
  save_to('random-mispredict.png')
  plot_pma_counters()
  plot_cuckoo_counters()
  plot_ksplay_ltr_counters()
  for dict in ['cobt', 'array', 'btree', 'rbtree']:
    for operation in ['find', 'insert']:
      plot_cache_events(implementation=('dict_%s' % dict),
                        experiment=('serial-%sonly' % operation))
      save_to('cache-%s-%s.png' % (operation, dict))

main()
