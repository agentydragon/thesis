#ifndef MEASUREMENT_H
#define MEASUREMENT_H

#include <jansson.h>
#include <stdint.h>

struct measurement;
typedef struct measurement measurement;

struct measurement_results;
typedef struct measurement_results measurement_results;

measurement* measurement_begin(void);
measurement_results* measurement_end(measurement*);
json_t* measurement_results_to_json(measurement_results*);
void measurement_results_release(measurement_results*);

#endif
