/*
 * Copyright (c) 2018 HummingLab.io
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include "json_parser.h"
#include <string.h>
#include <stdlib.h>

/* simple JSON parser only support 1-depth object structure
 * without memory alloction */

int32_t a_json_init(a_json_t *json, char *buf, size_t buf_len,
		 int *entry, size_t entry_len, char sep)
{
	size_t i;
	if(json == 0 || buf == 0 || buf_len == 0 || entry_len == 0) { return -1; }
	memset(json, 0, sizeof(a_json_t));

	for (i = 0; i < entry_len; i++)
		entry[i] = -1;

	json->buf = buf;
	json->buf_max = buf_len;
	json->entry = entry;
	json->entry_max = entry_len;
	json->sep = sep;
	return 0;
}

bool a_json_append(a_json_t *json, char c)
{
	if (json->s == JSON_BAD || json->s == JSON_GOOD)
		return true;

	if (json->s == JSON_NAME_IN_STR || json->s == JSON_VALUE_IN_STR) {
		if (json->escaped) {
			json->escaped = false;
			switch (c) {
			case '\\': json->buf[json->buf_pos++] = '\\'; break;
			case '/':  json->buf[json->buf_pos++] = '/';  break;
			case '\r': json->buf[json->buf_pos++] = '\r'; break;
			case '\n': json->buf[json->buf_pos++] = '\n'; break;
			case '\f': json->buf[json->buf_pos++] = '\f'; break;
			case '\t': json->buf[json->buf_pos++] = '\t'; break;
			case '"':  json->buf[json->buf_pos++] = '"';  break;
			default:
				goto _bad;
			}
		} else if (c == '\\') {
			json->escaped = true;
		} else if (c == '"') {
			if (json->s == JSON_NAME_IN_STR &&
			    json->buf_pos == json->entry[json->entry_pos-1]) {
				/* zero length name */
				goto _bad;
			} else {
				json->buf[json->buf_pos++] = json->sep;
				json->s++; /* JSON_XXXX_END */
			}
		} else {
			json->buf[json->buf_pos++] = c;
		}
		goto _check_length;
	}

	if (isspace((int)c)) {
	_space:
		if (json->s == JSON_VALUE_IN_VAL) {
			json->buf[json->buf_pos++] = json->sep;
			json->s = JSON_VALUE_END;
		}
 	} else if (c == '{') {
		if (json->s == JSON_INIT) {
			json->s = JSON_NAME;
		} else if (json->s == JSON_VALUE) {
			json->entry[json->entry_pos++] = json->buf_pos;
			json->buf[json->buf_pos++] = json->sep;
			json->recur++;
			json->s = JSON_NAME;
		} else {
			goto _bad;
		}
	} else if (c == '}') {
		if (json->recur > 0) {
			json->recur--;
			goto _space;
		} else {
			if (json->s == JSON_VALUE_IN_VAL) {
				json->buf[json->buf_pos++] = json->sep;
				goto _good;
			} else if (json->s == JSON_VALUE_END) {
				goto _good;
			} else {
				goto _bad;
			}
		}
	} else if (c == ':') {
		if (json->s == JSON_NAME_END)
			json->s = JSON_VALUE;
		else
			goto _bad;
	} else if (c == ',') {
		if (json->s == JSON_VALUE_IN_VAL) {
			json->buf[json->buf_pos++] = json->sep;
			json->s = JSON_NAME;
		} else if (json->s == JSON_VALUE_END) {
			json->s = JSON_NAME;
		} else {
			goto _bad;
		}
	} else if (c == '"') {
		if (json->s == JSON_NAME || json->s == JSON_VALUE) {
			json->entry[json->entry_pos++] = json->buf_pos;
			json->s++;
		} else
			goto _bad;
	} else {
		if (json->s == JSON_VALUE_IN_VAL) {
			json->buf[json->buf_pos++] = c;
		} else if (json->s == JSON_VALUE) {
			json->entry[json->entry_pos++] = json->buf_pos;
			json->buf[json->buf_pos++] = c;
			json->s = JSON_VALUE_IN_VAL;
		}
	}

_check_length:
	if (json->buf_pos < (int)json->buf_max && json->entry_pos <= (int)json->entry_max)
		return false;

_bad:
	json->s = JSON_BAD;
	return true;

_good:
	json->buf[json->buf_pos - 1] = '\0';
	json->s = JSON_GOOD;
	return true;
}

bool a_json_append_str(a_json_t *json, char *str)
{
	while (*str != '\0') {
		if (a_json_append(json, *str++) == true)
			return true;
	}
	return false;
}

bool a_json_append_str_sized(a_json_t *json, char *str, size_t len)
{
	size_t i;
	for (i = 0; i < len; i++) {
		if (a_json_append(json, *str++) == true)
			return true;
	}
	return false;
}

static char * _get_prop(char *buf, int *entry, int max, const char *prop)
{
	int i;
	const char *p;
	for (i = 0; i < max || entry[i * 2] < 0; i++) {
		p = buf + entry[i * 2];
		if (strcmp(p, prop) == 0)
			return buf + entry[i * 2 + 1];
	}
	return NULL;
}

const char * a_json_get_prop(a_json_t *json, const char *prop)
{
	return _get_prop(json->buf, json->entry, json->entry_max/2, prop);
}

const char * a_json_get_prop_safe(a_json_t *json, const char *prop, const char* def)
{
	const char *r = a_json_get_prop(json, prop);
	return r ? r : def;
}

int a_json_comp_prop_val(a_json_t *json, const char * prop, const char *value)
{
	const char *v = a_json_get_prop(json, prop);
	if (!v)
		return -1;
	return strcmp(value, v);
}

int a_json_get_prop_int(a_json_t *json, const char *prop, int min, int max)
{
	int i;
	const char *val = a_json_get_prop(json, prop);
	if (!val)
		return min;

	i = atoi(val);
	if (i <= min) return min;
	if (i >= max) return max;
	return i;
}
