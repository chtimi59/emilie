#include "common.h"
#include "eloop.h"
#include <net/if.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>



int linux_create_socket() {
	int sock = socket(PF_INET, SOCK_DGRAM, 0);
	return sock;
}

void linux_free_socket(int socket) {
	close(socket);
}

int linux_iface_up(int sock, const char *ifname)
{
	struct ifreq ifr;
	int ret;

	if (sock < 0)
		return -1;

	memset(&ifr, 0, sizeof(ifr));
	strlcpy(ifr.ifr_name, ifname, IFNAMSIZ);

	if (ioctl(sock, SIOCGIFFLAGS, &ifr) != 0) {
		ret = errno ? -errno : -999;
		fprintf(stderr, "Could not read interface %s flags: %s", ifname, strerror(errno));
		return ret;
	}

	return !!(ifr.ifr_flags & IFF_UP);
}
