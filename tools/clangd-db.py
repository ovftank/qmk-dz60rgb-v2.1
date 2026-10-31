#!/usr/bin/env python3
import json
import os
import pathlib
import subprocess

KEYBOARD = 'dztech/dz60rgb/v2_1'
KEYMAP = 'ovftank'
KEYMAP_DIR = pathlib.Path('keyboards/dztech/dz60rgb/keymaps') / KEYMAP
QMK_SOURCE = 'quantum/keymap_introspection.c'

repo = pathlib.Path(__file__).resolve().parent.parent
qmk = pathlib.Path(os.environ.get('QMK_HOME', '~/qmk_firmware')).expanduser()
qmk_db = qmk / 'compile_commands.json'

flags_sources = [repo / KEYMAP_DIR / 'config.h', repo / KEYMAP_DIR / 'rules.mk']
if not qmk_db.exists() or any(p.stat().st_mtime > qmk_db.stat().st_mtime for p in flags_sources):
    subprocess.run(['qmk', 'compile', '--compiledb', '-kb', KEYBOARD, '-km', KEYMAP], check=True, cwd=qmk)

entry = next(e for e in json.loads(qmk_db.read_text()) if e['file'] == QMK_SOURCE)
args = list(entry['arguments'])

source = str(repo / KEYMAP_DIR / 'keymap.c')
index = args.index(QMK_SOURCE)
args[index] = source
for flag in ('-o', '-MF'):
    if flag in args:
        pos = args.index(flag)
        del args[pos:pos + 2]

out = repo / 'compile_commands.json'
out.write_text(json.dumps([{'directory': entry['directory'], 'file': source, 'arguments': args}], indent=1))
print(f'wrote {out}')
