#include "user/syscall.h"
#include "user/errno.h"
#include "user/time.h"
#include "arch/amd64/cpu.h"
#include "sys/thread.h"
#include "sys/debug.h"

#include "sys/sys_file.h"
#include "sys/sys_sys.h"
#include "sys/sys_proc.h"
#include "sys/sys_net.h"

#define MSR_IA32_STAR               0xC0000081
#define MSR_IA32_LSTAR              0xC0000082
#define MSR_IA32_SFMASK             0xC0000084

extern void syscall_entry(void);

void *syscall_table[256] = {
    // I/O
    [SYSCALL_NR_READ] = sys_read,
    [SYSCALL_NR_WRITE] = sys_write,
    [SYSCALL_NR_OPEN] = sys_open,
    [SYSCALL_NR_CLOSE] = sys_close,
    [SYSCALL_NR_STAT] = sys_stat,
    [SYSCALL_NR_LSEEK] = sys_lseek,
    [SYSCALL_NR_IOCTL] = sys_ioctl,
    [SYSCALL_NR_ACCESS] = sys_access,
    [SYSCALL_NR_SELECT] = sys_select,
    [SYSCALL_NR_GETCWD] = sys_getcwd,
    [SYSCALL_NR_CHDIR] = sys_chdir,
    [SYSCALL_NR_MKDIR] = sys_mkdir,
    [SYSCALL_NR_RMDIR] = sys_rmdir,
    [SYSCALL_NR_CREAT] = sys_creat,
    [SYSCALL_NR_UNLINK] = sys_unlink,
    [SYSCALL_NR_READDIR] = sys_readdir,
    [SYSCALL_NR_CHMOD] = sys_chmod,
    [SYSCALL_NR_CHOWN] = sys_chown,

    // User control
    [SYSCALL_NR_GETUID] = sys_getuid,
    [SYSCALL_NR_GETGID] = sys_getgid,
    [SYSCALL_NR_SETUID] = sys_setuid,
    [SYSCALL_NR_SETGID] = sys_setgid,
    // Process control
    [SYSCALL_NR_BRK] = sys_brk,
    [SYSCALL_NRX_SIGENTRY] = sys_sigentry,
    [SYSCALL_NR_EXECVE] = sys_execve,
    [SYSCALL_NR_GETPID] = sys_getpid,
    [SYSCALL_NR_KILL] = sys_kill,
    [SYSCALL_NR_EXIT] = sys_exit,
    [SYSCALL_NR_SIGRETURN] = sys_sigreturn,
    [SYSCALL_NRX_WAITPID] = sys_waitpid,
    [SYSCALL_NR_GETPGID] = sys_getpgid,
    [SYSCALL_NR_SETPGID] = sys_setpgid,

    // System
    [SYSCALL_NR_SYNC] = sys_sync,
    [SYSCALL_NR_MOUNT] = sys_mount,
    [SYSCALL_NRX_UMOUNT] = sys_umount,
    [SYSCALL_NR_UNAME] = sys_uname,
    [SYSCALL_NR_NANOSLEEP] = sys_nanosleep,
    [SYSCALL_NR_GETTIMEOFDAY] = sys_gettimeofday,
    [SYSCALL_NR_REBOOT] = sys_reboot,

    // Network
    [SYSCALL_NR_SOCKET] = sys_socket,
    [SYSCALL_NR_SENDTO] = sys_sendto,
    [SYSCALL_NR_RECVFROM] = sys_recvfrom,
    [SYSCALL_NR_BIND] = sys_bind,
    [SYSCALL_NR_SETSOCKOPT] = sys_setsockopt,
    [SYSCALL_NRX_NETCTL] = sys_netctl,
};

int syscall_undefined(uint64_t rax) {
    kwarn("Undefined syscall: %d\n", rax);
    return -ENOSYS;
}

void syscall_init(void) {
    // LSTAR = syscall_entry
    wrmsr(MSR_IA32_LSTAR, (uintptr_t) syscall_entry);

    // SFMASK = (1 << 9) /* IF */
    wrmsr(MSR_IA32_SFMASK, (1 << 9));

    // STAR = ((ss3 - 8) << 48) | (cs0 << 32)
    wrmsr(MSR_IA32_STAR, ((uint64_t) (0x1B - 8) << 48) | ((uint64_t) 0x08 << 32));

    // Set SCE bit
    uint64_t efer = rdmsr(MSR_IA32_EFER);
    efer |= (1 << 0);
    wrmsr(MSR_IA32_EFER, efer);
}
