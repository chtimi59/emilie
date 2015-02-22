#ifndef IOCTL_HEADER
#define IOCTL_HEADER

void linux_free_socket(int socket);
int  linux_create_socket();

int linux_iface_up(int sock, const char *ifname);

#endif