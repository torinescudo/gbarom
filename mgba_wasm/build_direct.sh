#!/usr/bin/env bash
set -e

# Activate Emscripten
source "/workspaces/gbarom/emsdk/emsdk_env.sh" 2>/dev/null

MGBA="/workspaces/gbarom/mgba"
BUILD="/workspaces/gbarom/mgba_wasm"
GLUE="/workspaces/gbarom/mgba_wasm/mgba_glue.c"
OUT="/workspaces/gbarom/web_emulator"

# Common defines matching the library build
DEFS="-D_GNU_SOURCE -DMINIMAL_CORE=2 -DDISABLE_THREADING -DM_CORE_GBA -DENABLE_VFS -DENABLE_VFS_FD -DDISABLE_ANON_MMAP -DHAVE_STRDUP -DHAVE_STRLCPY -DHAVE_LOCALTIME_R -DHAVE_STRTOF_L -DHAVE_XLOCALE"
INCS="-I$MGBA/include -I$BUILD/include -I$MGBA/src -I$MGBA/src/third-party"
WARNS="-Wno-implicit-function-declaration -Wno-int-conversion -Wno-incompatible-pointer-types -Wno-unterminated-string-initialization -Wno-format"

# All mGBA source files needed for GBA core
SRCS="
$MGBA/src/core/bitmap-cache.c
$MGBA/src/core/cache-set.c
$MGBA/src/core/cheats.c
$MGBA/src/core/config.c
$MGBA/src/core/core.c
$MGBA/src/core/directories.c
$MGBA/src/core/input.c
$MGBA/src/core/interface.c
$MGBA/src/core/lockstep.c
$MGBA/src/core/log.c
$MGBA/src/core/map-cache.c
$MGBA/src/core/mem-search.c
$MGBA/src/core/rewind.c
$MGBA/src/core/serialize.c
$MGBA/src/core/sync.c
$MGBA/src/core/thread.c
$MGBA/src/core/tile-cache.c
$MGBA/src/core/timing.c
$MGBA/src/core/library.c
$MGBA/src/arm/arm.c
$MGBA/src/arm/decoder-arm.c
$MGBA/src/arm/decoder.c
$MGBA/src/arm/decoder-thumb.c
$MGBA/src/arm/isa-arm.c
$MGBA/src/arm/isa-thumb.c
$MGBA/src/gb/audio.c
$MGBA/src/gba/audio.c
$MGBA/src/gba/bios.c
$MGBA/src/gba/cart/ereader.c
$MGBA/src/gba/cart/gpio.c
$MGBA/src/gba/cart/matrix.c
$MGBA/src/gba/cart/unlicensed.c
$MGBA/src/gba/cart/vfame.c
$MGBA/src/gba/cheats.c
$MGBA/src/gba/cheats/codebreaker.c
$MGBA/src/gba/cheats/gameshark.c
$MGBA/src/gba/cheats/parv3.c
$MGBA/src/gba/core.c
$MGBA/src/gba/dma.c
$MGBA/src/gba/gba.c
$MGBA/src/gba/hle-bios.c
$MGBA/src/gba/input.c
$MGBA/src/gba/io.c
$MGBA/src/gba/memory.c
$MGBA/src/gba/overrides.c
$MGBA/src/gba/renderers/cache-set.c
$MGBA/src/gba/renderers/common.c
$MGBA/src/gba/renderers/gl.c
$MGBA/src/gba/renderers/software-bg.c
$MGBA/src/gba/renderers/software-mode0.c
$MGBA/src/gba/renderers/software-obj.c
$MGBA/src/gba/renderers/video-software.c
$MGBA/src/gba/savedata.c
$MGBA/src/gba/serialize.c
$MGBA/src/gba/sharkport.c
$MGBA/src/gba/sio.c
$MGBA/src/gba/sio/gbp.c
$MGBA/src/gba/timer.c
$MGBA/src/gba/video.c
$MGBA/src/util/circle-buffer.c
$MGBA/src/util/configuration.c
$MGBA/src/util/crc32.c
$MGBA/src/util/formatting.c
$MGBA/src/util/gbk-table.c
$MGBA/src/util/hash.c
$MGBA/src/util/md5.c
$MGBA/src/util/sha1.c
$MGBA/src/util/string.c
$MGBA/src/util/table.c
$MGBA/src/util/vector.c
$MGBA/src/util/vfs.c
$MGBA/src/util/audio-buffer.c
$MGBA/src/util/audio-resampler.c
$MGBA/src/util/convolve.c
$MGBA/src/util/elf-read.c
$MGBA/src/util/geometry.c
$MGBA/src/util/image.c
$MGBA/src/util/image/export.c
$MGBA/src/util/image/font.c
$MGBA/src/util/image/png-io.c
$MGBA/src/util/interpolator.c
$MGBA/src/util/patch.c
$MGBA/src/util/patch-fast.c
$MGBA/src/util/patch-ips.c
$MGBA/src/util/patch-ups.c
$MGBA/src/util/ring-fifo.c
$MGBA/src/util/sfo.c
$MGBA/src/util/text-codec.c
$MGBA/src/util/vfs/vfs-mem.c
$MGBA/src/util/vfs/vfs-fifo.c
$MGBA/src/util/memory.c
$MGBA/src/third-party/inih/ini.c
$MGBA/src/util/vfs/vfs-fd.c
$BUILD/version.c
"

echo "=== Compiling mGBA + glue directly to WASM ==="
echo "Source files: $(echo $SRCS | wc -w)"

emcc $GLUE $SRCS \
    $DEFS $INCS $WARNS \
    -O2 \
    -s WASM=1 \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s INITIAL_MEMORY=134217728 \
    -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap","HEAPU8","HEAP16","HEAP32"]' \
    -s EXPORTED_FUNCTIONS='["_mgba_create","_mgba_create_step1","_mgba_create_step2","_mgba_create_step3","_mgba_load_rom","_mgba_reset","_mgba_run_frame","_mgba_get_pixels","_mgba_set_keys","_mgba_get_width","_mgba_get_height","_mgba_read8","_mgba_read16","_mgba_read32","_mgba_write8","_mgba_write16","_mgba_frame_counter","_mgba_save_state_size","_mgba_save_state","_mgba_load_state","_mgba_get_audio_samples","_mgba_get_audio_buffer","_mgba_destroy","_malloc","_free"]' \
    -s MODULARIZE=1 \
    -s EXPORT_NAME="createMGBA" \
    -o "$OUT/mgba.js"

echo ""
echo "=== Build complete! ==="
ls -lh "$OUT/mgba.js" "$OUT/mgba.wasm"
