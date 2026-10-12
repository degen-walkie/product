"""Generate the LVGL fonts used by the UI (same families as degenwalkie.trade).

Latin fonts cover ASCII, Latin-1, Latin Extended-A and Vietnamese, so Spanish,
Portuguese, Turkish and Vietnamese render. dw_cjk_14 (Noto Sans SC + KR) holds the
3,755 GB2312 level-1 hanzi, kana, CJK punctuation and the 2,350 KS X 1001 hangul;
the text fonts fall back to it, so translated rooms and AI answers render too.

Needs: npm i lv_font_conv, and static TTF instances in FONT_DIR (see README)."""
import os, re, subprocess, sys

FONT_DIR = os.environ.get('DW_FONT_DIR', os.path.expanduser('~/tools/fonts'))
CONV = os.environ.get('LV_FONT_CONV', 'lv_font_conv')
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'core', 'fonts')
LATIN = ('0x20-0x7E,0xA0-0x17F,0x1EA0-0x1EF9,0x2013,0x2014,0x2018,0x2019,0x201C,0x201D,0x2022,0x2026,'
         '0x2190-0x2193,0x2212,0x2248,0x2264,0x2265')
FONTS = [  # name, ttf, size
    ('dw_mono_9', 'JBM-Medium.ttf', 9), ('dw_mono_11', 'JBM-Medium.ttf', 11), ('dw_mono_13', 'JBM-Medium.ttf', 13),
    ('dw_mono_15', 'JBM-Medium.ttf', 15), ('dw_mono_24', 'JBM-Medium.ttf', 24), ('dw_mono_30', 'JBM-Medium.ttf', 30),
    ('dw_monob_9', 'JBM-Bold.ttf', 9), ('dw_monob_11', 'JBM-Bold.ttf', 11), ('dw_monob_13', 'JBM-Bold.ttf', 13),
    ('dw_monob_18', 'JBM-Bold.ttf', 18),
    ('dw_sans_12', 'Inter-Regular.ttf', 12), ('dw_sans_14', 'Inter-Regular.ttf', 14), ('dw_sansb_15', 'Inter-SemiBold.ttf', 15),
    ('dw_disp_14', 'ArchivoXC-Black.ttf', 14), ('dw_disp_20', 'ArchivoXC-Black.ttf', 20), ('dw_disp_24', 'ArchivoXC-Black.ttf', 24),
    ('dw_disp_32', 'ArchivoXC-Black.ttf', 32),
]
FALLBACK = {'dw_sans_12', 'dw_sans_14', 'dw_sansb_15', 'dw_mono_11', 'dw_mono_13', 'dw_monob_11', 'dw_mono_9'}


def run(args):
    r = subprocess.run([CONV] + args, capture_output=True, text=True, shell=sys.platform == 'win32')
    if r.returncode:
        print(r.stderr)
        sys.exit(1)


def common(name, size):
    return ['--no-compress', '--no-prefilter', '--bpp', '4', '--size', str(size), '--format', 'lvgl', '--lv-include', 'lvgl.h',
            '-o', os.path.join(OUT, name + '.c')]


os.makedirs(OUT, exist_ok=True)
for name, ttf, size in FONTS:
    run(common(name, size) + ['--font', os.path.join(FONT_DIR, ttf), '-r', LATIN])
    path = os.path.join(OUT, name + '.c')
    if name in FALLBACK:
        s = open(path, encoding='utf-8').read()
        s = s.replace('#include "lvgl.h"', '#include "lvgl.h"\nextern const lv_font_t dw_cjk_14;', 1)
        s = re.sub(r'(\.dsc = &font_dsc)(\s*/\*.*?\*/)?\n', r'\1,\2\n    .fallback = &dw_cjk_14,\n', s, count=1)
        open(path, 'w', encoding='utf-8').write(s)
    print(name, os.path.getsize(path) // 1024, 'KB')

han = open(os.path.join(FONT_DIR, 'han.txt'), encoding='utf-8').read()
hangul = open(os.path.join(FONT_DIR, 'hangul.txt'), encoding='utf-8').read()
run(common('dw_cjk_14', 14) + ['--font', os.path.join(FONT_DIR, 'NotoSC-Regular.ttf'), '-r', '0x3000-0x303F,0x3040-0x30FF,0xFF01-0xFF5E',
                              '--symbols', han, '--font', os.path.join(FONT_DIR, 'NotoKR-Regular.ttf'), '--symbols', hangul])
print('dw_cjk_14', os.path.getsize(os.path.join(OUT, 'dw_cjk_14.c')) // 1024, 'KB')

with open(os.path.join(OUT, 'dw_fonts.h'), 'w') as f:
    f.write('#pragma once\n#include "lvgl.h"\n\n' + ''.join(f'LV_FONT_DECLARE({n})\n' for n, _, _ in FONTS) + 'LV_FONT_DECLARE(dw_cjk_14)\n')
