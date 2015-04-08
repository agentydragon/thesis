#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "dict/btree.h"
#include "dict/cobt.h"
#include "dict/dict.h"
#include "dict/kforest.h"
#include "dict/ksplay.h"
#include "dict/splay.h"
#include "log/log.h"
#include "measurement/stopwatch.h"

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
	char air_temperature_dc_x10[4];
	char dew_point_depression[3];
	char elevation_sea_surface_temp[4];
	char wind_speed_indicator[1];
	char ip_ih[1];
} __attribute__((packed)) record;

typedef struct {
	uint64_t id;   // monotonically increasing
	int lat_x100;  // -9000..9000
	int lon_x100;  // 0..36000
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
	return (((uint64_t) expand_16((x & 0xFFFF0000ULL) >> 16ULL) << 32ULL) | expand_16(x & 0x0000FFFFULL));
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

static void load_records() {
	FILE* input = fopen("experiments/cloud/SEP97L", "r");
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

		pr.id = records_size;

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
			records = realloc(records, sizeof(parsed_record) * records_capacity);
			ASSERT(records);
		}
		records[records_size++] = pr;
	}
	fclose(input);
	log_info("loaded %" PRIu64 " records", records_size);
}

struct {
	const dict_api* measured_apis[20];
} FLAGS;

static void set_defaults() {
	FLAGS.measured_apis[0] = &dict_btree;
	FLAGS.measured_apis[1] = &dict_cobt;
	FLAGS.measured_apis[2] = &dict_splay;
	FLAGS.measured_apis[3] = &dict_kforest;
	FLAGS.measured_apis[4] = &dict_ksplay;
	FLAGS.measured_apis[5] = NULL;
}

void parse_flags(int argc, char** argv) {
	set_defaults();

	// TODO
	(void) argc; (void) argv;
}

static void test_api(const dict_api* api) {
	dict* map;
	ASSERT(dict_init(&map, api, NULL) == 0);

	stopwatch watch = stopwatch_start();
	for (uint64_t i = 0; i < records_size; ++i) {
		const uint64_t position_code = build_position_code(
				records[i].lat_x100, records[i].lon_x100);
		assert(position_code < (1ULL << 35ULL));

		// 35 bits: lat_code, lon_code
		// 24 bits: record identifier
		const uint64_t key = (position_code << 24ULL) |
				(records[i].id & ((1 << 24ULL) - 1));
		ASSERT(dict_insert(map, key, 4242) == 0);
	}

	log_info("%20s: %20" PRIu64 " ns", api->name, stopwatch_read_ns(watch));
	dict_destroy(&map);
}

int main(int argc, char** argv) {
	parse_flags(argc, argv);

	test_bit_ops();

	load_records();

	for (uint64_t i = 0; FLAGS.measured_apis[i]; ++i) {
		test_api(FLAGS.measured_apis[i]);
	}

	return 0;
}
