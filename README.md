Dump & restore decrypted savefile.
[![Build Status](https://travis-ci.org/d3m3vilurr/vita-savemgr.svg?branch=master)](https://travis-ci.org/d3m3vilurr/vita-savemgr)

## How to use
1. Install vpk
2. Launch savemgr in Home shell
3. Select game
4. Wait 3sec, will auto launch dumper
5. Dump or Restore game save.
6. Exit with X (DO NOT FORCE CLOSE)
7. Auto relaunch savemgr, then cleanup injected things.
8. Repeat 3~7 steps or close savemgr
9. Play game.

## Configure
If you want to use another dump directory format, make simple `config.ini` file into `ux0:data/savemgr`

- use `ux0:data/savegames/PCSH00000_SLOT0`

```ini
base=/data/savegames
slot_format=%s_SLOT%d
```

Default config like this; it will save to `ux0:data/savegames/PCSH00000/SLOT0`

```ini
base=/data/savegames
slot_format=%s/SLOT%d
```

## Emergency guide
If you do something mess, please do these steps.

1. Open VitaShell
2. If you used this app for dumped or digital game, move to `ux0:app/{TITLE_ID}`.
  If used for cartridge game, move to `ux0:patch`

3. follow your case;

  1. Remove `eboot.bin` and rename `eboot.bin.orig` to `eboot.bin`
  2. Remove `{TITLE_ID}` directory and rename `{TITLE_ID}_orig` to `{TITLE_ID}` if exists directory

4. Check `ux0:data/savemgr/tmp`, if exists, remove this.

## License
GPLv3

## Credits
Project use these project's codes.

* [VitaShell][]
* [rinCheat][]

[VitaShell]: https://github.com/TheOfficialFloW/VitaShell
[rinCheat]: https://github.com/Rinnegatamante/rinCheat
