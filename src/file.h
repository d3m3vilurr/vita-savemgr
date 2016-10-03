#ifndef __FILE_H__
#define __FILE_H__

int exists(const char *path);
int is_dir(const char *path);

int rmdir(const char *path);
int mvdir(const char *src, const char *dest);
int copydir(const char *src, const char *dest);

int copyfile(char *src, char *dest);

int is_dumper_eboot(const char *path);
int is_encrypted_eboot(const char *path);
#endif
