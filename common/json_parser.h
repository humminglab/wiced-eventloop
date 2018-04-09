/*
 * Copyright (c) 2018 HummingLab.io
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#pragma once

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define N_ELEMENT(tbl)	(sizeof(tbl)/sizeof((tbl)[0]))
#define EXT_PARM		4
#define NJSON(n)		(((n)+EXT_PARM)*2)

enum a_json_state {
	JSON_INIT,
	JSON_NAME,
	JSON_NAME_IN_STR,
	JSON_NAME_END,
	JSON_VALUE,
	JSON_VALUE_IN_STR,
	JSON_VALUE_END,
	JSON_VALUE_IN_VAL,
	JSON_BAD,
	JSON_GOOD,
};

typedef struct {
	char *buf;
	uint32_t buf_max;
	int *entry;
	uint32_t entry_max;
	char sep;
	int recur;

	int buf_pos;
	int entry_pos;

	enum a_json_state s;
	bool escaped;
} a_json_t;

/* simple JSON parser only support 1-depth object structure */

int32_t a_json_init(a_json_t *json, char *buf, size_t buf_len, int *entry, size_t entry_len, char sep);
bool a_json_append(a_json_t *json, char c);
bool a_json_append_str(a_json_t *json, char *str);
bool a_json_append_str_sized(a_json_t *json, char *str, size_t len);

static inline bool a_json_is_finished(a_json_t *json) {
	return (json->s >= JSON_BAD) ? true : false;
}

static inline bool a_json_is_good(a_json_t *json) {
	return (json->s == JSON_GOOD) ? true : false;
}

const char * a_json_get_prop(a_json_t *json, const char *prop);
const char * a_json_get_prop_safe(a_json_t *json, const char *prop, const char* def);
int a_json_comp_prop_val(a_json_t *json, const char * prop, const char *value);
int a_json_get_prop_int(a_json_t *json, const char *prop, int min, int max);
