#pragma once

#include <string>
#include <cstdint>

struct OcrRegion {
	int x;
	int y;
	int width;
	int height;
};

class OcrEngine {
public:
	OcrEngine();
	~OcrEngine();

	OcrEngine(const OcrEngine &) = delete;
	OcrEngine &operator=(const OcrEngine &) = delete;

	bool init(const std::string &tessdata_path);
	void shutdown();
	bool is_initialized() const { return initialized; }

	/**
	 * Recognize SR value from a BGRA pixel buffer.
	 * @param bgra_data  Pointer to the full frame BGRA pixels
	 * @param linesize   Bytes per row in the full frame
	 * @param region     Sub-region to OCR within the frame
	 * @return Parsed SR integer, or -1 on failure/low confidence
	 */
	int recognize(const uint8_t *bgra_data, int linesize,
		      const OcrRegion &region);

private:
	void *tess_api; // tesseract::TessBaseAPI* (opaque to avoid header leak)
	bool initialized;
};
