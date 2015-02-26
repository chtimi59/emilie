#ifndef IOCTL_HEADER
#define IOCTL_HEADER

void linux_free_socket(int socket);
int  linux_create_socket();

int linux_set_iface_flags(int sock, const char *ifname, int dev_up);
int linux_iface_up(int sock, const char *ifname);

int linux_get_ifhwaddr(int sock, const char *ifname, u8 *addr);

int linux_br_get(char *brname, const char *ifname, int* ifindex);
#endif