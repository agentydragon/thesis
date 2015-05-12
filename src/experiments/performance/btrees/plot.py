#!/usr/bin/python

import json
from matplotlib import pyplot

FIGSIZE = (6,6)

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

def load_data(file, discard_trivial=1000, **filters):
  with open(file) as results_file:
    results = json.load(results_file)

  def point_is_good(point):
    if discard_trivial and point['size'] < discard_trivial:
      return False
    return point_fits_filters(point, filters)

  return list(filter(point_is_good, results))

def sizes(api_data):
  return [point['size'] for point in api_data]
def times(api_data):
  return [point['time_ns'] / point['size'] for point in api_data]

def plot_data(data, *rest, **kwargs):
  pyplot.plot(sizes(data), times(data), *rest, **kwargs)

def main():
  for percentage in [0, 50, 100]:
    pyplot.figure(1, figsize=FIGSIZE)
    pyplot.xscale('log')
    pyplot.xlabel('Dictionary size')
    pyplot.ylabel('Time per operation [ns]')
    pyplot.grid(True)
    for node_size, color in [(64, 'r-'), (128, 'g-'), (256, 'b-'), (512, 'c-'), (1024, 'm-')]:
      plot_data(load_data('results-btree-%dB.json' % node_size,
                          experiment='serial-findonly',
                          success_percentage=percentage),
                color, label=('%d B nodes' % node_size))
    pyplot.legend(loc='upper left')
    pyplot.savefig('random-find-%d.png' % percentage, bbox_inches='tight')
    pyplot.clf()

  pyplot.figure(1, figsize=FIGSIZE)
  pyplot.xscale('log')
  pyplot.xlabel('Dictionary size')
  pyplot.ylabel('Time per operation [ns]')
  pyplot.grid(True)
  for node_size, color in [(64, 'r-'), (128, 'g-'), (256, 'b-'), (512, 'c-'), (1024, 'm-')]:
    plot_data(load_data('results-btree-%dB.json' % node_size,
                        experiment='serial-insertonly'),
              color, label=('%d B nodes' % node_size))
  pyplot.legend(loc='upper left')
  pyplot.savefig('random-insert.png', bbox_inches='tight')
  pyplot.clf()


main()
