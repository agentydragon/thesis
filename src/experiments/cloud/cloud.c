#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "dict/splay.h"
#include "experiments/cloud/flags.h"
#include "log/log.h"
#include "measurement/stopwatch.h"
#include "rand/rand.h"
#include "util/count_of.h"

typedef struct {
	char time[10];
	char sky_brightness_indicator[1];
	char lat_x100[5];
	char lon_x100[5];
	char station_id[5];
	char land_ocean_indicator[1];
	char present_weather[2];
	char total_cloud_cover[1];
	char low_amount[2];
	char low_base_height[2];
	char low_type[2];
	char mid_type[2];
	char high_type[2];
	char mid_amount_x100[3];
	char high_amount_x100[3];
	char mid_amount_nonover[1];
	char high_amount_nonover[1];
	char change_code[2];
	char solar_altitude_degx10[4];
	char relative_lunar_illuminance_x100[4];
	char sea_level_pressure[5];
	char wind_speed[3];
	char wind_direction_deg[3];
	char air_temp_dc_x10[4];
	char dew_point_depression[3];
	char elevation_sea_surface_temp[4];
	char wind_speed_indicator[1];
	char ip_ih[1];
} __attribute__((packed)) record;

typedef struct {
	int lat_x100;  // -9000..9000
	int lon_x100;  // 0..36000
	uint64_t id;   // (lat,lon,id) is an unique identifier

	int sea_level_pressure; // 8000..11000
	int wind_speed;         // 0..1000
} parsed_record;

static void parse_line(char* line, record* r) {
	memcpy(r, line, strlen(line));
}

static uint32_t expand_16(uint32_t x) {
	// 0000 0000 0000 0000 1234 5678 abcd efgh
	x = ((x & 0x0000FF00) << 8) | (x & 0x000000FF);
	// 0000 0000 1234 5678 0000 0000 abcd efgh
	x = ((x & 0x00F000F0) << 4) | (x & 0x000F000F);
	// 0000 1234 0000 5678 0000 abcd 0000 efgh
	x = ((x & 0x0C0C0C0C) << 2) | (x & 0x03030303);
	// 0012 0034 0056 0078 00ab 00cd 00ef 00gh
	x = ((x & 0x22222222) << 1) | (x & 0x11111111);
	// 0102 0304 0506 0708 0a0b 0c0d 0e0f 0g0h
	return x;
}

static uint64_t expand_32(uint64_t x) {
	return (((uint64_t) expand_16((x & 0xFFFF0000ULL) >> 16ULL) << 32ULL) |
			expand_16(x & 0x0000FFFFULL));
}

static uint64_t mix_32w32(uint32_t a, uint32_t b) {
	return (expand_32(a) << 1) | expand_32(b);
}

// lat_x100: latitude in degrees * 100, minimum -9000, maximum 9000
// lon_x100: longitude in degrees * 100, minimum 0, maximum 36000
// Returns 34-bit position code.
static uint64_t build_position_code(int lat_x100, int lon_x100) {
	assert(lat_x100 >= -9000 && lat_x100 <= 9000);
	assert(lon_x100 >= 0 && lon_x100 <= 36000);

	const uint32_t lat_code = (lat_x100 + 9000);
	ASSERT(lat_code < (1ULL << 20ULL));

	const uint32_t lon_code = lon_x100;
	ASSERT(lon_code < (1ULL << 20ULL));

	const uint64_t code = mix_32w32(lat_code, lon_code);
	assert(code < (1ULL << 35ULL));
	return code;
}

static void test_bit_ops() {
	ASSERT(expand_32(0x00000000) == 0x0000000000000000ULL);
	ASSERT(expand_32(0x0000FFFF) == 0x0000000055555555ULL);
	ASSERT(expand_32(0xFF00FFFF) == 0x5555000055555555ULL);
	ASSERT(expand_16(0x0000) == 0x00000000);
	ASSERT(expand_16(0xFFFF) == 0x55555555);
	ASSERT(expand_16(0xFF00) == 0x55550000);
	ASSERT(expand_16(0xFF11) == 0x55550101);
}

static parsed_record* records = NULL;
static uint64_t records_capacity = 0;
static uint64_t records_size = 0;
static dict* id_map;

static FILE* averages_file;
static FILE* positions_file;

static void load_records(const char* path) {
	FILE* input = fopen(path, "r");
	CHECK(input != NULL, "cannot open %s", path);
	while (!feof(input)) {
		char line[1024];
		fgets(line, sizeof(line) - 1, input);

		record r;
		parse_line(line, &r);

		parsed_record pr;
		char buffer[100];

		memset(buffer, 0, sizeof(buffer));
		memcpy(buffer, r.lat_x100, sizeof(r.lat_x100));
		pr.lat_x100 = atoi(buffer);

		memset(buffer, 0, sizeof(buffer));
		memcpy(buffer, r.lon_x100, sizeof(r.lon_x100));
		pr.lon_x100 = atoi(buffer);

		if (pr.sea_level_pressure <= 20000 && pr.wind_speed <= 1000 && pr.wind_speed >= 0 && pr.sea_level_pressure >= 8000) {
			if (FLAGS.dump_averages) {
				fprintf(positions_file, "%d\t%d\n",
						pr.lat_x100, pr.lon_x100);
			}
		}

		memset(buffer, 0, sizeof(buffer));
		memcpy(buffer, r.wind_speed, sizeof(r.wind_speed));
		pr.wind_speed = atoi(buffer);
		assert(pr.wind_speed >= 0 && pr.wind_speed < 1000);

		memset(buffer, 0, sizeof(buffer));
		memcpy(buffer, r.sea_level_pressure, sizeof(r.sea_level_pressure));
		pr.sea_level_pressure = atoi(buffer);
		assert(pr.sea_level_pressure >= 8000 && pr.sea_level_pressure < 11000);

		const uint64_t position_code = build_position_code(pr.lat_x100,
				pr.lon_x100);
		uint64_t value;
		bool found;
		ASSERT(dict_find(id_map, position_code, &value, &found) == 0);
		if (found) {
			ASSERT(dict_delete(id_map, position_code) == 0);
			++value;
		} else {
			value = 0;
		}
		ASSERT(dict_insert(id_map, position_code, value) == 0);

		pr.id = value;
		ASSERT(pr.id < (1ULL << 20ULL));

		// log_info("time=%.10s lat_x100=%.5s lon_x100=%.5s wind_dir_deg=%.3s",
		// 		r.time, r.lat_x100, r.lon_x100,
		// 		r.wind_direction_deg);
		// log_info("lat=%d lon=%d", pr.lat_x100, pr.lon_x100);

		if (records_capacity == records_size) {
			if (records_capacity == 0) {
				records_capacity = 64;
			} else {
				records_capacity *= 2;
			}
			records = realloc(records, sizeof(parsed_record) *
					records_capacity);
			ASSERT(records);
		}
		records[records_size++] = pr;
	}
	fclose(input);
}

static uint64_t build_record_code(parsed_record r) {
	const uint64_t position_code = build_position_code(
			r.lat_x100, r.lon_x100);
	ASSERT(position_code < (1ULL << 35ULL));
	ASSERT(r.id < (1ULL << 20ULL));

	// 35 bits: lat_code, lon_code
	// 21 bits: record identifier
	return (position_code << 21ULL) | r.id;
}

static uint64_t build_record_value(parsed_record r) {
	return (((uint64_t) r.sea_level_pressure) << 32ULL) | (r.wind_speed);
}

static bool is_sane_pressure(int pressure) {
	return pressure >= 8000 && pressure <= 20000;
}

static bool is_sane_speed(int speed) {
	return speed >= 0 && speed <= 1000;
}

static void collect_wind_speed(const uint64_t value,
		uint64_t* total_wind_speed, uint64_t* wind_speed_count) {
	const uint64_t wind_speed = (value & ((1ULL << 32ULL) - 1));
	if (is_sane_speed(wind_speed)) {
		(*total_wind_speed) += wind_speed;
		(*wind_speed_count)++;
		//log_info("%" PRIu64 " => wind speed %" PRIu64 ", value,
		//		wind_speed);
	}
}

static void collect_sea_level_pressure(const uint64_t value,
		uint64_t* total_sea_level_pressure,
		uint64_t* sea_pressure_count) {
	const uint64_t sea_level_pressure = (value >> 32ULL);

	if (is_sane_pressure(sea_level_pressure)) {
		(*total_sea_level_pressure) += sea_level_pressure;
		(*sea_pressure_count)++;
		//log_info("%" PRIu64 " => pressure %" PRIu64 ", value,
		//		sea_level_pressure);
	}
}

static void query_close_points(dict* map, const int lat, const int lon) {
	const uint64_t position_code = build_position_code(lat, lon);
	const uint64_t key = position_code << 21ULL;

	uint64_t wind_speed_count = 0, sea_level_pressure_count = 0;

	const uint64_t CLOSE = FLAGS.close_point_count;

	uint64_t total_wind_speed = 0;
	uint64_t total_sea_level_pressure = 0;

	uint64_t next;
	uint64_t prev;
	bool got_prev, got_next;

	ASSERT(dict_next(map, key, &next, &got_next) == 0);
	ASSERT(dict_prev(map, key, &prev, &got_prev) == 0);

	uint64_t even_odd = 0;
	while ((wind_speed_count < CLOSE || sea_level_pressure_count < CLOSE) &&
			(got_prev || got_next)) {
		uint64_t value;
		if (got_next && (!got_prev || even_odd % 2 == 0)) {
			bool found;
			ASSERT(dict_find(map, next, &value, &found) == 0);
			ASSERT(found);

			ASSERT(dict_next(map, next, &next, &got_next) == 0);
		} else {
			ASSERT(got_prev);
			bool found;
			ASSERT(dict_find(map, prev, &value, &found) == 0);
			ASSERT(found);

			ASSERT(dict_prev(map, prev, &prev, &got_prev) == 0);
		}
		if (wind_speed_count < CLOSE) {
			collect_wind_speed(value,
					&total_wind_speed, &wind_speed_count);
		}
		if (sea_level_pressure_count < CLOSE) {
			collect_sea_level_pressure(value,
					&total_sea_level_pressure,
					&sea_level_pressure_count);
		}
		++even_odd;
	}

	if (FLAGS.dump_averages) {
		const double pressure = (double) total_sea_level_pressure / sea_level_pressure_count;
		const double speed = (double) total_wind_speed / wind_speed_count;
		// log_info("%d\t%d\t%lf\t%lf", lat, lon, pressure, speed);
		if (is_sane_pressure(pressure) && is_sane_speed(speed)) {
			fprintf(averages_file, "%d\t%d\t%lf\t%lf\n",
				lat, lon, pressure, speed);
		}
	}
	//log_info("total sea-level pressure: %" PRIu64,
	//		total_sea_level_pressure);
	//log_info("total wind speed: %" PRIu64, total_wind_speed);
	//log_info("at [%d;%d]: (sea-level pressure: %lf, wind speed: %lf)",
}

static void test_api(const dict_api* api) {
	dict* map;
	ASSERT(dict_init(&map, api, NULL) == 0);

	// Insert all records.
	for (uint64_t i = 0; i < records_size; ++i) {
		const uint64_t key = build_record_code(records[i]);
		const uint64_t value = build_record_value(records[i]);
		ASSERT(dict_insert(map, key, value) == 0);
	}

	stopwatch watch = stopwatch_start();

	// Find closest measurements for all grid points.
	switch (FLAGS.scatter) {
	case SCATTER_GRID:
		for (int lat = -9000; lat < 9000; lat += FLAGS.lat_step) {
			for (int lon = 0; lon < 36000; lon += FLAGS.lon_step) {
				query_close_points(map, lat, lon);
			}
		}
		break;
	case SCATTER_RANDOM: {
		rand_generator rand = { .state = 0 };
		for (uint64_t i = 0; i < FLAGS.sample_count; ++i) {
			const int lat = -9000 + rand_next(&rand, 9000 * 2);
			const int lon = rand_next(&rand, 36000);
			query_close_points(map, lat, lon);
		}
		break;
	}
	default:
		log_fatal("invalid scatter mode");
	}

	log_info("%15s: %15" PRIu64 " ns", api->name, stopwatch_read_ns(watch));
	dict_destroy(&map);
}

int main(int argc, char** argv) {
	parse_flags(argc, argv);

	if (FLAGS.dump_averages) {
		averages_file = fopen("experiments/cloud/averages.csv", "w");
		CHECK(averages_file, "cannot open averages.csv");

		positions_file = fopen("experiments/cloud/positions.csv", "w");
		CHECK(positions_file, "cannot open positions.csv");
	}

	test_bit_ops();

	const char* MONTHS[] = {
		"JAN", "FEB", "MAR", "APR", "MAY", "JUN",
		"JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
	};
	log_info("loading records");
	ASSERT(dict_init(&id_map, &dict_splay, NULL) == 0);
	for (int year = FLAGS.min_year; year <= FLAGS.max_year; ++year) {
		for (uint64_t month = 0; month < COUNT_OF(MONTHS); ++month) {
			char filename[100];
			sprintf(filename, "experiments/cloud/data/%s%02dL",
					MONTHS[month], year % 100);
			load_records(filename);
		}
	}
	dict_destroy(&id_map);
	log_info("loaded %" PRIu64 " records", records_size);

	for (uint64_t i = 0; FLAGS.measured_apis[i]; ++i) {
		test_api(FLAGS.measured_apis[i]);
	}

	if (FLAGS.dump_averages) {
		fclose(averages_file);
		fclose(positions_file);
	}

	return 0;
}
