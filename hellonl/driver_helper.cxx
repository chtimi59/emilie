

#ifndef CONFIG_LIBNL20
/*
* libnl 1.1 has a bug, it tries to allocate socket numbers densely
* but when you free a socket again it will mess up its bitmap and
* and use the wrong number the next time it needs a socket ID.
* Therefore, we wrap the handle alloc/destroy and add our own pid
* accounting.
*/
uint32_t port_bitmap[32] = { 0 };

struct nl_handle *nl80211_handle_alloc(void *cb)
{
	struct nl_handle *handle;
	uint32_t pid = getpid() & 0x3FFFFF;
	int i;

	handle = nl_handle_alloc_cb(cb);

	for (i = 0; i < 1024; i++) {
		if (port_bitmap[i / 32] & (1 << (i % 32)))
			continue;
		port_bitmap[i / 32] |= 1 << (i % 32);
		pid += i << 22;
		break;
	}

	nl_socket_set_local_port(handle, pid);

	return handle;
}

void nl80211_handle_destroy(struct nl_handle *handle)
{
	uint32_t port = nl_socket_get_local_port(handle);

	port >>= 22;
	port_bitmap[port / 32] &= ~(1 << (port % 32));

	nl_handle_destroy(handle);
}
#endif /* CONFIG_LIBNL20 */


/* create/delete nl80211_handle */

struct nl_handle * nl_create_handle(struct nl_cb *cb, const char *dbg)
{
	struct nl_handle *handle;

	handle = nl80211_handle_alloc(cb);
	if (handle == NULL) {
		fprintf(stderr, "nl80211: Failed to allocate netlink callbacks (%s)", dbg);
		return NULL;
	}

	if (genl_connect(handle)) {
		fprintf(stderr, "nl80211: Failed to connect to generic netlink (%s)", dbg);
		nl80211_handle_destroy(handle);
		return NULL;
	}

	return handle;
}

void nl_destroy_handles(struct nl_handle **handle) {
	if (*handle == NULL) return;
	fprintf(stderr, "nl_destroy_handles()\n");
	nl80211_handle_destroy(*handle); // seems to kill the app :(
	*handle = NULL;
}




#if __WORDSIZE == 64
#define ELOOP_SOCKET_INVALID	(intptr_t) 0x8888888888888889ULL
#else
#define ELOOP_SOCKET_INVALID	(intptr_t) 0x88888889ULL
#endif



/* register/unregister in eloop squeduler */

void nl80211_register_eloop_read(struct nl_handle **handle, eloop_sock_handler handler, void *eloop_data)
{
#ifdef CONFIG_LIBNL20
	/*
	* libnl uses a pretty small buffer (32 kB that gets converted to 64 kB)
	* by default. It is possible to hit that limit in some cases where
	* operations are blocked, e.g., with a burst of Deauthentication frames
	* to hostapd and STA entry deletion. Try to increase the buffer to make
	* this less likely to occur.
	*/
	if (nl_socket_set_buffer_size(*handle, 262144, 0) < 0) {
		wpa_printf(MSG_DEBUG,
			"nl80211: Could not set nl_socket RX buffer size: %s",
			strerror(errno));
		/* continue anyway with the default (smaller) buffer */
	}
#endif /* CONFIG_LIBNL20 */

	nl_socket_set_nonblocking(*handle);
	eloop_register_read_sock(nl_socket_get_fd(*handle), handler, eloop_data, *handle);
	//*handle = (void *)(((intptr_t)*handle) ^ ELOOP_SOCKET_INVALID); ??????
}

void nl80211_destroy_eloop_handle(struct nl_handle **handle) {
	//*handle = (void *)(((intptr_t)*handle) ^ ELOOP_SOCKET_INVALID); ??????
	eloop_unregister_read_sock(nl_socket_get_fd(*handle));
	nl_destroy_handles(handle);
}

