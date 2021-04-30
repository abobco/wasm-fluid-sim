'''
Build project for windows x64 and wasm targets

python3 build_all.py Release

python3 build_all.py Debug

'''

import sys
import os
from distutils import dir_util
import subprocess

build_type = 'Release'
if len(sys.argv) > 1:
    build_type = sys.argv[1]

dir_util.copy_tree('assets', 'build/assets')
dir_util.copy_tree('assets', 'build_wasm/assets')
dir_util.copy_tree('assets', 'build_wasm/emscripten/assets')
dir_util.copy_tree('assets', 'build_wasm/html/assets')

os.chdir('build')
subprocess.run(['cmake', '..'])
subprocess.run(['cmake', '--build', '.', '--config', build_type])

os.chdir('../build_wasm')
subprocess.run(['cmake', '.'])
subprocess.run(['cmake', '--build', '.', '--config', build_type])
