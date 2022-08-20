# -*- mode: python ; coding: utf-8 -*-

block_cipher = None

SETUP_DIR = '/home/sgx/repoes/qshield/rest/'

a = Analysis(['app.py',
              'apis.py',
              'config.py',
              'coroweb.py',
              'field.py',
              'format.py',
              'gl.py',
              'handlers.py',
              'tools.py',
              SETUP_DIR + 'conf/config_default.py',
              SETUP_DIR + 'conf/config_override.py',
              SETUP_DIR + 'models/qshield/models.py',
              SETUP_DIR + 'models/qshield/orm.py',
              SETUP_DIR + 'timers/BaseTimer.py',
              SETUP_DIR + 'timers/PipelineTimer.py'],
             pathex=['/home/sgx/repoes/qshield/rest'],
             binaries=[],
             datas=[],
             hiddenimports=['models', 'orm', 'timers', 'conf'],
             hookspath=[],
             runtime_hooks=[],
             excludes=[],
             win_no_prefer_redirects=False,
             win_private_assemblies=False,
             cipher=block_cipher,
             noarchive=False)
pyz = PYZ(a.pure, a.zipped_data,
             cipher=block_cipher)
exe = EXE(pyz,
          a.scripts,
          a.binaries,
          a.zipfiles,
          a.datas,
          [],
          name='app',
          debug=False,
          bootloader_ignore_signals=False,
          strip=False,
          upx=True,
          upx_exclude=[],
          runtime_tmpdir=None,
          console=True )
