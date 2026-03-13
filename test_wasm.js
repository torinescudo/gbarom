// Node.js test for mGBA WASM module
const fs = require('fs');
const path = require('path');

async function test() {
    console.log('Loading mGBA WASM module...');
    
    // Load the module
    const createMGBA = require('./web_emulator/mgba.js');
    
    let mgba;
    try {
        mgba = await createMGBA();
        console.log('Module loaded OK');
    } catch(e) {
        console.error('Module load failed:', e.message);
        process.exit(1);
    }
    
    // Test create - step by step
    console.log('Step 1: mCoreCreate()...');
    try {
        const step1 = mgba.cwrap('mgba_create_step1', 'number', []);
        const r1 = step1();
        console.log('  Step1 returned:', r1);
        if (!r1) { console.error('mCoreCreate failed'); process.exit(1); }
    } catch(e) {
        console.error('Step1 error:', e.message);
        process.exit(1);
    }

    console.log('Step 2: core->init()...');
    try {
        const step2 = mgba.cwrap('mgba_create_step2', 'number', []);
        const r2 = step2();
        console.log('  Step2 returned:', r2);
        if (!r2) { console.error('core->init failed'); process.exit(1); }
    } catch(e) {
        console.error('Step2 error:', e.message);
        process.exit(1);
    }

    console.log('Step 3: setVideoBuffer + setAudioBufferSize...');
    try {
        const step3 = mgba.cwrap('mgba_create_step3', 'number', []);
        const r3 = step3();
        console.log('  Step3 returned:', r3);
        if (!r3) { console.error('step3 failed'); process.exit(1); }
    } catch(e) {
        console.error('Step3 error:', e.message);
        process.exit(1);
    }
    console.log('Core fully initialized!');
    
    // Test load ROM
    const romPath = path.join(__dirname, 'pokeemerald', 'pokeemerald.gba');
    if (fs.existsSync(romPath)) {
        console.log('Loading ROM from', romPath);
        const romData = fs.readFileSync(romPath);
        const romBytes = new Uint8Array(romData);
        
        const ptr = mgba._malloc(romBytes.length);
        mgba.HEAPU8.set(romBytes, ptr);
        
        try {
            const mgbaLoadRom = mgba.cwrap('mgba_load_rom', 'number', ['number','number']);
            const ok = mgbaLoadRom(ptr, romBytes.length);
            console.log('mgba_load_rom() returned:', ok);
            mgba._free(ptr);
            
            if (ok > 0) {
                // Now try reset separately
                console.log('Calling mgba_reset()...');
                const mgbaReset = mgba.cwrap('mgba_reset', 'number', []);
                const resetOk = mgbaReset();
                console.log('mgba_reset() returned:', resetOk);
                
                if (resetOk) {
                    // Test running a frame
                    const mgbaRunFrame = mgba.cwrap('mgba_run_frame', null, []);
                    console.log('Running 5 frames...');
                    for (let i = 0; i < 5; i++) {
                        mgbaRunFrame();
                        console.log('  Frame', i+1, 'OK');
                    }
                    
                    // Test get pixels
                    const mgbaGetPixels = mgba.cwrap('mgba_get_pixels', 'number', []);
                    const pixPtr = mgbaGetPixels();
                    console.log('mgba_get_pixels() returned ptr:', pixPtr);
                    if (pixPtr) {
                        const px = new Uint8Array(mgba.HEAPU8.buffer, pixPtr, 16);
                        console.log('First 16 bytes of pixel data:', Array.from(px));
                    }
                    
                    console.log('\n=== ALL TESTS PASSED ===');
                }
            }
        } catch(e) {
            console.error('ROM load/run error:', e.message);
            console.error(e.stack);
            mgba._free(ptr);
            process.exit(1);
        }
    } else {
        console.log('ROM not found at', romPath, '- skipping ROM test');
        console.log('mgba_create() works - core init OK');
    }
    
    // Cleanup
    const mgbaDestroy = mgba.cwrap('mgba_destroy', null, []);
    mgbaDestroy();
    console.log('Cleanup done');
}

test().catch(e => {
    console.error('Fatal:', e);
    process.exit(1);
});
