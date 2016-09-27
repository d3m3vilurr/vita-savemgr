#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <psp2/io/stat.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/dirent.h>

#include "file.h"

#define printf psvDebugScreenPrintf

#define SCE_ERROR_ERRNO_ENOENT 0x80010002
#define SCE_ERROR_ERRNO_EEXIST 0x80010011
#define SCE_ERROR_ERRNO_ENODEV 0x80010013

int exists(const char *path) {
    SceIoStat stat = {0};
    int ret = sceIoGetstat(path, &stat);
    return ret != SCE_ERROR_ERRNO_ENOENT &&
        ret != SCE_ERROR_ERRNO_ENOENT &&
        ret != SCE_ERROR_ERRNO_ENODEV;
}

int is_dir(const char *path) {
    SceIoStat stat = {0};
    if (sceIoGetstat(path, &stat) == SCE_ERROR_ERRNO_ENOENT) {
        return 0;
    }
    return SCE_S_ISDIR(stat.st_mode);
}

int rmdir(const char *path) {
    SceUID dfd = sceIoDopen(path);
    if (dfd < 0) {
        int ret = sceIoRemove(path);
        if (ret < 0)
            return ret;
    }
    int res = 0;

    do {
        SceIoDirent dir;
        memset(&dir, 0, sizeof(SceIoDirent));

        res = sceIoDread(dfd, &dir);
        if (res > 0) {
            if (strcmp(dir.d_name, ".") == 0 || strcmp(dir.d_name, "..") == 0)
                continue;

            char *new_path = malloc(strlen(path) + strlen(dir.d_name) + 2);
            snprintf(new_path, 1024, "%s/%s", path, dir.d_name);

            if (SCE_S_ISDIR(dir.d_stat.st_mode)) {
                int ret = rmdir(new_path);
                if (ret <= 0) {
                    free(new_path);
                    sceIoDclose(dfd);
                    return ret;
                }
            } else {
                int ret = sceIoRemove(new_path);
                if (ret < 0) {
                    free(new_path);
                    sceIoDclose(dfd);
                    return ret;
                }
            }

            free(new_path);
        }
    } while (res > 0);

    sceIoDclose(dfd);

    int ret = sceIoRmdir(path);
    if (ret < 0)
        return ret;

    return 1;
}

int mvdir(const char *src, const char *dest) {
    if (strcasecmp(src, dest) == 0) {
        return -1;
    }
    // TODO need we any guards like vitashell?
    return sceIoRename(src, dest);
}

int copyfile(char *src, char *dest) {
    // The source and destination paths are identical
    if (strcasecmp(src, dest) == 0) {
        return -1;
    }

    int ignore_error = strncmp(dest, "savedata0:", 10) == 0;

    SceUID fdsrc = sceIoOpen(src, SCE_O_RDONLY, 0);
    if (!ignore_error && fdsrc < 0)
        return fdsrc;

    int size = sceIoLseek(fdsrc, 0, SEEK_END);
    sceIoLseek(fdsrc, 0, SEEK_SET);
    void *buf = malloc(size);
    sceIoRead(fdsrc, buf, size);
    sceIoClose(fdsrc);

    SceUID fddst = sceIoOpen(dest, SCE_O_WRONLY | SCE_O_CREAT, 0777);
    if (!ignore_error && fddst < 0) {
        return fddst;
    }
    sceIoWrite(fddst, buf, size);
    sceIoClose(fddst);


    free(buf);
    return 1;
}

int copydir(const char *src, const char *dest) {
    if (strcasecmp(src, dest) == 0) {
        return -1;
    }

    SceUID dfd = sceIoDopen(src);

    if (!exists(dest)) {
        int ret = sceIoMkdir(dest, 0777);
        if (ret < 0 && ret != SCE_ERROR_ERRNO_EEXIST) {
            sceIoDclose(dfd);
            return ret;
        }
    }

    int res = 0;

    do {
        SceIoDirent dir;
        memset(&dir, 0, sizeof(SceIoDirent));

        res = sceIoDread(dfd, &dir);
        if (res > 0) {
            if (strcmp(dir.d_name, ".") == 0 || strcmp(dir.d_name, "..") == 0)
                continue;

            char *new_src = malloc(strlen(src) + strlen(dir.d_name) + 2);
            snprintf(new_src, 1024, "%s/%s", src, dir.d_name);

            char *new_dest = malloc(strlen(dest) + strlen(dir.d_name) + 2);
            snprintf(new_dest, 1024, "%s/%s", dest, dir.d_name);

            int ret = 0;

            if (SCE_S_ISDIR(dir.d_stat.st_mode)) {
                ret = copydir(new_src, new_dest);
            } else {
                ret = copyfile(new_src, new_dest);
            }

            free(new_dest);
            free(new_src);

            if (ret <= 0) {
                sceIoDclose(dfd);
                return ret;
            }
        }
    } while (res > 0);

    sceIoDclose(dfd);
    return 1;
}

int is_dumper_eboot(const char *path) {
    SceUID fd = sceIoOpen(path, SCE_O_RDONLY, 0777);
    if (fd < 0) {
        return 0;
    }
    int size = sceIoLseek(fd, 0, SEEK_END);
    sceIoLseek(fd, 0, SEEK_SET);
    char *eboot = malloc(size);
    sceIoRead(fd, eboot, size);
    sceIoClose(fd);

    int ret = memmem(eboot, size, "savemgr.elf", 11) != 0;
    free(eboot);
    return ret;
}

int is_encrypted_eboot(const char *path) {
    SceUID fd = sceIoOpen(path, SCE_O_RDONLY, 0777);
    if (fd < 0) {
        return 0;
    }
    char code[4] = {0};
    sceIoRead(fd, code, 4);
    sceIoClose(fd);

    return strncmp(code, "SCE", 4) != 0;
}
