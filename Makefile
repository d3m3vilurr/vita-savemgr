TITLE_ID = SAVEMGR00
TARGET = savemgr
OBJS   = main.o savedata.o file.o appdb.o vita_sqlite.o sqlite3/sqlite3.o

LIBS = -lSceKernel_stub -lSceDisplay_stub -lSceGxm_stub \
	   -lSceAppMgr_stub -lSceAppUtil_stub -lSceSysmodule_stub \
	   -lSceCtrl_stub -lScePgf_stub -lz -lm \
	   -lsqlite3 -lvita2d -lfreetype

PREFIX  = arm-vita-eabi
CC      = $(PREFIX)-gcc
CFLAGS  = -Wl,-q -Wall -O3 -std=c99 \
		  -DSQLITE_OS_OTHER=1 -DSQLITE_TEMP_STORE=3 -DSQLITE_THREADSAFE=0
ASFLAGS = $(CFLAGS)

all: $(TARGET).vpk

%.vpk: eboot.bin
	vita-mksfoex -s TITLE_ID=$(TITLE_ID) "$(TARGET)" param.sfo
	vita-pack-vpk -s param.sfo -b eboot.bin $@

eboot.bin: $(TARGET).velf
	vita-make-fself $< eboot.bin

%.velf: %.elf
	vita-elf-create $< $@

$(TARGET).elf: $(OBJS)
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

clean:
	@rm -rf *.velf *.elf *.vpk $(OBJS) param.sfo eboot.bin
