#ifndef __SYSTEM_H__
#define __SYSTEM_H__

// below codes use part of vitashell codeset
uint64_t get_accountid();
int8_t change_accountid(const char *sfo_path, const uint64_t aid);

int pfs_mount(const char *path);
int pfs_unmount();
#endif
