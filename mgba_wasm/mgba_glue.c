/* mGBA WASM Glue - Exposes mGBA core to JavaScript via Emscripten */
#include <mgba/core/core.h>
#include <mgba/gba/interface.h>
#include <mgba-util/vfs.h>
#include <mgba-util/image.h>
#include <mgba-util/audio-buffer.h>

#include <emscripten/emscripten.h>
#include <stdlib.h>
#include <string.h>

#define WIDTH  GBA_VIDEO_HORIZONTAL_PIXELS
#define HEIGHT GBA_VIDEO_VERTICAL_PIXELS

static struct mCore *core = NULL;
static mColor *videoBuffer = NULL;
static uint8_t *romData = NULL;
static size_t romSize = 0;

/* 32-bit RGBA output for canvas (always 240x160x4 bytes) */
static uint8_t rgbaBuffer[WIDTH * HEIGHT * 4];

/* Audio buffer - enough for one frame (~800 samples at 32768 Hz / 60fps) */
#define AUDIO_SAMPLES 2048
static int16_t audioBuffer[AUDIO_SAMPLES * 2]; /* stereo */
static int audioSamplesReady = 0;

EMSCRIPTEN_KEEPALIVE
int mgba_create(void) {
    core = mCoreCreate(mPLATFORM_GBA);
    if (!core) return -1;
    if (!core->init(core)) {
        core = NULL;
        return -2;
    }
    /* Initialize config (required before reset) */
    mCoreInitConfig(core, NULL);
    /* Allocate video buffer */
    videoBuffer = (mColor *)malloc(WIDTH * HEIGHT * sizeof(mColor));
    if (!videoBuffer) {
        core->deinit(core);
        core = NULL;
        return -3;
    }
    core->setVideoBuffer(core, videoBuffer, WIDTH);
    /* Configure audio */
    core->setAudioBufferSize(core, AUDIO_SAMPLES);
    return 1;
}

/* Step-by-step creation for debugging */
EMSCRIPTEN_KEEPALIVE
int mgba_create_step1(void) {
    core = mCoreCreate(mPLATFORM_GBA);
    return core ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE
int mgba_create_step2(void) {
    if (!core) return 0;
    if (!core->init(core)) return 0;
    mCoreInitConfig(core, NULL);
    return 1;
}

EMSCRIPTEN_KEEPALIVE
int mgba_create_step3(void) {
    if (!core) return 0;
    videoBuffer = (mColor *)malloc(WIDTH * HEIGHT * sizeof(mColor));
    if (!videoBuffer) return 0;
    core->setVideoBuffer(core, videoBuffer, WIDTH);
    core->setAudioBufferSize(core, AUDIO_SAMPLES);
    return 1;
}

EMSCRIPTEN_KEEPALIVE
int mgba_load_rom(const uint8_t *data, int size) {
    if (!core || !data || size <= 0) return 0;
    /* Free previous ROM data */
    if (romData) {
        free(romData);
        romData = NULL;
    }
    /* Copy ROM data so mGBA can own the VFile */
    romData = (uint8_t *)malloc(size);
    if (!romData) return 0;
    memcpy(romData, data, size);
    romSize = size;

    struct VFile *vf = VFileFromMemory(romData, romSize);
    if (!vf) return -2;
    if (!core->loadROM(core, vf)) return -3;
    return 1;
}

EMSCRIPTEN_KEEPALIVE
int mgba_reset(void) {
    if (!core) return 0;
    core->reset(core);
    return 1;
}

EMSCRIPTEN_KEEPALIVE
void mgba_run_frame(void) {
    if (!core) return;
    core->runFrame(core);
}

EMSCRIPTEN_KEEPALIVE
uint8_t *mgba_get_pixels(void) {
    if (!core || !videoBuffer) return NULL;
    /* Convert mColor buffer to RGBA */
    int i;
    for (i = 0; i < WIDTH * HEIGHT; i++) {
        mColor c = videoBuffer[i];
#ifdef COLOR_16_BIT
        rgbaBuffer[i * 4 + 0] = M_R8(c);
        rgbaBuffer[i * 4 + 1] = M_G8(c);
        rgbaBuffer[i * 4 + 2] = M_B8(c);
        rgbaBuffer[i * 4 + 3] = 255;
#else
        rgbaBuffer[i * 4 + 0] = (c >>  0) & 0xFF;
        rgbaBuffer[i * 4 + 1] = (c >>  8) & 0xFF;
        rgbaBuffer[i * 4 + 2] = (c >> 16) & 0xFF;
        rgbaBuffer[i * 4 + 3] = 255;
#endif
    }
    return rgbaBuffer;
}

EMSCRIPTEN_KEEPALIVE
void mgba_set_keys(int keys) {
    if (!core) return;
    core->setKeys(core, (uint32_t)keys);
}

EMSCRIPTEN_KEEPALIVE
int mgba_get_width(void) {
    return WIDTH;
}

EMSCRIPTEN_KEEPALIVE
int mgba_get_height(void) {
    return HEIGHT;
}

EMSCRIPTEN_KEEPALIVE
uint32_t mgba_read8(uint32_t addr) {
    if (!core) return 0;
    return core->busRead8(core, addr);
}

EMSCRIPTEN_KEEPALIVE
uint32_t mgba_read16(uint32_t addr) {
    if (!core) return 0;
    return core->busRead16(core, addr);
}

EMSCRIPTEN_KEEPALIVE
uint32_t mgba_read32(uint32_t addr) {
    if (!core) return 0;
    return core->busRead32(core, addr);
}

EMSCRIPTEN_KEEPALIVE
void mgba_write8(uint32_t addr, uint8_t val) {
    if (!core) return;
    core->busWrite8(core, addr, val);
}

EMSCRIPTEN_KEEPALIVE
void mgba_write16(uint32_t addr, uint16_t val) {
    if (!core) return;
    core->busWrite16(core, addr, val);
}

EMSCRIPTEN_KEEPALIVE
uint32_t mgba_frame_counter(void) {
    if (!core) return 0;
    return core->frameCounter(core);
}

EMSCRIPTEN_KEEPALIVE
int mgba_save_state_size(void) {
    if (!core) return 0;
    return (int)core->stateSize(core);
}

EMSCRIPTEN_KEEPALIVE
int mgba_save_state(void *buf) {
    if (!core || !buf) return 0;
    return core->saveState(core, buf) ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE
int mgba_load_state(const void *buf) {
    if (!core || !buf) return 0;
    return core->loadState(core, buf) ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE
int mgba_get_audio_samples(void) {
    return audioSamplesReady;
}

EMSCRIPTEN_KEEPALIVE
int16_t *mgba_get_audio_buffer(void) {
    if (!core) return NULL;
    struct mAudioBuffer *abuf = core->getAudioBuffer(core);
    if (!abuf) return NULL;
    audioSamplesReady = (int)mAudioBufferAvailable(abuf);
    if (audioSamplesReady > AUDIO_SAMPLES) audioSamplesReady = AUDIO_SAMPLES;
    if (audioSamplesReady > 0) {
        mAudioBufferRead(abuf, audioBuffer, audioSamplesReady);
    }
    return audioBuffer;
}

EMSCRIPTEN_KEEPALIVE
void mgba_destroy(void) {
    if (core) {
        mCoreConfigDeinit(&core->config);
        core->deinit(core);
        core = NULL;
    }
    if (videoBuffer) {
        free(videoBuffer);
        videoBuffer = NULL;
    }
    if (romData) {
        free(romData);
        romData = NULL;
    }
}
