#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>

#include "common.h"
#include "file.h"

void *memmem(const void *, size_t, const void *, size_t);

#define SCE_ERROR_ERRNO_ENOENT (int)(0x80010002)
#define SCE_ERROR_ERRNO_EEXIST (int)(0x80010011)
#define SCE_ERROR_ERRNO_ENODEV (int)(0x80010013)
#define SCE_ERROR_ERRNO_EINVAL (int)(0x80010016)

char *blacklists[] = {
    "sce_pfs/",
    "sce_sys/safemem.dat",
    "sce_sys/keystone",
    //"sce_sys/param.sfo",
    "sce_sys/sealedkey",
    NULL,
};

int exists(const char *path) {
    SceIoStat stat = {0};
    int ret = sceIoGetstat(path, &stat);
    return ret != SCE_ERROR_ERRNO_ENOENT && ret != SCE_ERROR_ERRNO_ENODEV;
}

int is_dir(const char *path) {
    SceIoStat stat = {0};
    if (sceIoGetstat(path, &stat) == SCE_ERROR_ERRNO_ENOENT) {
        return 0;
    }
    return SCE_S_ISDIR(stat.st_mode);
}

int mkdir(const char *path, int mode) {
    if (exists(path)) {
        if (is_dir(path)) {
            return 0;
        }
        return SCE_ERROR_ERRNO_EEXIST;
    }
    int len = strlen(path);
    char p[len];
    memset(p, 0, len);
    for (int i = 0; i < len; i++) {
        if (path[i] == '/') {
            if (strcmp(p, "ux0:/") == 0) {
                p[i] = path[i];
                continue;
            }
            if (!exists(p)) {
                sceIoMkdir(p, mode);
            } else {
                if (!is_dir(p)) {
                    return SCE_ERROR_ERRNO_EINVAL;
                }
            }
        }
        p[i] = path[i];
    }
    sceIoMkdir(path, mode);
    return 0;
}

int rmdir(const char *path, void (*callback)()) {
    SceUID dfd = sceIoDopen(path);
    if (dfd < 0) {
        int ret = sceIoRemove(path);
        if (ret < 0) {
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

            char *new_path = malloc(strlen(path) + strlen(dir.d_name) + 2);
            snprintf(new_path, 1024, "%s/%s", path, dir.d_name);

            if (SCE_S_ISDIR(dir.d_stat.st_mode)) {
                int ret = rmdir(new_path, callback);
                if (ret <= 0) {
                    free(new_path);
                    sceIoDclose(dfd);
                    return ret;
                }
            } else {
                int ret = sceIoRemove(new_path);
                if (callback) {
                    callback();
                }
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
    if (ret < 0) {
        return ret;
    }

    return 1;
}

int copyfile(char *src, char *dest) {
    // The source and destination paths are identical
    if (strcasecmp(src, dest) == 0) {
        return -1;
    }

    int i = 0;
    while (blacklists[i]) {
        // if (strcasecmp(dest, blacklists[i]) == 0) {
        if (strstr(dest, blacklists[i])) {
            return 2;
        }
        i += 1;
    }

    int ignore_error = strncmp(dest, "savedata0:", 10) == 0;

    SceUID fdsrc = sceIoOpen(src, SCE_O_RDONLY, 0);
    if (!ignore_error && fdsrc < 0) {
        return fdsrc;
    }

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

int file_count(char *path, int check_blacklist) {
    if (check_blacklist) {
        int i = 0;
        while (blacklists[i]) {
            if (strstr(path, blacklists[i])) {
                return 0;
            }
            i += 1;
        }
    }

    if (!exists(path)) {
        return 0;
    }

    SceUID dfd = sceIoDopen(path);
    int cnt = 0;
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
                cnt += file_count(new_path, check_blacklist);
            } else {
                cnt += 1;
            }

            free(new_path);
        }
    } while (res > 0);

    sceIoDclose(dfd);
    return cnt;
}

int copydir(const char *src, const char *dest, void (*callback)()) {
    if (strcasecmp(src, dest) == 0) {
        return -1;
    }

    int i = 0;
    while (blacklists[i]) {
        // if (strcasecmp(dest, blacklists[i]) == 0) {
        if (strstr(dest, blacklists[i])) {
            return 2;
        }
        i += 1;
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
                ret = copydir(new_src, new_dest, callback);
            } else {
                ret = copyfile(new_src, new_dest);
                if (callback) {
                    callback();
                }
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
