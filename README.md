Dump & restore decrypted savefile.
[![Build Status](https://travis-ci.org/d3m3vilurr/vita-savemgr.svg?branch=master)](https://travis-ci.org/d3m3vilurr/vita-savemgr)

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

## License
GPLv3

## Credits
Project use these project's codes.

* [VitaShell][]
* [rinCheat][]

[VitaShell]: https://github.com/TheOfficialFloW/VitaShell
[rinCheat]: https://github.com/Rinnegatamante/rinCheat
