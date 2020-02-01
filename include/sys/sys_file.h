#pragma once
#include "sys/types.h"

ssize_t sys_read(int fd, void *data, size_t lim);
ssize_t sys_write(int fd, const void *data, size_t lim);
int sys_creat(const char *pathname, int mode);
int sys_mkdir(const char *pathname, int mode);
int sys_unlink(const char *pathname);
int sys_rmdir(const char *pathname);
int sys_chdir(const char *filename);
int sys_ioctl(int fd, unsigned int cmd, void *arg);
// Kinda incompatible with linux, but who cares as long as it's
// POSIX on the libc side
int sys_getcwd(char *buf, size_t lim);
int sys_open(const char *filename, int flags, int mode);
void sys_close(int fd);
int sys_stat(const char *filename, struct stat *st);
int sys_access(const char *path, int mode);
int sys_chmod(const char *path, mode_t mode);
int sys_chown(const char *path, uid_t uid, gid_t gid);
off_t sys_lseek(int fd, off_t offset, int whence);