#include "sr-source.h"
#include "plugin-support.h"

#include <obs-module.h>
#include <graphics/graphics.h>
#include <util/platform.h>

#include <cstring>
#include <string>
#include <sstream>

/* ------------------------------------------------------------------ */
/* Forward declarations for obs_source_info callbacks                  */
/* ------------------------------------------------------------------ */

static const char *sr_get_name(void *unused);
static void *sr_create(obs_data_t *settings, obs_source_t *source);
static void sr_destroy(void *data);
static void sr_get_defaults(obs_data_t *settings);
static obs_properties_t *sr_get_properties(void *data);
static void sr_update(void *data, obs_data_t *settings);
static void sr_video_tick(void *data, float seconds);
static void sr_video_render(void *data, gs_effect_t *effect);
static uint32_t sr_get_width(void *data);
static uint32_t sr_get_height(void *data);

/* ------------------------------------------------------------------ */
/* Worker thread                                                       */
/* ------------------------------------------------------------------ */

static void sr_worker_thread(SrSourceData *sd)
{
	sr_log_info("Worker thread started");

	while (sd->running.load()) {
		// Wait for a new frame or shutdown
		{
			std::unique_lock<std::mutex> lock(sd->frame_mutex);
			sd->frame_cv.wait_for(lock,
					      std::chrono::milliseconds(500),
					      [sd] {
						      return sd->frame_ready ||
							     !sd->running.load();
					      });

			if (!sd->running.load())
				break;

			if (!sd->frame_ready)
				continue;

			sd->frame_ready = false;
		}

		// Run OCR on the captured pixels
		int sr = -1;
		{
			std::lock_guard<std::mutex> lock(sd->frame_mutex);
			if (!sd->pixel_buffer.empty() &&
			    sd->ocr.is_initialized()) {
				sr = sd->ocr.recognize(sd->pixel_buffer.data(),
						       sd->pixel_linesize,
						       sd->region);
			}
		}

		if (sr < 0)
			continue;

		int prev = sd->current_sr.load();
		if (sr == prev)
			continue;

		sd->current_sr.store(sr);
		sr_log_info("SR changed: %d -> %d", prev, sr);

		// Update overlay text
		if (sd->text_source) {
			std::string fmt = sd->display_format;
			std::string sr_str = std::to_string(sr);

			// Replace {sr} placeholder
			size_t pos = fmt.find("{sr}");
			if (pos != std::string::npos)
				fmt.replace(pos, 4, sr_str);
			else
				fmt = "SR: " + sr_str;

			obs_data_t *text_settings = obs_data_create();
			obs_data_set_string(text_settings, "text",
					    fmt.c_str());
			obs_source_update(sd->text_source, text_settings);
			obs_data_release(text_settings);
		}

		// POST to API
		if (sd->api.is_configured()) {
			sd->api.send_sr(sr);
		}
	}

	sr_log_info("Worker thread stopped");
}

/* ------------------------------------------------------------------ */
/* Helper: get tessdata path next to the plugin DLL                    */
/* ------------------------------------------------------------------ */

static std::string get_tessdata_path()
{
	char *module_path = obs_module_file("../tessdata");
	if (module_path) {
		std::string path(module_path);
		bfree(module_path);
		return path;
	}

	// Fallback: tessdata next to plugin binary
	char *plugin_dir = obs_module_file("");
	if (plugin_dir) {
		std::string path(plugin_dir);
		bfree(plugin_dir);
		path += "/../tessdata";
		return path;
	}

	return "tessdata";
}

/* ------------------------------------------------------------------ */
/* Source callbacks                                                     */
/* ------------------------------------------------------------------ */

static const char *sr_get_name(void *)
{
	return obs_module_text("SourceName");
}

static void *sr_create(obs_data_t *settings, obs_source_t *source)
{
	auto *sd = new SrSourceData();
	sd->self = source;
	sd->texrender = nullptr;
	sd->stagesurface = nullptr;
	sd->stage_width = 0;
	sd->stage_height = 0;
	sd->frame_ready = false;
	sd->pixel_linesize = 0;
	sd->pixel_width = 0;
	sd->pixel_height = 0;
	sd->current_sr.store(-1);
	sd->manual_sr = 0;
	sd->time_since_capture = 0.0f;
	sd->text_source = nullptr;
	sd->display_format = "SR: {sr}";

	// Initialize OCR engine
	std::string tessdata = get_tessdata_path();
	if (!sd->ocr.init(tessdata)) {
		sr_log_warn(
			"OCR init failed — OCR will be unavailable until tessdata is configured");
	}

	// Create graphics resources (must be on the graphics thread)
	obs_enter_graphics();
	sd->texrender = gs_texrender_create(GS_BGRA, GS_ZS_NONE);
	obs_leave_graphics();

	// Create internal text source for overlay rendering
	obs_data_t *text_settings = obs_data_create();
	obs_data_set_string(text_settings, "text", "SR: ---");
	obs_data_set_int(text_settings, "color", 0xFFFFFFFF);

#ifdef _WIN32
	sd->text_source =
		obs_source_create_private("text_gdiplus", "sr_text_internal",
					  text_settings);
#else
	sd->text_source =
		obs_source_create_private("text_ft2_source",
					  "sr_text_internal", text_settings);
#endif
	obs_data_release(text_settings);

	if (!sd->text_source)
		sr_log_warn("Failed to create text source for overlay");

	// Apply initial settings
	sr_update(sd, settings);

	// Start worker thread
	sd->running.store(true);
	sd->worker_thread = std::thread(sr_worker_thread, sd);

	sr_log_info("SR source created");
	return sd;
}

static void sr_destroy(void *data)
{
	auto *sd = static_cast<SrSourceData *>(data);

	// Stop worker thread
	sd->running.store(false);
	sd->frame_cv.notify_all();
	if (sd->worker_thread.joinable())
		sd->worker_thread.join();

	// Clean up text source
	if (sd->text_source) {
		obs_source_release(sd->text_source);
		sd->text_source = nullptr;
	}

	// Clean up graphics resources
	obs_enter_graphics();
	if (sd->stagesurface) {
		gs_stagesurface_destroy(sd->stagesurface);
		sd->stagesurface = nullptr;
	}
	if (sd->texrender) {
		gs_texrender_destroy(sd->texrender);
		sd->texrender = nullptr;
	}
	obs_leave_graphics();

	// Shutdown OCR
	sd->ocr.shutdown();

	sr_log_info("SR source destroyed");
	delete sd;
}

static void sr_get_defaults(obs_data_t *settings)
{
	obs_data_set_default_string(settings, S_SOURCE_NAME, "");
	obs_data_set_default_int(settings, S_REGION_X, 0);
	obs_data_set_default_int(settings, S_REGION_Y, 0);
	obs_data_set_default_int(settings, S_REGION_W, 200);
	obs_data_set_default_int(settings, S_REGION_H, 60);
	obs_data_set_default_double(settings, S_CAPTURE_INTERVAL, 3.0);
	obs_data_set_default_string(settings, S_API_URL, "");
	obs_data_set_default_string(settings, S_API_KEY, "");
	obs_data_set_default_int(settings, S_MANUAL_SR, 0);
	obs_data_set_default_string(settings, S_DISPLAY_FORMAT, "SR: {sr}");
}

/* Callback to populate source dropdown with available video sources */
static bool enum_video_sources(void *data, obs_source_t *source)
{
	auto *list = static_cast<obs_property_t *>(data);
	uint32_t flags = obs_source_get_output_flags(source);

	if (flags & OBS_SOURCE_VIDEO) {
		const char *name = obs_source_get_name(source);
		if (name && *name)
			obs_property_list_add_string(list, name, name);
	}
	return true;
}

/* Callback for "Test OCR" button */
static bool test_ocr_clicked(obs_properties_t *, obs_property_t *, void *data)
{
	auto *sd = static_cast<SrSourceData *>(data);

	int sr = sd->current_sr.load();
	if (sr >= 0) {
		sr_log_info("Test OCR: current SR = %d", sr);
	} else {
		sr_log_info(
			"Test OCR: no SR detected yet — check region settings and source");
	}

	// Trigger an immediate capture by resetting the timer
	sd->time_since_capture = sd->capture_interval + 1.0f;
	return true;
}

static obs_properties_t *sr_get_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();

	// Source selection
	obs_property_t *src_list = obs_properties_add_list(
		props, S_SOURCE_NAME, obs_module_text("Setting.SourceName"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(src_list, "(none)", "");
	obs_enum_sources(enum_video_sources, src_list);

	// OCR region
	obs_properties_add_int(props, S_REGION_X,
			       obs_module_text("Setting.RegionX"), 0, 7680, 1);
	obs_properties_add_int(props, S_REGION_Y,
			       obs_module_text("Setting.RegionY"), 0, 4320, 1);
	obs_properties_add_int(props, S_REGION_W,
			       obs_module_text("Setting.RegionWidth"), 1, 1920,
			       1);
	obs_properties_add_int(props, S_REGION_H,
			       obs_module_text("Setting.RegionHeight"), 1, 1080,
			       1);

	// Capture interval
	obs_properties_add_float(
		props, S_CAPTURE_INTERVAL,
		obs_module_text("Setting.CaptureInterval"), 0.5, 60.0, 0.5);

	// API settings
	obs_properties_add_text(props, S_API_URL,
				obs_module_text("Setting.ApiUrl"),
				OBS_TEXT_DEFAULT);
	obs_properties_add_text(props, S_API_KEY,
				obs_module_text("Setting.ApiKey"),
				OBS_TEXT_PASSWORD);

	// Manual SR override
	obs_properties_add_int(props, S_MANUAL_SR,
			       obs_module_text("Setting.ManualSR"), 0, 99999,
			       1);

	// Display format
	obs_properties_add_text(props, S_DISPLAY_FORMAT,
				obs_module_text("Setting.DisplayFormat"),
				OBS_TEXT_DEFAULT);

	// Test OCR button
	obs_properties_add_button2(props, S_TEST_OCR,
				   obs_module_text("Setting.TestOCR"),
				   test_ocr_clicked, data);

	return props;
}

static void sr_update(void *data, obs_data_t *settings)
{
	auto *sd = static_cast<SrSourceData *>(data);

	sd->target_source_name =
		obs_data_get_string(settings, S_SOURCE_NAME);

	sd->region.x = (int)obs_data_get_int(settings, S_REGION_X);
	sd->region.y = (int)obs_data_get_int(settings, S_REGION_Y);
	sd->region.width = (int)obs_data_get_int(settings, S_REGION_W);
	sd->region.height = (int)obs_data_get_int(settings, S_REGION_H);

	sd->capture_interval =
		(float)obs_data_get_double(settings, S_CAPTURE_INTERVAL);

	sd->display_format =
		obs_data_get_string(settings, S_DISPLAY_FORMAT);
	if (sd->display_format.empty())
		sd->display_format = "SR: {sr}";

	// API configuration
	std::string url = obs_data_get_string(settings, S_API_URL);
	std::string key = obs_data_get_string(settings, S_API_KEY);
	sd->api.configure(url, key);

	// Manual SR override
	int manual = (int)obs_data_get_int(settings, S_MANUAL_SR);
	sd->manual_sr = manual;

	if (manual > 0) {
		sd->current_sr.store(manual);

		// Update overlay text immediately
		if (sd->text_source) {
			std::string fmt = sd->display_format;
			std::string sr_str = std::to_string(manual);
			size_t pos = fmt.find("{sr}");
			if (pos != std::string::npos)
				fmt.replace(pos, 4, sr_str);
			else
				fmt = "SR: " + sr_str;

			obs_data_t *text_settings = obs_data_create();
			obs_data_set_string(text_settings, "text",
					    fmt.c_str());
			obs_source_update(sd->text_source, text_settings);
			obs_data_release(text_settings);
		}

		// Also POST to API if configured
		if (sd->api.is_configured()) {
			// Do this in a detached thread to avoid blocking the UI
			int sr_val = manual;
			std::thread([sd, sr_val] {
				sd->api.send_sr(sr_val);
			}).detach();
		}
	}
}

/* ------------------------------------------------------------------ */
/* Frame capture in video_tick                                         */
/* ------------------------------------------------------------------ */

static void sr_video_tick(void *data, float seconds)
{
	auto *sd = static_cast<SrSourceData *>(data);

	// If manual override is active, skip OCR capture
	if (sd->manual_sr > 0)
		return;

	// Throttle capture by interval
	sd->time_since_capture += seconds;
	if (sd->time_since_capture < sd->capture_interval)
		return;
	sd->time_since_capture = 0.0f;

	// Need a target source
	if (sd->target_source_name.empty())
		return;

	obs_source_t *target =
		obs_get_source_by_name(sd->target_source_name.c_str());
	if (!target)
		return;

	uint32_t target_w = obs_source_get_width(target);
	uint32_t target_h = obs_source_get_height(target);

	if (target_w == 0 || target_h == 0) {
		obs_source_release(target);
		return;
	}

	// Validate region is within bounds
	if (sd->region.x + sd->region.width > (int)target_w ||
	    sd->region.y + sd->region.height > (int)target_h) {
		obs_source_release(target);
		return;
	}

	// Render target source into texture
	if (!sd->texrender) {
		obs_source_release(target);
		return;
	}

	gs_texrender_reset(sd->texrender);

	if (!gs_texrender_begin(sd->texrender, target_w, target_h)) {
		obs_source_release(target);
		return;
	}

	// Set up orthographic projection
	gs_ortho(0.0f, (float)target_w, 0.0f, (float)target_h, -100.0f,
		 100.0f);

	obs_source_video_render(target);
	gs_texrender_end(sd->texrender);

	obs_source_release(target);

	// Get the rendered texture
	gs_texture_t *tex = gs_texrender_get_texture(sd->texrender);
	if (!tex)
		return;

	// Create or recreate stagesurface if size changed
	if (!sd->stagesurface || sd->stage_width != target_w ||
	    sd->stage_height != target_h) {
		if (sd->stagesurface)
			gs_stagesurface_destroy(sd->stagesurface);

		sd->stagesurface = gs_stagesurface_create(target_w, target_h,
							  GS_BGRA);
		sd->stage_width = target_w;
		sd->stage_height = target_h;
	}

	if (!sd->stagesurface)
		return;

	// Stage the texture (GPU → CPU)
	gs_stage_texture(sd->stagesurface, tex);

	uint8_t *stage_data = nullptr;
	uint32_t linesize = 0;

	if (!gs_stagesurface_map(sd->stagesurface, &stage_data, &linesize))
		return;

	// Copy the pixel data into the shared buffer
	{
		std::lock_guard<std::mutex> lock(sd->frame_mutex);
		size_t total_size = (size_t)linesize * target_h;
		sd->pixel_buffer.resize(total_size);
		std::memcpy(sd->pixel_buffer.data(), stage_data, total_size);
		sd->pixel_linesize = (int)linesize;
		sd->pixel_width = (int)target_w;
		sd->pixel_height = (int)target_h;
		sd->frame_ready = true;
	}

	gs_stagesurface_unmap(sd->stagesurface);

	// Wake the worker thread
	sd->frame_cv.notify_one();
}

/* ------------------------------------------------------------------ */
/* Overlay rendering                                                   */
/* ------------------------------------------------------------------ */

static void sr_video_render(void *data, gs_effect_t *)
{
	auto *sd = static_cast<SrSourceData *>(data);

	if (sd->text_source)
		obs_source_video_render(sd->text_source);
}

static uint32_t sr_get_width(void *data)
{
	auto *sd = static_cast<SrSourceData *>(data);

	if (sd->text_source)
		return obs_source_get_width(sd->text_source);

	return 0;
}

static uint32_t sr_get_height(void *data)
{
	auto *sd = static_cast<SrSourceData *>(data);

	if (sd->text_source)
		return obs_source_get_height(sd->text_source);

	return 0;
}

/* ------------------------------------------------------------------ */
/* Register source                                                     */
/* ------------------------------------------------------------------ */

void sr_source_register()
{
	struct obs_source_info info = {};

	info.id = "sr_tracker";
	info.type = OBS_SOURCE_TYPE_INPUT;
	info.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW;
	info.get_name = sr_get_name;
	info.create = sr_create;
	info.destroy = sr_destroy;
	info.get_defaults = sr_get_defaults;
	info.get_properties = sr_get_properties;
	info.update = sr_update;
	info.video_tick = sr_video_tick;
	info.video_render = sr_video_render;
	info.get_width = sr_get_width;
	info.get_height = sr_get_height;

	obs_register_source(&info);
}
