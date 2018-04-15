## Unreleased
- Re-add list mode (#69, 266fad1d)
- Add button only mode (#63, e0641e54, 3e6796bd, 864814fa)

## 2.0.0
- Re-enable package compression (cdf1833c)
- Fix confirm button always ASIA mode (#59)
- Fix not shown all games (#62)
- Fix crash if not have icon files (#68)
- Embed VitaShell modules into vpk file (#66)

## 2.0.0-b1
- Move 2.0.0 tree
- Support GUI
- No more needs app switching

## 1.0.0
- Block PS button on the critical sections
- Reword import/export to restore/backup (#32)
- Use newer vita-toolchain for CI build
- Use ninja-build for CI build
- Fix cannot earning achievement (#34)
- Enable package compress

## 0.8.0
- Change default directories to `ux0:/data/savegames`
- Auto migrate older directories
- Support drop save slot
- Support confirm popups
- Support format savedata (L+R trigger on dumper main menu)

## 0.7.0
- Support device confirmation button & fix text grammers (#16)
- Use icon for button information instead text
- Support left analog & up/down hold for select item
- Support multiple save dump slots
- Support configure

## 0.6.0
- Fix broken import process of 0.5.0
- Support save export/import of abnormal titleid games like YS8 HK
- Add icon & livearea :)
- Change name `savemgr` to `Save Manager`

## 0.5.0
This version can be unstable. Please backup using CMA before use
this application.

- Support encrypted digital games; Thanks @mopi1402 (#11)
- Revert some codes for Gundam Breaker3 AV; so may not support this game. :(
- Remove rinCheat's code

## 0.4.1
- Fix cannot decrypt problem of physical copy gundam breaker 3 asia version

Known issues
- not support cartridge version muramasa rebirth (#3)
- cannot encrypt cartridge version gundom breaker3 (#9)

## 0.4.0
- Fix crash at startup with non print games.
- Support unmatched TITLE_ID games like Gundam Breaker 3 Asia version.

## 0.3.0
- Add helper messages
- Detect cartridge for save dump

## 0.2.0
- Support screen scrolling; now can use more then 20 games.
- Always use screen bottom area for print button commands
- Some code clean up :)

## 0.1.0
Initial release

Dump / Restore for PSVita game save

- alpha release
- support cartridge & vitamin games
- dump datas have compatibility with rinCheat and also data store
  into ux0:data/rinCheat
