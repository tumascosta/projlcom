#include <stdio.h>
#include <minix/syslib.h>
#include <minix/drivers.h>

int main(void) {
    sef_startup();
    freopen("/dev/console", "w", stdout);

    int ret = sys_outb(0x70, 0x0A);
    printf("sys_outb returned: %d\n", ret);

    return 0;
}

