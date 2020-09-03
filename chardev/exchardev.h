#ifndef __EXCHARDEV_H__
#define __EXCHARDEV_H__

#include <linux/ioctl.h>

#define EXCHARDEV       "exchardev"
#define EXCHARDEV_CLASS "exchar"
#define EXCHARDEV_MAJOR 124

typedef enum {
    COLOR_DEF,
    COLOR_RED,
    COLOR_GREEN,
    LAST_COLOR,
} color_t;

#define EXCHARDEV_SET_COLOR _IOR(EXCHARDEV_MAJOR, 0, color_t)
#define EXCHARDEV_GET_COLOR _IOW(EXCHARDEV_MAJOR, 1, color_t *)

#endif /* ifndef __EXCHARDEV_H__ */
