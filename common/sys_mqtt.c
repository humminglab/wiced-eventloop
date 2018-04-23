/*
 * Copyright (c) 2018 HummingLab.io
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include "wiced.h"
#include "wiced_log.h"
#include "platform_button.h"

#include "eventloop.h"
#include "sys_mqtt.h"
#include "network.h"

#define MQTT_REQUEST_TIMEOUT	(5000U)
#define MQTT_RECONNECT_TIMEOUT	(5000U)
#define MQTT_RECONNECT_RANDOM_WINDOW (5000U)

#define MQTT_MAX_TOPIC_SIZE	128
#define MQTT_MAX_PAYLOAD_SIZE	512

static wiced_result_t mqtt_inner_event_loop(sys_mqtt_t *s, uint32_t event_type)
{
	wiced_result_t res;
	s->mqtt_pending_event_bitmap = 0;
	s->mqtt_interruptable_event_bitmap = (1 << WICED_MQTT_EVENT_TYPE_DISCONNECTED) |
		(uint32_t)(1 << event_type);
	wiced_log_msg(WLF_DEF, WICED_LOG_DEBUG0, "Waiting mqtt response (%d ms)\n", MQTT_REQUEST_TIMEOUT);
	res = a_eventloop(s->evt, MQTT_REQUEST_TIMEOUT);
	wiced_log_msg(WLF_DEF, WICED_LOG_DEBUG0, "Event Loop Result: return=%d, bitmap=0x%0x\n",
		      res, s->mqtt_pending_event_bitmap);
	s->mqtt_interruptable_event_bitmap = 0;
	if (s->mqtt_pending_event_bitmap & (1 << event_type)) {
		return WICED_SUCCESS;
	} else {
		wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Fail to response\n");
		return WICED_ERROR;
	}
}

static wiced_result_t mqtt_connection_event_cb(wiced_mqtt_object_t mqtt_object,
					       wiced_mqtt_event_info_t *event)
{
	const char *name;
	sys_mqtt_t *s = (sys_mqtt_t*)mqtt_object;

	switch (event->type) {
        case WICED_MQTT_EVENT_TYPE_CONNECT_REQ_STATUS:
		name = "CONNECT_REQ_STATUS"; break;
        case WICED_MQTT_EVENT_TYPE_DISCONNECTED:
		name = "DISCONNECTED"; break;
        case WICED_MQTT_EVENT_TYPE_PUBLISHED:
		name = "PUBLISHED"; break;
        case WICED_MQTT_EVENT_TYPE_SUBCRIBED:
		name = "SUBSCRIBED"; break;
        case WICED_MQTT_EVENT_TYPE_UNSUBSCRIBED:
		name = "UNSUBSCRIBED"; break;
        case WICED_MQTT_EVENT_TYPE_PUBLISH_MSG_RECEIVED:
		name = "PUBLISH_MSG_RECEIVED"; break;
	default:
	case WICED_MQTT_EVENT_TYPE_UNKNOWN:
		name = "UNKNOWN"; break;
	}
	wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "MQTT Event: %s\n", name);

	switch (event->type) {
        case WICED_MQTT_EVENT_TYPE_CONNECT_REQ_STATUS:
		a_eventloop_set_flag(s->evt, A_EVENT_MQTT_CONNECT_REQ);
		break;
        case WICED_MQTT_EVENT_TYPE_PUBLISHED:
		a_eventloop_set_flag(s->evt, A_EVENT_MQTT_PUBLISHED);
		break;
        case WICED_MQTT_EVENT_TYPE_SUBCRIBED:
		a_eventloop_set_flag(s->evt, A_EVENT_MQTT_SUBSCRIBED);
		break;
        case WICED_MQTT_EVENT_TYPE_UNSUBSCRIBED:
		break;
        case WICED_MQTT_EVENT_TYPE_DISCONNECTED:
		a_eventloop_set_flag(s->evt, A_EVENT_MQTT_DISCONNECTED);
		break;
        case WICED_MQTT_EVENT_TYPE_PUBLISH_MSG_RECEIVED:
		if (s->subscribe_cb)
			(*s->subscribe_cb)(s, &event->data.pub_recvd, s->arg);
		break;
	case WICED_MQTT_EVENT_TYPE_UNKNOWN:
        default:
		break;
	}
	return WICED_SUCCESS;
}

static wiced_result_t mqtt_app_open(sys_mqtt_t *s)
{
	wiced_ip_address_t broker_address;
	wiced_result_t res;
	wiced_mqtt_security_t security;
	wiced_mqtt_pkt_connect_t conninfo;

	wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Connecting MQTT\n");
	res = wiced_hostname_lookup(s->hostname, &broker_address, 10000, WICED_STA_INTERFACE);
	if (res != WICED_SUCCESS) {
		wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Fail to get broker ip address of %s\n",
			      s->hostname);
		return res;
	}
	wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "MQTT %s -> %d.%d.%d.%d\n",
		      s->hostname,
		      (broker_address.ip.v4 >> 24) & 0xff,
		      (broker_address.ip.v4 >> 16) & 0xff,
		      (broker_address.ip.v4 >>  8) & 0xff,
		      (broker_address.ip.v4 >>  0) & 0xff);

	/* NOTE: Wiced MQTT library is supposed to use TLS alone, so patch it.
	 *       (skip wiced_tls_init_root_ca_certificates())
	 *       If you want TlS, pass security pointer initialized to 0.
	 *       If not, pass NULL pointer
	 */
	memset(&security, 0, sizeof(security));
	memset(&conninfo, 0, sizeof(conninfo));
	conninfo.port_number = 0;
	conninfo.mqtt_version = WICED_MQTT_PROTOCOL_VER4;
	conninfo.clean_session = 1;
	conninfo.client_id = 0;
	conninfo.keep_alive = 60;
	conninfo.password = 0;
	conninfo.username = (uint8_t*)s->user_token;
	conninfo.peer_cn = (uint8_t*)s->peer_cn;
	res = wiced_mqtt_connect(s->mqtt_obj, &broker_address, WICED_STA_INTERFACE,
				 mqtt_connection_event_cb,
				 (s->use_tls) ? &security : NULL, &conninfo);

	if (res != WICED_SUCCESS) {
		wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Fail to connect MQTT (%d)\n", res);
		return WICED_ERROR;
	}
	return mqtt_inner_event_loop(s, WICED_MQTT_EVENT_TYPE_CONNECT_REQ_STATUS);
}

static wiced_result_t mqtt_app_close(sys_mqtt_t *s)
{
	wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Disconnecting MQTT\n");
	if (wiced_mqtt_disconnect(s->mqtt_obj) != WICED_SUCCESS) {
		wiced_log_msg(WLF_DEF, WICED_LOG_ERR, " Fail to disconnect MQTT\n");
		return WICED_ERROR;
	}

	mqtt_inner_event_loop(s, WICED_MQTT_EVENT_TYPE_DISCONNECTED);
	return WICED_SUCCESS;
}

wiced_result_t a_sys_mqtt_app_subscribe(sys_mqtt_t *s, char *topic, int qos)
{
	wiced_mqtt_msgid_t pktid;

	wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Subscribing MQTT: %s\n", topic);
	pktid = wiced_mqtt_subscribe(s->mqtt_obj, topic, (uint8_t)qos);
	if (pktid == 0) {
		wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Fail to get msgid\n");
		return WICED_ERROR;
	}
	return mqtt_inner_event_loop(s, WICED_MQTT_EVENT_TYPE_SUBCRIBED);
}

wiced_result_t a_sys_mqtt_publish(sys_mqtt_t *s, char *topic, char* data, uint32_t data_len, int qos, wiced_bool_t retain)
{
	wiced_mqtt_msgid_t pktid;
	wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Publishing MQTT(QOS%d): %s\n", qos, topic);
	wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "    %s\n", data);
	pktid = wiced_mqtt_publish(s->mqtt_obj, (uint8_t*)topic, (uint8_t*)data,
				   data_len, (uint8_t)qos, retain);
	if (pktid == 0) {
		wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Fail to get msgid\n");
		return WICED_ERROR;
	}
	return mqtt_inner_event_loop(s, WICED_MQTT_EVENT_TYPE_PUBLISHED);
}

static void event_net_change(void *arg);
static void mqtt_retry_cb(void *arg)
{
	sys_mqtt_t *s = arg;
	a_eventloop_deregister_timer(s->evt, &s->retry_timer_node);
	event_net_change(s);
}

static void event_net_change(void *arg)
{
	sys_mqtt_t *s = arg;

	if (a_network_is_up()) {
		if (s->mqtt_connected) {
			s->mqtt_connected = WICED_FALSE;
			mqtt_app_close(s);
		}

		if (mqtt_app_open(s) == WICED_SUCCESS) {
			s->mqtt_connected = WICED_TRUE;
		} else {
			uint32_t tout = MQTT_RECONNECT_TIMEOUT +
				((uint32_t)rand() % MQTT_RECONNECT_RANDOM_WINDOW);
			wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Retry after %d ms\n", tout);
			a_eventloop_register_timer(s->evt, &s->retry_timer_node,
						   mqtt_retry_cb, tout, s);
		}
	} else {
		a_eventloop_deregister_timer(s->evt, &s->retry_timer_node);
		if (s->mqtt_connected) {
			s->mqtt_connected = WICED_FALSE;
			mqtt_app_close(s);
		}
	}

	if (s->net_event_cb)
		(*s->net_event_cb)(a_network_is_up(), s->mqtt_connected, s->arg);
}

static wiced_result_t event_mqtt(sys_mqtt_t *s, wiced_mqtt_event_type_t type)
{
	if (s->mqtt_interruptable_event_bitmap & (uint32_t)(1 << type)) {
		s->mqtt_pending_event_bitmap = (1 << type);
		a_eventloop_break(s->evt);
		return WICED_SUCCESS;
	}
	return WICED_ERROR;
}

static void event_mqtt_connect_req(void *arg)
{
	event_mqtt(arg, WICED_MQTT_EVENT_TYPE_CONNECT_REQ_STATUS);
}

static void event_mqtt_published(void *arg)
{
	event_mqtt(arg, WICED_MQTT_EVENT_TYPE_PUBLISHED);
}

static void event_mqtt_subscribed(void *arg)
{
	event_mqtt(arg, WICED_MQTT_EVENT_TYPE_SUBCRIBED);
}

static void event_mqtt_disconnected(void *arg) {
	sys_mqtt_t *s = arg;
	if (event_mqtt(s, WICED_MQTT_EVENT_TYPE_DISCONNECTED) != WICED_SUCCESS) {
		a_eventloop_deregister_timer(s->evt, &s->retry_timer_node);
		event_net_change(s);
	}
}

static void _net_event(wiced_bool_t up, void *arg)
{
	sys_mqtt_t *s = arg;
	wiced_log_msg(WLF_DEF, WICED_LOG_INFO, "Network Event: %s\n", up ? "UP" : "DOWN");
	a_eventloop_set_flag(s->evt, A_EVENT_NET_CHANGE);
}

wiced_result_t a_sys_mqtt_init(sys_mqtt_t* s, eventloop_t *e, const char* hostname, wiced_bool_t use_tls,
			       const char *user_token, const char *peer_cn, mqtt_subscribe_cb subscribe_cb,
			       mqtt_net_event_cb net_event_cb, void *arg)
{
	memset(s, 0, sizeof(*s));
	s->evt = e;
	s->hostname = hostname;
	s->use_tls = use_tls;
	s->user_token = user_token;
	s->peer_cn = peer_cn;
	s->subscribe_cb = subscribe_cb;
	s->net_event_cb = net_event_cb;
	s->arg = arg;
	wiced_rtos_init_mutex(&s->mutex);

	a_network_register_callback(_net_event, s);
	wiced_mqtt_init(s->mqtt_obj);

	a_eventloop_register_event(s->evt, &s->net_event_node, event_net_change, A_EVENT_NET_CHANGE, s);
	a_eventloop_register_event(s->evt, &s->mqtt_con_event_node, event_mqtt_connect_req, A_EVENT_MQTT_CONNECT_REQ, s);
	a_eventloop_register_event(s->evt, &s->mqtt_pub_event_node, event_mqtt_published, A_EVENT_MQTT_PUBLISHED, s);
	a_eventloop_register_event(s->evt, &s->mqtt_sub_event_node, event_mqtt_subscribed, A_EVENT_MQTT_SUBSCRIBED, s);
	a_eventloop_register_event(s->evt, &s->mqtt_discon_event_node, event_mqtt_disconnected, A_EVENT_MQTT_DISCONNECTED, s);

	if (a_network_is_up())
		a_eventloop_set_flag(s->evt, A_EVENT_NET_CHANGE);
	return WICED_SUCCESS;
}
