/*
 * This File is Part Of :
 *      ___                       ___           ___           ___           ___           ___
 *     /  /\        ___          /__/\         /  /\         /__/\         /  /\         /  /\          ___
 *    /  /::\      /  /\         \  \:\       /  /:/         \  \:\       /  /:/_       /  /::\        /  /\
 *   /  /:/\:\    /  /:/          \  \:\     /  /:/           \__\:\     /  /:/ /\     /  /:/\:\      /  /:/
 *  /  /:/~/:/   /__/::\      _____\__\:\   /  /:/  ___   ___ /  /::\   /  /:/ /:/_   /  /:/~/::\    /  /:/
 * /__/:/ /:/___ \__\/\:\__  /__/::::::::\ /__/:/  /  /\ /__/\  /:/\:\ /__/:/ /:/ /\ /__/:/ /:/\:\  /  /::\
 * \  \:\/:::::/    \  \:\/\ \  \:\~~\~~\/ \  \:\ /  /:/ \  \:\/:/__\/ \  \:\/:/ /:/ \  \:\/:/__\/ /__/:/\:\
 *  \  \::/~~~~      \__\::/  \  \:\  ~~~   \  \:\  /:/   \  \::/       \  \::/ /:/   \  \::/      \__\/  \:\
 *   \  \:\          /__/:/    \  \:\        \  \:\/:/     \  \:\        \  \:\/:/     \  \:\           \  \:\
 *    \  \:\         \__\/      \  \:\        \  \::/       \  \:\        \  \::/       \  \:\           \__\/
 *     \__\/                     \__\/         \__\/         \__\/         \__\/         \__\/
 *
 * Copyright (c) Rinnegatamante <rinnegatamante@gmail.com>
 *
 */
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <psp2/apputil.h>
#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>

// Dump/Restore decrypted savedata
void dumpSavedataDir(char* folder, char* target){
    sceIoMkdir(target, 0777);
    SceIoDirent dirent;
    SceUID fd = sceIoDopen(folder);
    while (sceIoDread(fd, &dirent) > 0) {
        if SCE_S_ISDIR(dirent.d_stat.st_mode){
            char path[256], target_p[256];
            sprintf(path,"%s/%s",folder,dirent.d_name);
            sprintf(target_p,"%s/%s",target,dirent.d_name);
            dumpSavedataDir(path, target_p);
        }else{
            char path[256], target_p[256];
            sprintf(path,"%s/%s",folder,dirent.d_name);
            sprintf(target_p,"%s/%s",target,dirent.d_name);
            int fd = sceIoOpen(path, SCE_O_RDONLY, 0777);
            int size = sceIoLseek(fd, 0, SEEK_END);
            sceIoLseek(fd, 0, SEEK_SET);
            char* data = (char*)malloc(size);
            sceIoRead(fd, data, size);
            sceIoClose(fd);
            fd = sceIoOpen(target_p, SCE_O_WRONLY | SCE_O_CREAT, 0777);
            sceIoWrite(fd, data, size);
            sceIoClose(fd);
            free(data);
        }
    }
}

void restoreSavedataDir(char* folder, char* target){
    SceUID fd = sceIoDopen(folder);
    SceIoDirent dirent;
    while (sceIoDread(fd, &dirent) > 0) {
        if SCE_S_ISDIR(dirent.d_stat.st_mode){
            char path[256], target_p[256];
            sprintf(path,"%s/%s",folder,dirent.d_name);
            if (target != NULL) sprintf(target_p,"%s/%s",target,dirent.d_name);
            else sprintf(target_p,"savedata0:%s",dirent.d_name);
            restoreSavedataDir(path, target_p);
        }else{
            char path[256], target_p[256];
            sprintf(path,"%s/%s",folder,dirent.d_name);
            if (target != NULL) sprintf(target_p,"%s/%s",target,dirent.d_name);
            else sprintf(target_p,"savedata0:%s",dirent.d_name);
            int fd = sceIoOpen(path, SCE_O_RDONLY, 0777);
            int size = sceIoLseek(fd, 0, SEEK_END);
            sceIoLseek(fd, 0, SEEK_SET);
            char* data = (char*)malloc(size);
            sceIoRead(fd, data, size);
            sceIoClose(fd);
            fd = sceIoOpen(target_p, SCE_O_WRONLY | SCE_O_CREAT, 0777);
            sceIoWrite(fd, data, size);
            sceIoClose(fd);
            free(data);
        }
    }
}
