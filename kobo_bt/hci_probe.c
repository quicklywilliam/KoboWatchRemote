/* Minimal HCI probe for Kobo - uses only basic syscalls */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/time.h>

static int read_response(int fd, unsigned char *buf, int bufsize, int timeout_sec) {
    fd_set rfds;
    struct timeval tv;
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);
    tv.tv_sec = timeout_sec;
    tv.tv_usec = 0;
    int ret = select(fd + 1, &rfds, NULL, NULL, &tv);
    if (ret > 0) {
        return read(fd, buf, bufsize);
    }
    return ret; /* 0 = timeout, -1 = error */
}

static void hexdump(const unsigned char *buf, int len) {
    int i;
    for (i = 0; i < len; i++)
        printf("%02x ", buf[i]);
    printf("\n");
}

int main(void) {
    int fd, n;
    unsigned char buf[256];

    fd = open("/dev/stpbt", O_RDWR | O_NOCTTY);
    if (fd < 0) {
        perror("open");
        return 1;
    }
    printf("Opened /dev/stpbt fd=%d\n", fd);

    /* HCI Reset: opcode 0x0c03, no params */
    unsigned char cmd1[] = {0x01, 0x03, 0x0c, 0x00};
    n = write(fd, cmd1, sizeof(cmd1));
    printf("HCI_Reset: wrote %d bytes\n", n);

    n = read_response(fd, buf, sizeof(buf), 3);
    if (n > 0) {
        printf("Response (%d bytes): ", n);
        hexdump(buf, n);
    } else {
        printf("No response (ret=%d, errno=%d)\n", n, errno);
    }

    /* HCI Read Local Version: opcode 0x1001, no params */
    unsigned char cmd2[] = {0x01, 0x01, 0x10, 0x00};
    n = write(fd, cmd2, sizeof(cmd2));
    printf("\nHCI_Read_Local_Version: wrote %d bytes\n", n);

    n = read_response(fd, buf, sizeof(buf), 3);
    if (n > 0) {
        printf("Response (%d bytes): ", n);
        hexdump(buf, n);
    } else {
        printf("No response (ret=%d, errno=%d)\n", n, errno);
    }

    /* HCI LE Read Supported Features: opcode 0x2003 */
    unsigned char cmd3[] = {0x01, 0x03, 0x20, 0x00};
    n = write(fd, cmd3, sizeof(cmd3));
    printf("\nHCI_LE_Read_Local_Supported_Features: wrote %d bytes\n", n);

    n = read_response(fd, buf, sizeof(buf), 3);
    if (n > 0) {
        printf("Response (%d bytes): ", n);
        hexdump(buf, n);
    } else {
        printf("No response (ret=%d, errno=%d)\n", n, errno);
    }

    close(fd);
    return 0;
}
