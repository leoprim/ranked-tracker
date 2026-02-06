#pragma once

#include <obs-module.h>
#include <mutex>
#include <atomic>
#include <thread>
#include <condition_variable>
#include <string>
#include <vector>

#include "ocr-engine.h"
#include "api-client.h"

// Settings keys
#define S_SOURCE_NAME "source_name"
#define S_REGION_X "region_x"
#define S_REGION_Y "region_y"
#define S_REGION_W "region_w"
#define S_REGION_H "region_h"
#define S_CAPTURE_INTERVAL "capture_interval"
#define S_API_URL "api_url"
#define S_API_KEY "api_key"
#define S_MANUAL_SR "manual_sr"
#define S_TEST_OCR "test_ocr"
#define S_DISPLAY_FORMAT "display_format"
#define S_FONT "font"
#define S_FONT_SIZE "font_size"
#define S_FONT_COLOR "font_color"

struct SrSourceData {
	obs_source_t *self;

	// Target source to capture from
	std::string target_source_name;

	// OCR region
	OcrRegion region;

	// Timing
	float capture_interval;
	float time_since_capture;

	// Frame capture
	gs_texrender_t *texrender;
	gs_stagesurface_t *stagesurface;
	uint32_t stage_width;
	uint32_t stage_height;

	// Pixel buffer shared between tick and worker
	std::vector<uint8_t> pixel_buffer;
	int pixel_linesize;
	int pixel_width;
	int pixel_height;
	bool frame_ready;
	std::mutex frame_mutex;
	std::condition_variable frame_cv;

	// OCR engine
	OcrEngine ocr;

	// API client
	ApiClient api;

	// Current SR value
	std::atomic<int> current_sr;
	int manual_sr;

	// Text overlay (internal text_gdiplus source)
	obs_source_t *text_source;
	std::string display_format;

	// Worker thread
	std::thread worker_thread;
	std::atomic<bool> running;
};

// Register the source with OBS
void sr_source_register();
