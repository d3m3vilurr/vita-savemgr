Dump & restore decrypted savefile.
[![Build Status](https://travis-ci.org/d3m3vilurr/vita-savemgr.svg?branch=master)](https://travis-ci.org/d3m3vilurr/vita-savemgr)

## Configure
R trigger in mainscreen will open simple configure screen.
Or you can change `ux0:data/savemgr/config.ini` manually.

If you want to use another dump directory format, current time you need modify
ini file.

for example, if you want to use `ux0:data/savegames/PCSH00000_SLOT0`,

```ini
base=/data/savegames
slot_format=%s_SLOT%d
```

Default ini config is below;

```ini
base=/data/savegames
slot_format=%s/SLOT%d
list_mode=icon
use_dpad=true
```

## Development
Need VitaShell's [modules][];
install `kernel` and `user` and copy `kernel.skprx` and `user.suprx` into `sce_sys`.
then you can build vpk.

```bash
mkdir build
cd build && cmake -GNinja .. && ninja`
```

## License
GPLv3

## Credits
Project use these project's codes.

* [VitaShell][]
* [rinCheat][]

[modules]: https://github.com/TheOfficialFloW/VitaShell/tree/master/modules
[VitaShell]: https://github.com/TheOfficialFloW/VitaShell
[rinCheat]: https://github.com/Rinnegatamante/rinCheat
