#include <Arduino.h>

#include "Audio.h"
#include "SD_MMC.h"

static constexpr std::array<const char *, 3> testFiles = {"/JSBach_testfile_no_cover.mp3",
	"/JSBach_testfile_cover_756k.mp3",
	"/JSBach_testfile_cover_800k.mp3"};

#define I2S_DOUT 25 // Digital out (I2S)
#define I2S_BCLK 27 // BCLK (I2S)
#define I2S_LRC	 26 // LRC (I2S)

static Audio audio;
constexpr uint32_t deepsleepTimeAfterBootFails = 20;

void setup() {
	Serial.begin(115200);
	log_n("Testing audioI2S");

	log_n("Mounting SD Card");
	pinMode(2, INPUT_PULLUP);
	while (!SD_MMC.begin("/sdcard", true)) {
		log_w("Unable to mount sd-card.");
		delay(500);
		if (millis() >= deepsleepTimeAfterBootFails * 1000) {
			log_e("Failed to boot due to SD. Will go to deepsleep...");
			esp_deep_sleep_start();
		}
	}

	log_n("Starting I2s audio");
	audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
	audio.setVolume(5, 0);
	audio.forceMono(true);
	audio.setTone(3, 0, 0);

	log_n("Will start playing all 3 files after each other");
	log_n("Look for crashes");

	for (const auto &path : testFiles) {
		if (!SD_MMC.exists(path)) {
			log_e("Could not open file: %s", path);
			continue;
		}
		if (!audio.connecttoFS(SD_MMC, path)) {
			log_e("Failed to init I2S");
			continue;
		}
		log_n("Starting playback of: %s", path);
		while (audio.isRunning()) {
			yield();
			constexpr size_t trasherSize = 512;
			void *trasher = ps_malloc(trasherSize);
			memset(trasher, 0, trasherSize);
			audio.loop();
			free(trasher);
		}
	}
}

void loop() {
	log_n("No fail, retrying...");
	ESP.restart();
}
