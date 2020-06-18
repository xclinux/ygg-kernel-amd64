#include "arch/amd64/hw/rs232.h"
#include "arch/amd64/hw/con.h"
#include "arch/amd64/hw/ps2.h"
#include "user/termios.h"
#include "user/signum.h"
#include "sys/char/input.h"
#include "user/errno.h"
#include "sys/char/line.h"
#include "sys/char/ring.h"
#include "arch/amd64/cpu.h"
#include "sys/char/tty.h"
#include "sys/char/chr.h"
#include "sys/string.h"
#include "sys/thread.h"
#include "sys/assert.h"
#include "sys/ctype.h"
#include "sys/debug.h"
#include "sys/sched.h"
#include "sys/panic.h"
#include "sys/dev.h"
#include "sys/mm.h"

static ssize_t tty_write(struct chrdev *tty, const void *buf, size_t pos, size_t lim);
static int tty_ioctl(struct chrdev *tty, unsigned int cmd, void *arg);

// Keyboard + display
static struct tty_data _dev_tty0_data = {
    .fg_pgid = 1,
    .out_dev = NULL,
    .out_putc = NULL, //pc_con_putc
};

static struct chrdev _dev_tty0 = {
    .type = CHRDEV_TTY,
    .dev_data = &_dev_tty0_data,
    .tc = TERMIOS_DEFAULT,
    .write = tty_write,
    // Line discipline
    .read = line_read,
    .ioctl = tty_ioctl
};

void tty_control_write(struct chrdev *tty, char c) {
    struct tty_data *data = tty->dev_data;
    _assert(data);

    switch (c) {
    case 'd':
        ring_signal(&tty->buffer, RING_SIGNAL_EOF);
        break;
    case '.':
        //ring_signal(&tty->buffer, RING_SIGNAL_BRK);
        thread_signal_pgid(data->fg_pgid, SIGUSR1);
        break;
    case 'c':
        //ring_signal(&tty->buffer, RING_SIGNAL_BRK);
        thread_signal_pgid(data->fg_pgid, SIGINT);
        break;
    default:
        panic("Unhandled control to TTY: ^%c\n", c);
    }
}

void tty_data_write(struct chrdev *tty, char c) {
    _assert(tty && tty->type == CHRDEV_TTY);
    ring_putc(NULL, &tty->buffer, c, 0);

    switch (c) {
    case '\n':
        // TODO: this should also check ICANON
        if (tty->tc.c_lflag & ECHONL) {
            tty_putc(tty, c);
        }
        if (tty->tc.c_iflag & ICANON) {
            // Trigger line flush
            ring_signal(&tty->buffer, RING_SIGNAL_RET);
        }
        break;
    case '\b':
        ring_signal(&tty->buffer, 0);
        break;
    case '\033':
        if ((tty->tc.c_lflag & ECHO) || (tty->tc.c_iflag & ICANON)) {
            tty_puts(tty, "^[");
        }
        break;
    default:
        // This ignores ICANON
        if (tty->tc.c_lflag & ECHO) {
            tty_putc(tty, c);
        }
    }
}

void tty_puts(struct chrdev *tty, const char *s) {
    for (; *s; ++s) {
        tty_putc(tty, *s);
    }
}

void tty_putc(struct chrdev *tty, char c) {
    struct tty_data *data = tty->dev_data;
    _assert(data);
    //_assert(data->out_putc);
    //data->out_putc(data->out_dev, c);
}

void tty_init(void) {
    // tty0
    ring_init(&_dev_tty0.buffer, 16);
    g_keyboard_tty = &_dev_tty0;
    dev_add(DEV_CLASS_CHAR, DEV_CHAR_TTY, &_dev_tty0, "tty0");

    // ttyS0
    //ring_init(&_dev_ttyS0.buffer, 16);
    //rs232_set_tty(RS232_COM1, &_dev_ttyS0);
    //dev_add(DEV_CLASS_CHAR, DEV_CHAR_TTY, &_dev_ttyS0, "ttyS0");
}

static ssize_t tty_write(struct chrdev *tty, const void *buf, size_t pos, size_t lim) {
    return lim;
}

static int tty_ioctl(struct chrdev *tty, unsigned int cmd, void *arg) {
    return -EINVAL;
}
