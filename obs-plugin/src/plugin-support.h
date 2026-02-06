#pragma once

#include <obs-module.h>

#define PLUGIN_NAME "obs-sr-tracker"
#define PLUGIN_VERSION "1.0.0"

#define blog(level, msg, ...) \
	blog(level, "[" PLUGIN_NAME "] " msg, ##__VA_ARGS__)

#define sr_log_info(msg, ...) blog(LOG_INFO, msg, ##__VA_ARGS__)
#define sr_log_warn(msg, ...) blog(LOG_WARNING, msg, ##__VA_ARGS__)
#define sr_log_error(msg, ...) blog(LOG_ERROR, msg, ##__VA_ARGS__)
#define sr_log_debug(msg, ...) blog(LOG_DEBUG, msg, ##__VA_ARGS__)
