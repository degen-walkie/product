"""Generate the LVGL fonts used by the UI (same families as degenwalkie.trade).
Needs: npm i lv_font_conv, and static TTF instances in FONT_DIR (see README)."""
import os, subprocess, sys

FONT_DIR = os.environ.get('DW_FONT_DIR', os.path.expanduser('~/tools/fonts'))
CONV = os.environ.get('LV_FONT_CONV', 'lv_font_conv')
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'core', 'fonts')
RANGE = '0x20-0x7E,0xB0,0xB7,0xD7,0x2013,0x2014,0x2018,0x2019,0x201C,0x201D,0x2022,0x2026,0x2190-0x2193,0x2248,0x2264,0x2265'
FONTS = [  # name, ttf, size
    ('dw_mono_9', 'JBM-Medium.ttf', 9), ('dw_mono_11', 'JBM-Medium.ttf', 11), ('dw_mono_13', 'JBM-Medium.ttf', 13),
    ('dw_mono_15', 'JBM-Medium.ttf', 15), ('dw_mono_24', 'JBM-Medium.ttf', 24), ('dw_mono_30', 'JBM-Medium.ttf', 30),
    ('dw_monob_9', 'JBM-Bold.ttf', 9), ('dw_monob_11', 'JBM-Bold.ttf', 11), ('dw_monob_13', 'JBM-Bold.ttf', 13),
    ('dw_monob_18', 'JBM-Bold.ttf', 18),
    ('dw_sans_12', 'Inter-Regular.ttf', 12), ('dw_sans_14', 'Inter-Regular.ttf', 14), ('dw_sansb_15', 'Inter-SemiBold.ttf', 15),
    ('dw_disp_14', 'ArchivoXC-Black.ttf', 14), ('dw_disp_20', 'ArchivoXC-Black.ttf', 20), ('dw_disp_24', 'ArchivoXC-Black.ttf', 24),
    ('dw_disp_32', 'ArchivoXC-Black.ttf', 32),
]
os.makedirs(OUT, exist_ok=True)
for name, ttf, size in FONTS:
    out = os.path.join(OUT, name + '.c')
    cmd = [CONV, '--no-compress', '--no-prefilter', '--bpp', '4', '--size', str(size), '--format', 'lvgl',
           '--font', os.path.join(FONT_DIR, ttf), '-r', RANGE, '--lv-include', 'lvgl.h', '-o', out]
    r = subprocess.run(cmd, capture_output=True, text=True, shell=sys.platform == 'win32')
    if r.returncode: print(r.stderr); sys.exit(1)
    print(name, os.path.getsize(out) // 1024, 'KB')
with open(os.path.join(OUT, 'dw_fonts.h'), 'w') as f:
    f.write('#pragma once\n#include "lvgl.h"\n\n' + ''.join(f'LV_FONT_DECLARE({n})\n' for n, _, _ in FONTS))
