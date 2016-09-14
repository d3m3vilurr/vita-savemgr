// This was only tested with sqlite 3.6.23.1
// Please note that this .c file does not implement thread safety for sqlite, and as such requires it to be built with -DSQLITE_THREADSAFE=0 as well
// Build flags for sqlite: -DSQLITE_OS_OTHER=1 -DSQLITE_TEMP_STORE=3 -DSQLITE_THREADSAFE=0

// It's also hacky -- no sync support, no tempdir support, access returning bs, etc... don't use in production!

// based on test_demovfs.c

#include "sqlite3.h"

#include <stdio.h>
#include <string.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/rtc.h>

#define VERBOSE 0
#if VERBOSE
#define LOG psvDebugScreenPrintf
#else
#define LOG(...)
#endif

typedef struct VitaFile {
	sqlite3_file base;
	unsigned fd;
} VitaFile;

// File ops
static int vita_xClose(sqlite3_file *pFile) {
	VitaFile *p = (VitaFile*)pFile;
	sceIoClose(p->fd);
	LOG("close %x\n", p->fd);
	return SQLITE_OK;
}

static int vita_xRead(sqlite3_file *pFile, void *zBuf, int iAmt, sqlite_int64 iOfst) {
	VitaFile *p = (VitaFile*)pFile;
	memset(zBuf, 0, iAmt);
	sceIoLseek(p->fd, iOfst, SCE_SEEK_SET);
	int read = sceIoRead(p->fd, zBuf, iAmt);
	LOG("read %x %x %x => %x\n", p->fd, zBuf, iAmt, read);
	if (read == iAmt)
		return SQLITE_OK;
	else if (read >= 0)
		return SQLITE_IOERR_SHORT_READ;
	return SQLITE_IOERR_READ;
}

static int vita_xWrite(sqlite3_file *pFile, const void *zBuf, int iAmt, sqlite_int64 iOfst) {
	VitaFile *p = (VitaFile*)pFile;
	int ofst = sceIoLseek(p->fd, iOfst, SCE_SEEK_SET);
	LOG("seek %x %x => %x\n", p->fd, iOfst, ofst);
	if (ofst != iOfst)
		return SQLITE_IOERR_WRITE;
	int write = sceIoWrite(p->fd, zBuf, iAmt);
	LOG("write %x %x %x => %x\n", p->fd, zBuf, iAmt);
	if (write != iAmt)
		return SQLITE_IOERR_WRITE;
	return SQLITE_OK;
}

static int vita_xTruncate(sqlite3_file *pFile, sqlite_int64 size) {
	LOG("truncate\n");
	return SQLITE_OK;
}

static int vita_xSync(sqlite3_file *pFile, int flags) {
	return SQLITE_OK;
}

static int vita_xFileSize(sqlite3_file *pFile, sqlite_int64 *pSize) {
	VitaFile *p = (VitaFile*)pFile;
	SceIoStat stat = {0};
	sceIoGetstatByFd(p->fd, &stat);
	LOG("filesize %x => %x\n", p->fd, stat.st_size);
	*pSize = stat.st_size;
	return SQLITE_OK;
}

static int vita_xLock(sqlite3_file *pFile, int eLock) {
	return SQLITE_OK;
}

static int vita_xUnlock(sqlite3_file *pFile, int eLock) {
	return SQLITE_OK;
}

static int vita_xCheckReservedLock(sqlite3_file *pFile, int *pResOut) {
	*pResOut = 0;
	return SQLITE_OK;
}

static int vita_xFileControl(sqlite3_file *pFile, int op, void *pArg) {
	return SQLITE_OK;
}

static int vita_xSectorSize(sqlite3_file *pFile) {
	return 0;
}

static int vita_xDeviceCharacteristics(sqlite3_file *pFile) {
	return 0;
}

// VFS ops
static int vita_xOpen(sqlite3_vfs *vfs, const char *name, sqlite3_file *file, int flags, int *outFlags) {
	static const sqlite3_io_methods vitaio = {
		1,
		vita_xClose,
		vita_xRead,
		vita_xWrite,
		vita_xTruncate,
		vita_xSync,
		vita_xFileSize,
		vita_xLock,
		vita_xUnlock,
		vita_xCheckReservedLock,
		vita_xFileControl,
		vita_xSectorSize,
		vita_xDeviceCharacteristics,
	};

	VitaFile *p = (VitaFile*)file;
	unsigned oflags = 0;
	if (flags & SQLITE_OPEN_EXCLUSIVE)
		oflags |= SCE_O_EXCL;
	if (flags & SQLITE_OPEN_CREATE)
		oflags |= SCE_O_CREAT;
	if (flags & SQLITE_OPEN_READONLY)
		oflags |= SCE_O_RDONLY;
	if (flags & SQLITE_OPEN_READWRITE)
		oflags |= SCE_O_RDWR;
	// TODO(xyz): sqlite tries to open inexistant journal and then tries to read from it, wtf?
	// so force O_CREAT here
	if (flags & SQLITE_OPEN_MAIN_JOURNAL && !(flags & SQLITE_OPEN_EXCLUSIVE))
		oflags |= SCE_O_CREAT;
	memset(p, 0, sizeof(*p));
	p->fd = sceIoOpen(name, oflags, 7);
	LOG("open %s %x orig flags %x => %x\n", name, oflags, flags, p->fd);
	if (p->fd < 0) {
		return SQLITE_CANTOPEN;
	}
	if (outFlags)
		*outFlags = flags;

	p->base.pMethods = &vitaio;

	return SQLITE_OK;
}

int vita_xDelete(sqlite3_vfs *vfs, const char *name, int syncDir) {
	int ret = sceIoRemove(name);
	if (ret < 0)
		return SQLITE_IOERR_DELETE;
	return SQLITE_OK;
}

int vita_xAccess(sqlite3_vfs *vfs, const char *name, int flags, int *pResOut) {
	*pResOut = 1;
	return SQLITE_OK;
}

int vita_xFullPathname(sqlite3_vfs *vfs, const char *zName, int nOut, char *zOut) {
	snprintf(zOut, nOut, "%s", zName);
	return 0;
}

void* vita_xDlOpen(sqlite3_vfs *vfs, const char *zFilename) {
	return NULL;
}

void vita_xDlError(sqlite3_vfs *vfs, int nByte, char *zErrMsg) {
}

void (*vita_xDlSym(sqlite3_vfs *vfs,void*p, const char *zSymbol))(void) {
	return NULL;
}

void vita_xDlClose(sqlite3_vfs *vfs, void*p) {
}

int vita_xRandomness(sqlite3_vfs *vfs, int nByte, char *zOut) {
	return SQLITE_OK;
}

int vita_xSleep(sqlite3_vfs *vfs, int microseconds) {
	sceKernelDelayThread(microseconds);
	return SQLITE_OK;
}

int vita_xCurrentTime(sqlite3_vfs *vfs, double *pTime) {
	time_t t = 0;
	SceDateTime time = {0};
	sceRtcGetCurrentClock(&time, 0);
	sceRtcGetTime_t(&time, &t);
	*pTime = t/86400.0 + 2440587.5; 
	return SQLITE_OK;
}

int vita_xGetLastError(sqlite3_vfs *vfs, int e, char *err) {
	return 0;
}

sqlite3_vfs vita_vfs = {
	.iVersion = 1,
	.szOsFile = sizeof(VitaFile),
	.mxPathname = 0x100,
	.pNext = NULL,
	.zName = "psp2",
	.pAppData = NULL,
	.xOpen = vita_xOpen,
	.xDelete = vita_xDelete,
	.xAccess = vita_xAccess,
	.xFullPathname = vita_xFullPathname,
	.xDlOpen = vita_xDlOpen,
	.xDlError = vita_xDlError,
	.xDlSym = vita_xDlSym,
	.xDlClose = vita_xDlClose,
	.xRandomness = vita_xRandomness,
	.xSleep = vita_xSleep,
	.xCurrentTime = vita_xCurrentTime,
	.xGetLastError = vita_xGetLastError,
};

int sqlite3_os_init(void) {
	sqlite3_vfs_register(&vita_vfs, 1);
	return 0;
}

int sqlite3_os_end(void) {
	return 0;
}