#include <obs-module.h>
#include "plugin-support.h"
#include "sr-source.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

bool obs_module_load(void)
{
	sr_source_register();
	sr_log_info("plugin loaded (version %s)", PLUGIN_VERSION);
	return true;
}

void obs_module_unload(void)
{
	sr_log_info("plugin unloaded");
}

const char *obs_module_name(void)
{
	return "SR Tracker";
}

const char *obs_module_description(void)
{
	return "Captures Warzone Ranked Play SR from screen via OCR";
}
