"""Build the firmware for the browser simulator (WebAssembly).

    python tools/build_sim.py            # needs emcc on PATH (emsdk) and LVGL (see LVGL_DIR)
    LVGL_DIR=/path/to/lvgl python tools/build_sim.py

Output: build/sim/walkie.js + walkie.wasm, copied to ../simulator/public/fw/."""
import glob, os, shutil, subprocess, sys
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LVGL = os.environ.get('LVGL_DIR', os.path.join(ROOT, 'components', 'lvgl'))
OUT = os.path.join(ROOT, 'build-sim')
OBJ = os.path.join(OUT, 'obj')
DEST = os.path.join(ROOT, '..', 'simulator', 'public', 'fw')
EMCC = os.environ.get('EMCC', 'emcc')

INCLUDES = ['core', 'core/include', 'core/crypto', 'core/venues', 'core/ui', 'core/fonts', 'third_party/cjson',
            'third_party/monocypher', 'third_party/secp256k1/include']
DEFINES = ['LV_CONF_INCLUDE_SIMPLE', 'ENABLE_MODULE_RECOVERY=1', 'ECMULT_WINDOW_SIZE=4', 'COMB_BLOCKS=11', 'COMB_TEETH=6']


def sources():
    src = glob.glob(os.path.join(LVGL, 'src', '**', '*.c'), recursive=True)
    src += glob.glob(os.path.join(ROOT, 'core', '**', '*.c'), recursive=True)
    src += glob.glob(os.path.join(ROOT, 'sim', '*.c'))
    tp = os.path.join(ROOT, 'third_party')
    src += [os.path.join(tp, 'cjson', 'cJSON.c'), os.path.join(tp, 'monocypher', 'monocypher.c'),
            os.path.join(tp, 'monocypher', 'monocypher-ed25519.c'), os.path.join(tp, 'secp256k1', 'src', 'secp256k1.c'),
            os.path.join(tp, 'secp256k1', 'src', 'precomputed_ecmult.c'),
            os.path.join(tp, 'secp256k1', 'src', 'precomputed_ecmult_gen.c')]
    return src


def obj_path(src):
    rel = os.path.relpath(src, ROOT) if src.startswith(ROOT) else 'lvgl_' + os.path.relpath(src, LVGL)
    return os.path.join(OBJ, rel.replace(os.sep, '_').replace('..', 'up').replace(':', '') + '.o')


def compile_one(src):
    o = obj_path(src)
    newest = max(os.path.getmtime(src), os.path.getmtime(os.path.join(ROOT, 'core', 'lv_conf.h')))
    if os.path.exists(o) and os.path.getmtime(o) > newest and 'core' not in src and 'sim' not in src:
        return None
    flags = ['-O2', '-std=gnu11', '-Wall', '-Wno-unused-function', '-Wno-unused-variable']
    if 'secp256k1' in src or 'lvgl' in src.lower() and 'core' not in src:
        flags = ['-O2', '-std=gnu11', '-w']
    cmd = [EMCC, *flags, '-c', src, '-o', o] + [f'-I{os.path.join(ROOT, i)}' for i in INCLUDES] + \
          [f'-I{LVGL}', f'-I{os.path.join(LVGL, "src")}'] + [f'-D{d}' for d in DEFINES]
    r = subprocess.run(cmd, capture_output=True, text=True, shell=False)
    if r.returncode:
        return f'{src}\n{r.stderr}'
    if r.stderr.strip() and 'warning' in r.stderr:
        print(r.stderr.strip()[:2000])
    return None


def main():
    os.makedirs(OBJ, exist_ok=True)
    src = sources()
    with ThreadPoolExecutor(max_workers=os.cpu_count() or 4) as ex:
        errors = [e for e in ex.map(compile_one, src) if e]
    if errors:
        print('\n\n'.join(errors[:8]))
        sys.exit(1)
    exports = ['_main', '_malloc', '_free', '_dw_sim_fb', '_dw_sim_touch', '_dw_sim_input', '_dw_sim_http_done',
               '_dw_sim_ws_msg', '_dw_sim_ws_state', '_dw_sim_voice']
    out_js = os.path.join(OUT, 'walkie.js')
    rsp = os.path.join(OUT, 'objs.rsp')
    with open(rsp, 'w') as f:
        f.write('\n'.join('"' + obj_path(s).replace(os.sep, '/') + '"' for s in src))
    cmd = [EMCC, '-O2', '@' + rsp, '-o', out_js, '-sALLOW_MEMORY_GROWTH=1', '-sINITIAL_MEMORY=33554432',
           '-sMODULARIZE=1', '-sEXPORT_NAME=createWalkie', '-sENVIRONMENT=web', '-sSTACK_SIZE=262144',
           f'-sEXPORTED_FUNCTIONS={",".join(exports)}',
           '-sEXPORTED_RUNTIME_METHODS=UTF8ToString,stringToNewUTF8,HEAPU8,HEAPU16,lengthBytesUTF8,stringToUTF8']
    r = subprocess.run(cmd, capture_output=True, text=True, shell=False)
    if r.returncode:
        print(r.stderr)
        sys.exit(1)
    os.makedirs(DEST, exist_ok=True)
    for f in ('walkie.js', 'walkie.wasm'):
        shutil.copy(os.path.join(OUT, f), os.path.join(DEST, f))
    print('ok', {f: os.path.getsize(os.path.join(OUT, f)) // 1024 for f in ('walkie.js', 'walkie.wasm')}, 'KB ->', os.path.normpath(DEST))


if __name__ == '__main__':
    main()
