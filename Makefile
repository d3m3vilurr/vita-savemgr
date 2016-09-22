TITLE_ID = SAVEMGR00
TITLE = Save Manager
VERSION = 0.6.0
APP_VER = $(shell python -c "print '%02d.%02d' % tuple(map(lambda x: int(x), '$(VERSION)'.split('.')[:2]))")

ifeq ($(RELEASE),)
	VERSION := $(VERSION)-$(shell git describe --abbrev=7 --dirty --always)
endif

TARGET = savemgr
OBJS   = main.o file.o appdb.o vita_sqlite.o sqlite3/sqlite3.o

LIBS = -lSceKernel_stub -lSceDisplay_stub -lSceGxm_stub \
	   -lSceAppMgr_stub -lSceAppUtil_stub -lSceSysmodule_stub \
	   -lSceCtrl_stub -lScePgf_stub -lz -lm \
	   -lvita2d -lfreetype

PREFIX  = arm-vita-eabi
CC      = $(PREFIX)-gcc
CFLAGS  = -Wl,-q -Wall -O3 -std=c99 \
		  -DSQLITE_OS_OTHER=1 -DSQLITE_TEMP_STORE=3 -DSQLITE_THREADSAFE=0 \
		  -DVERSION=\"$(VERSION)\"
ASFLAGS = $(CFLAGS)

all: $(TARGET).vpk

%.vpk: eboot.bin
	vita-mksfoex -s TITLE_ID=$(TITLE_ID) -s APP_VER=$(APP_VER) "$(TITLE)" param.sfo
	vita-pack-vpk -s param.sfo -b eboot.bin \
				  --add sce_sys/icon0.png=sce_sys/icon0.png \
				  --add sce_sys/livearea/contents/bg.png=sce_sys/livearea/contents/bg.png \
				  --add sce_sys/livearea/contents/startup.png=sce_sys/livearea/contents/startup.png \
				  --add sce_sys/livearea/contents/template.xml=sce_sys/livearea/contents/template.xml \
				  $@

eboot.bin: $(TARGET).velf
	vita-make-fself $< eboot.bin

%.velf: %.elf
	vita-elf-create $< $@

$(TARGET).elf: $(OBJS)
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

clean:
	@rm -rf *.velf *.elf *.vpk $(OBJS) param.sfo eboot.bin
