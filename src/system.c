#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <psp2/appmgr.h>
#include <psp2/registrymgr.h>

#include <vitashell_user.h>

#include "system.h"
#include "util.h"

#define SFO_MAGIC 0x46535000 // \x00PSF
#define MAX_MOUNT_POINT_LENGTH 16

struct sfo_header {
    uint32_t magic;
    uint32_t version;
    uint32_t key_table_offset;
    uint32_t data_table_offset;
    uint32_t entries;
};

struct sfo_index {
    uint16_t key_offset;
    uint16_t param_format;
    uint32_t param_length;
    uint32_t param_max_length;
    uint32_t data_offset;
};

uint8_t g_aid_loaded = 0;
uint64_t g_aid;

char pfs_mount_point[MAX_MOUNT_POINT_LENGTH];
int known_pfs_ids[] = {
    0x6E,
    0x12E,
    0x12F,
    0x3ED,
};

// below codes use part of vitashell codeset
uint64_t get_accountid() {
    if (g_aid_loaded) {
        return g_aid;
    }
    if (sceRegMgrGetKeyBin(
            "/CONFIG/NP", "account_id", &g_aid, sizeof(uint64_t)) < 0) {
        return 0;
    }
    g_aid_loaded = 1;
    return g_aid;
}

int8_t change_accountid(const char *sfo_path, const uint64_t aid) {
    FILE *f = fopen(sfo_path, "r+b");

    struct sfo_header hdr = {0};
    fread(&hdr, sizeof(struct sfo_header), 1, f);
    if (hdr.magic != SFO_MAGIC) {
        printf("magic mismatch\n");
        fclose(f);
        return -1;
    }
    uint32_t data_offset = -1;
    for (int i = 0; i < hdr.entries; i++) {
        fseek(f,
              sizeof(struct sfo_header) + sizeof(struct sfo_index) * i,
              SEEK_SET);
        struct sfo_index idx = {0};
        fread(&idx, sizeof(struct sfo_index), 1, f);
        char key[64];
        fseek(f, hdr.key_table_offset + idx.key_offset, SEEK_SET);
        fread(&key, sizeof(char), 64, f);
        if (strncmp(key, "ACCOUNT_ID", 10) != 0) {
            continue;
        }
        data_offset = hdr.data_table_offset + idx.data_offset;
        break;
    }
    if (data_offset == -1) {
        printf("not exist ACCOUNT_ID\n");
        fclose(f);
        return -2;
    }

    uint64_t old_aid;
    fseek(f, data_offset, SEEK_SET);
    fread(&old_aid, sizeof(uint64_t), 1, f);
    if (old_aid == aid) {
        fclose(f);
        printf("aid is already same\n");
        return 1;
    }

    printf("replace ACCOUNT_ID - %08x to %08x\n", old_aid, aid);
    fseek(f, data_offset, SEEK_SET);
    fwrite(&aid, sizeof(uint64_t), 1, f);
    fclose(f);
    return 0;
}

int pfs_mount(const char *path) {
    char klicensee[0x10];
    // char license_buf[0x200];
    ShellMountIdArgs args;

    memset(klicensee, 0, sizeof(klicensee));

    args.process_titleid = "SAVEMGR00";
    args.path = path;
    args.desired_mount_point = NULL;
    args.klicensee = klicensee;
    args.mount_point = pfs_mount_point;

    int i;
    for (i = 0; i < sizeof(known_pfs_ids) / sizeof(int); i++) {
        args.id = known_pfs_ids[i];

        int res = shellUserMountById(&args);
        if (res >= 0) {
            return res;
        }
    }

    return sceAppMgrGameDataMount(path, 0, 0, pfs_mount_point);
}

int pfs_unmount() {
    if (pfs_mount_point[0] == 0) {
        return -1;
    }

    int res = sceAppMgrUmount(pfs_mount_point);
    if (res >= 0) {
        memset(pfs_mount_point, 0, sizeof(pfs_mount_point));
        // memset(pfs_mounted_path, 0, sizeof(pfs_mounted_path));
    }

    return res;
}
