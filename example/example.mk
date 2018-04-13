NAME := App_example

COMMON := ..//common

$(NAME)_SOURCES    := main.c \
			$(COMMON)/network.c \
			$(COMMON)/util.c $(COMMON)/eventloop.c \
			$(COMMON)/console.c \
			$(COMMON)/sys_led.c \
			$(COMMON)/sys_pwm.c \
			$(COMMON)/sys_button.c \
			$(COMMON)/sys_mqtt.c \
			$(COMMON)/sys_worker.c \
			$(COMMON)/json_parser.c \
			$(COMMON)/device.c

GLOBAL_INCLUDES += $(COMMON)
$(NAME)_INCLUDES   += . \
		      $(COMMON)

$(NAME)_COMPONENTS += protocols/HTTP \
		      protocols/MQTT \
		      utilities/wiced_log \
		      utilities/command_console \
	              utilities/command_console/wifi \
		      utilities/command_console/platform \
	              utilities/command_console/dct

GLOBAL_DEFINES     += MAC_ADDRESS_SET_BY_HOST \
		      WICED_DCT_INCLUDE_BT_CONFIG

GLOBAL_DEFINES	   += TARGET_LED_GW

WIFI_CONFIG_DCT_H := $(COMMON)/wifi_config_dct.h

APPLICATION_DCT := $(COMMON)/app_dct.c
