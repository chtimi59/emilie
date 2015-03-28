#include "common.h"
#include "eloop.h"
#include <net/if.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if_arp.h>



int linux_create_socket() {
	int sock = socket(PF_INET, SOCK_DGRAM, 0);
	return sock;
}

void linux_free_socket(int socket) {
	close(socket);
}

/* linux_get_if_ifidx
 * @param ifname interface name (like "wlan0")
 * @return return -1 if not exists otherwise return interface index
 */
int linux_get_if_ifidx(int sock, const char *ifname) {
    struct ifreq    ifr;
    strncpy(ifr.ifr_name, ifname, IF_NAMESIZE);
    if (-1==ioctl(sock, SIOCGIFINDEX, &ifr)) return -1;
    return ifr.ifr_ifindex;
}

/* linux_set_iface_flags
 * set interface up or down 
 * @param ifname interface name (like "wlan0")
 * @param dev_up boolean true to push it up
 * @return return 0 if succeed
 */
int linux_set_iface_flags(int sock, const char *ifname, int dev_up)
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

	if (dev_up) {
		if (ifr.ifr_flags & IFF_UP)
			return 0;
		ifr.ifr_flags |= IFF_UP;
	} else {
		if (!(ifr.ifr_flags & IFF_UP))
			return 0;
		ifr.ifr_flags &= ~IFF_UP;
	}

	if (ioctl(sock, SIOCSIFFLAGS, &ifr) != 0) {
		ret = errno ? -errno : -999;
		fprintf(stderr, "Could not set interface %s flags (%s): %s", ifname, dev_up ? "UP" : "DOWN", strerror(errno));
		return ret;
	}

	return 0;
}

/* linux_iface_up
* get interface status
* @param ifname interface name (like "wlan0")
* @return return 1 if UP, 0 if DOWN, <0 if error
*/
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
		fprintf(stderr, "Could not read interface %s flags: %s\n", ifname, strerror(errno));
		return ret;
	}

	return !!(ifr.ifr_flags & IFF_UP);
}

/* linux_get_ifhwaddr
* get interface mac address
* @param ifname interface name (like "wlan0")
* @param addr out mac adress
* @return return 0 if suceed
*/
int linux_get_ifhwaddr(int sock, const char *ifname, u8 *addr)
{
	struct ifreq ifr;

	memset(&ifr, 0, sizeof(ifr));
	strlcpy(ifr.ifr_name, ifname, IFNAMSIZ);
	if (ioctl(sock, SIOCGIFHWADDR, &ifr)) {
		fprintf(stderr, "Could not get interface %s hwaddr: %s", ifname, strerror(errno));
		return -1;
	}

	if (ifr.ifr_hwaddr.sa_family != ARPHRD_ETHER) {
		fprintf(stderr, "%s: Invalid HW-addr family 0x%04x", ifname, ifr.ifr_hwaddr.sa_family);
		return -1;
	}

	memcpy(addr, ifr.ifr_hwaddr.sa_data, ETH_ALEN);
	return 0;
}



/* linux_br_get
* get interface bridge
* @param ifname interface name (like "wlan0")
* @param brname out bridge interface name (like "eth0")
* @param brname out bridge index
* @return return 0 if suceed
*/
int linux_br_get(char *brname, const char *ifname, int* ifindex)
{
	char path[128], brlink[128], *pos;
	ssize_t res;

	snprintf(path, sizeof(path), "/sys/class/net/%s/brport/bridge", ifname);
	res = readlink(path, brlink, sizeof(brlink));
	if (res < 0 || (size_t)res >= sizeof(brlink))
		return -1;
	brlink[res] = '\0';
	pos = strrchr(brlink, '/');
	if (pos == NULL)
		return -1;
	pos++;

	if (brname)  strlcpy(brname, pos, IFNAMSIZ);
	if (ifindex) *ifindex = if_nametoindex(ifname);
	return 0;
}
