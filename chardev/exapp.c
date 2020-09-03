#include <fcntl.h>         /* For open and flags */
#include <stdio.h>         /* For printf and perror */
#include <string.h>        /* For strlen */
#include <sys/ioctl.h>     /* For ioctl syscall */
#include <unistd.h>        /* For read write close */

#include "exchardev.h"     /* For device specific data */

int main(void)
{
   const char *wrmsg = "Hello from userspace";
   char rdmsg[100] = {0};
   int fd;
   color_t color;
   int ret;

   /* Open device */
   fd = open("/dev/"EXCHARDEV"0", O_RDWR);
   if (fd < 0) {
      perror("Failed to open device");
      return -1;
   }

   /* Read from device */
   ret = read(fd, rdmsg, sizeof(rdmsg));
   if (ret < 0) {
      perror("Failed to read from device");
      return -2;
   } else {
      printf("Read the next message from kernel: %s\n", rdmsg);
   }

   /* Send ioctl */
   ret = ioctl(fd, EXCHARDEV_SET_COLOR, COLOR_GREEN);
   if (ret) {
      perror("Failed to send ioctl");
      return -3;
   }

   /* Send ioctl */
   ret = ioctl(fd, EXCHARDEV_GET_COLOR, &color);
   if (ret) {
      perror("Failed to send ioctl");
      return -4;
   } else {
      printf("Kernel has the next color: %d\n", color);
   }

   /* Send ioctl */
   ret = ioctl(fd, EXCHARDEV_SET_COLOR, COLOR_RED);
   if (ret) {
      perror("Failed to send ioctl");
      return -5;
   }   

   /* Write to device */
   ret = write(fd, wrmsg, strlen(wrmsg));
   if (ret < 0) {
      perror("Failed to write to device");
      return -6;
   }

   /* Send ioctl */
   ret = ioctl(fd, EXCHARDEV_SET_COLOR, color);
   if (ret) {
      perror("Failed to send ioctl");
      return -7;
   }   

   /* Write to device */
   ret = write(fd, wrmsg, strlen(wrmsg));
   if (ret < 0) {
      perror("Failed to write to device");
      return -8;
   }

   close(fd);

   return 0;
}
