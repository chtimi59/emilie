#ifndef IOCTL_HEADER
#define IOCTL_HEADER

void linux_free_socket(int socket);
int  linux_create_socket();

int linux_set_iface_flags(int sock, const char *ifname, int dev_up);
int linux_iface_up(int sock, const char *ifname);

#endif