#include "ocr-engine.h"
#include "plugin-support.h"

#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>

#include <vector>
#include <string>
#include <algorithm>
#include <cstring>

OcrEngine::OcrEngine() : tess_api(nullptr), initialized(false) {}

OcrEngine::~OcrEngine()
{
	shutdown();
}

bool OcrEngine::init(const std::string &tessdata_path)
{
	if (initialized)
		shutdown();

	auto *api = new tesseract::TessBaseAPI();

	int result = api->Init(tessdata_path.c_str(), "eng",
			       tesseract::OEM_LSTM_ONLY);
	if (result != 0) {
		sr_log_error("Tesseract init failed (path: %s)",
			     tessdata_path.c_str());
		delete api;
		return false;
	}

	// Restrict to digits and comma (for thousands separator like 2,450)
	api->SetVariable("tessedit_char_whitelist", "0123456789,");
	// Single line mode — SR is always a single number
	api->SetPageSegMode(tesseract::PSM_SINGLE_LINE);

	tess_api = api;
	initialized = true;

	sr_log_info("Tesseract OCR initialized (tessdata: %s)",
		    tessdata_path.c_str());
	return true;
}

void OcrEngine::shutdown()
{
	if (tess_api) {
		auto *api = static_cast<tesseract::TessBaseAPI *>(tess_api);
		api->End();
		delete api;
		tess_api = nullptr;
	}
	initialized = false;
}

int OcrEngine::recognize(const uint8_t *bgra_data, int linesize,
			 const OcrRegion &region)
{
	if (!initialized || !bgra_data)
		return -1;

	if (region.width <= 0 || region.height <= 0)
		return -1;

	// Convert BGRA region to grayscale
	std::vector<uint8_t> gray(region.width * region.height);

	for (int row = 0; row < region.height; row++) {
		const uint8_t *src =
			bgra_data + (region.y + row) * linesize + region.x * 4;
		uint8_t *dst = gray.data() + row * region.width;

		for (int col = 0; col < region.width; col++) {
			uint8_t b = src[col * 4 + 0];
			uint8_t g = src[col * 4 + 1];
			uint8_t r = src[col * 4 + 2];
			// Standard luminance weights
			dst[col] = static_cast<uint8_t>(0.299f * r + 0.587f * g +
							0.114f * b);
		}
	}

	auto *api = static_cast<tesseract::TessBaseAPI *>(tess_api);

	api->SetImage(gray.data(), region.width, region.height, 1,
		      region.width);

	char *text = api->GetUTF8Text();
	int confidence = api->MeanTextConf();

	if (!text) {
		sr_log_warn("OCR returned null text");
		return -1;
	}

	if (confidence < 50) {
		sr_log_debug("OCR low confidence (%d): '%s'", confidence, text);
		delete[] text;
		api->Clear();
		return -1;
	}

	// Parse the result: strip commas and whitespace, convert to integer
	std::string raw(text);
	delete[] text;
	api->Clear();

	// Remove commas, spaces, newlines
	std::string cleaned;
	for (char c : raw) {
		if (c >= '0' && c <= '9')
			cleaned += c;
	}

	if (cleaned.empty()) {
		sr_log_debug("OCR produced no digits (raw: '%s')", raw.c_str());
		return -1;
	}

	int sr_value = 0;
	try {
		sr_value = std::stoi(cleaned);
	} catch (...) {
		sr_log_warn("OCR parse failed: '%s'", cleaned.c_str());
		return -1;
	}

	// Sanity check: SR values are typically 0-10000
	if (sr_value < 0 || sr_value > 99999) {
		sr_log_warn("OCR value out of range: %d", sr_value);
		return -1;
	}

	sr_log_debug("OCR result: %d (confidence: %d)", sr_value, confidence);
	return sr_value;
}
