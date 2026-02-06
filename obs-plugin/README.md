# OBS SR Tracker Plugin

An OBS Studio plugin that captures Warzone Ranked Play SR (Skill Rating) from the game screen using OCR and displays it as an overlay. Optionally syncs SR updates to a webapp API.

## How It Works

1. The plugin captures frames from a selected video source (e.g., Game Capture)
2. Tesseract OCR reads the SR number from a configured screen region
3. The detected SR is displayed as a text overlay in the scene
4. SR changes are POSTed to a configured API endpoint

## Prerequisites

- **Windows 10/11** (x64)
- **Visual Studio 2022** with C++ desktop development workload
- **CMake** 3.16+
- **OBS Studio** 30.x (for SDK headers/libs)
- **vcpkg** (for Tesseract and libcurl)

## Setup

### 1. Install vcpkg dependencies

```bash
vcpkg install tesseract:x64-windows curl:x64-windows
```

### 2. Download Tesseract trained data

Download `eng.traineddata` from [tessdata](https://github.com/tesseract-ocr/tessdata) and place it in `obs-plugin/tessdata/`.

```bash
curl -L -o tessdata/eng.traineddata https://github.com/tesseract-ocr/tessdata/raw/main/eng.traineddata
```

### 3. Configure and build

```bash
cd obs-plugin
cmake --preset windows-x64 -DCMAKE_PREFIX_PATH="<path-to-obs-studio>"
cmake --build --preset windows-x64 --config Release
```

Set `CMAKE_PREFIX_PATH` to your OBS Studio install directory (e.g., `C:/Program Files/obs-studio`).

### 4. Install

Copy the built DLL to your OBS plugins directory:

```
build_x64/Release/obs-sr-tracker.dll → C:/Program Files/obs-studio/obs-plugins/64bit/
tessdata/eng.traineddata            → C:/Program Files/obs-studio/data/obs-plugins/obs-sr-tracker/tessdata/
data/locale/en-US.ini               → C:/Program Files/obs-studio/data/obs-plugins/obs-sr-tracker/locale/
```

## Usage

1. Open OBS Studio
2. Add a new **SR Tracker** source to your scene
3. In the source properties:
   - **Video Source**: Select your game capture source
   - **Region X/Y/Width/Height**: Set the pixel coordinates of the SR number on screen
   - **Capture Interval**: How often to OCR (default: 3 seconds)
   - **API Endpoint URL** / **API Key**: Optional — configure to sync SR to the webapp
   - **Manual SR Override**: Set a value manually (0 = use OCR)
   - **Display Format**: Customize the overlay text (use `{sr}` as placeholder)
4. Position and resize the SR Tracker source in your scene

## API Integration

When configured, the plugin POSTs SR changes to your API:

```
POST <api_url>
Authorization: Bearer <api_key>
Content-Type: application/json

{"sr": 2450, "timestamp": 1700000000}
```

## Troubleshooting

- **OCR not detecting**: Check that the region coordinates match where the SR number appears on screen. Use the "Test OCR" button.
- **Low accuracy**: Ensure the SR number is clearly visible and not obscured by other UI elements. The region should tightly crop just the number.
- **Plugin not loading**: Verify the DLL is in `obs-plugins/64bit/` and `eng.traineddata` is in the correct tessdata path.
