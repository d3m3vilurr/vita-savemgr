#ifndef __FILE_H__
#define __FILE_H__

int exists(const char *path);
int is_dir(const char *path);

int mkdir(const char *path, int mode);
int rmdir(const char *path, void (*callback)());
int file_count(char *path, int check_blacklist);
int copydir(const char *src, const char *dest, void (*callback)());

int copyfile(char *src, char *dest);

// below codes are part of vitashell
#define MAX_MOUNT_POINT_LENGTH 16

int pfs_mount(const char *path);
int pfs_unmount();
#endif
