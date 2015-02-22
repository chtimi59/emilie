#include "common.h"
#include "list.h"


/*
   eloop_data eloop contains :

     ------------------    -----------------    --------------------    --------------------    --------------------
     |  LinkedList    |    |     Array     |    |    Table of      |    |    Table of      |    |    Table of      |
	 |       of       |    |      of       |    |   eloop_sock     |    |   eloop_sock     |    |   eloop_sock     |
     | eloop_timeout  |    | eloop_signal  |    | for socket READ  |    | for socket WRITE |    | for socket EXPT  |
     ------------------    -----------------    --------------------    --------------------    --------------------

*/


struct eloop_timeout {
	struct dl_list list;
	struct realtime time;
	// ------ Callback ---------
	void *eloop_data;
	void *user_data;
	eloop_timeout_handler handler;
};
void eloop_remove_timeout(struct eloop_timeout *timeout);
int eloop_register_timeout(unsigned int secs, unsigned int usecs, eloop_timeout_handler handler, void *eloop_data, void *user_data);



struct eloop_signal {
	int sig;
	int signaled;
	void *user_data;
	eloop_signal_handler handler;	
};
int eloop_register_signal(int sig, eloop_signal_handler handler, void *user_data);





struct eloop_sock {
	int sock;
	// ------ Callback ---------
	void *eloop_data;
	void *user_data;
	eloop_sock_handler handler;
};
struct eloop_sock_table {
	int count;
	struct eloop_sock *table;
	int changed;
};
int eloop_sock_table_add_sock(struct eloop_sock_table *table, int sock, eloop_sock_handler handler, void *eloop_data, void *user_data);
void eloop_sock_table_remove_sock(struct eloop_sock_table *table, int sock);



struct eloop_data {

	int count;    // count total elements (in each tables)
	int max_sock; // socket max detection (file descriptor) see eloop_sock_table_add_sock

	// TABLES and LINKEDLIST
	struct eloop_sock_table readers;      // table
	struct eloop_sock_table writers;      // table
	struct eloop_sock_table exceptions;   // table
	struct dl_list          timeout;      // order linked list
	struct eloop_signal *   signals;      // array

	int signal_count;
	int signaled;
	int pending_terminate;

	int terminate; // terminated flag
};
static struct eloop_data eloop;


// Brutal exit!
static void eloop_handle_alarm(int sig)
{
	fprintf(stderr, "eloop: could not process SIGINT or SIGTERM in "
		"two seconds. Looks like there\n"
		"is a bug that ends up in a busy loop that "
		"prevents clean shutdown.\n"
		"Killing program forcefully.\n");
	exit(1);
}

// A unique signal handler count nb of trigger
void eloop_handle_signal(int sig)
{
	int i;
	if ((sig == SIGINT || sig == SIGTERM) && !eloop.pending_terminate) {
		/* Use SIGALRM to break out from potential busy loops that
		* would not allow the program to be killed. */
		eloop.pending_terminate = 1;
		signal(SIGALRM, eloop_handle_alarm);
		alarm(2);
	}

	eloop.signaled++;
	for (i = 0; i < eloop.signal_count; i++) {
		if (eloop.signals[i].sig == sig) {
			eloop.signals[i].signaled++;
			break;
		}
	}
}

// for sockets fd_set must be initialize for select() for multiple waiting
void eloop_sock_table_set_fds(struct eloop_sock_table *table, fd_set *fds) {
	int i;

	// clear fds
	FD_ZERO(fds);
	
	// set fds for all elements of table
	if (table->table == NULL) return;
	for (i = 0; i < table->count; i++) {
		assert(table->table[i].sock >= 0);
		FD_SET(table->table[i].sock, fds);
	}
}

// and then we handle it (factorisation purpose, called from eloop_run)
void eloop_sock_table_dispatch(struct eloop_sock_table *table, fd_set *fds) {
	int i;

	if (table == NULL || table->table == NULL) return;

	table->changed = 0;
	for (i = 0; i < table->count; i++) {
		if (FD_ISSET(table->table[i].sock, fds)) {
			/* ** Something append on fds for this socket **
			   ** >>> CALL THE HANDLER! ***/
			table->table[i].handler(table->table[i].sock, table->table[i].eloop_data, table->table[i].user_data);
			if (table->changed)
				break;
		}
	}
}



// ------------- SCHEDULER ----------

int eloop_init(void) {
	memset(&eloop, 0, sizeof(eloop));
	dl_list_init(&eloop.timeout);
	return 0;
}

void eloop_terminate(void) {
	eloop.terminate = 1;
}

void eloop_run(void)
{
	int res;
	struct realtime tv, now;
	fd_set *rfds, *wfds, *efds;
	struct timeval _tv;

	rfds = malloc(sizeof(*rfds));
	wfds = malloc(sizeof(*wfds));
	efds = malloc(sizeof(*efds));
	if (rfds == NULL || wfds == NULL || efds == NULL)
		goto out;

	// MAIN LOOP
	while ((!eloop.terminate) &&				// if not terminated
		   ( (!dl_list_empty(&eloop.timeout)) || // something in timeout Linkedlist| 
		      eloop.readers.count > 0 ||          // something in reader sockets (table)
		      eloop.writers.count > 0 ||		    // something in writes sockets (table)
		      eloop.exceptions.count > 0		    // something in execption (table)
		   )
		)
	{


		// A1- Compute select timeout if we are waiting for something in eloop_timeout item list
		struct eloop_timeout *timeout;
		timeout = dl_list_first(&eloop.timeout, struct eloop_timeout, list);
		if (timeout) {
			get_realtime(&now);
			if (realtime_before(&now, &timeout->time))
				realtime_sub(&timeout->time, &now, &tv);
			else
				tv.sec = tv.usec = 0;
			_tv.tv_sec = tv.sec;
			_tv.tv_usec = tv.usec;
		}
		
		// A2- setup fds for select
		eloop_sock_table_set_fds(&eloop.readers, rfds);
		eloop_sock_table_set_fds(&eloop.writers, wfds);
		eloop_sock_table_set_fds(&eloop.exceptions, efds);

		// *** WAIT ! ***
		res = select(eloop.max_sock + 1, rfds, wfds, efds, timeout ? &_tv : NULL);
		if (res < 0 && errno != EINTR && errno != 0) {
			fprintf(stderr, "eloop: %s\n", strerror(errno));
			goto out;
		}

		// ACTIONS

		/* 0- Check signals */
		if (eloop.signaled != 0) {
			
			int i;

			if (eloop.pending_terminate) {
				alarm(0);
				eloop.pending_terminate = 0;
			}

			for (i = 0; i < eloop.signal_count; i++) {
				if (eloop.signals[i].signaled) {
					eloop.signals[i].signaled = 0;
					eloop.signals[i].handler(eloop.signals[i].sig,
					eloop.signals[i].user_data);
				}
			}
		}

		/* 1- Check if some registered timeouts have occurred */
		timeout = dl_list_first(&eloop.timeout, struct eloop_timeout, list);
		if (timeout) {
			get_realtime(&now);
			if (!realtime_before(&now, &timeout->time)) {
				void *eloop_data = timeout->eloop_data;
				void *user_data = timeout->user_data;
				eloop_timeout_handler handler = timeout->handler;
				eloop_remove_timeout(timeout);
				handler(eloop_data, user_data);
			}
		}

		/* 3- Check sockets */
		if (res <= 0) continue;
		eloop_sock_table_dispatch(&eloop.readers, rfds);
		eloop_sock_table_dispatch(&eloop.writers, wfds);
		eloop_sock_table_dispatch(&eloop.exceptions, efds);
	}

	
	eloop.terminate = 0;

out:
	free(rfds);
	free(wfds);
	free(efds);
	return;
}





/* Create a new eloop_timeout element and add it in linkedlist */
int eloop_register_timeout(unsigned int secs, unsigned int usecs, eloop_timeout_handler handler, void *eloop_data, void *user_data)
{
	struct eloop_timeout *timeout, *tmp;
	time_t now_sec;

	timeout = zalloc(sizeof(*timeout));
	if (timeout == NULL)
		return -1;
	
	// no event in the past !
	if (get_realtime(&timeout->time) < 0) {
		free(timeout);
		return -1;
	}
	
	now_sec = timeout->time.sec;
	timeout->time.sec += secs;
	if (timeout->time.sec < now_sec) {
		/*
		* Integer overflow - assume long enough timeout to be assumed
		* to be infinite, i.e., the timeout would never happen.
		*/
		fprintf(stderr, "Too long timeout (secs=%u) to ever happen - ignore it\n", secs);
		free(timeout);
		return 0;
	}
	timeout->time.usec += usecs;
	while (timeout->time.usec >= 1000000) {
		timeout->time.sec++;
		timeout->time.usec -= 1000000;
	}
	timeout->eloop_data = eloop_data;
	timeout->user_data = user_data;
	timeout->handler = handler;
	
	/* Maintain timeouts in order of increasing time */
	dl_list_for_each(tmp, &eloop.timeout, struct eloop_timeout, list) {
		if (realtime_before(&timeout->time, &tmp->time)) {
			dl_list_add(tmp->list.prev, &timeout->list);
			return 0;
		}
	}

	dl_list_add_tail(&eloop.timeout, &timeout->list);	
	return 0;
}

/* Remove a eloop_timeout element in linked list */
void eloop_remove_timeout(struct eloop_timeout *timeout) {
	dl_list_del(&timeout->list);
	free(timeout);
}



/* Create a new eloop_signal_handler element and add it in array */
int eloop_register_signal(int sig, eloop_signal_handler handler, void *user_data)
{
	struct eloop_signal *tmp;

	tmp = realloc_array(eloop.signals, eloop.signal_count + 1, sizeof(struct eloop_signal));
	if (tmp == NULL)
		return -1;

	tmp[eloop.signal_count].sig = sig;
	tmp[eloop.signal_count].user_data = user_data;
	tmp[eloop.signal_count].handler = handler;
	tmp[eloop.signal_count].signaled = 0;
	eloop.signal_count++;
	eloop.signals = tmp;
	signal(sig, eloop_handle_signal);
	return 0;
}

int eloop_register_signal_terminate(eloop_signal_handler handler, void *user_data) {
	int ret = eloop_register_signal(SIGINT, handler, user_data);
	if (ret == 0) ret = eloop_register_signal(SIGTERM, handler, user_data);
	return ret;
}



/* Create a new eloop_sock element and add it in table */
int eloop_sock_table_add_sock(struct eloop_sock_table *table, int sock, eloop_sock_handler handler, void *eloop_data, void *user_data)
{		
		struct eloop_sock *tmp;
		if (table == NULL) return -1;

		// Compute new Max
		int new_max_sock;
		new_max_sock = (sock > eloop.max_sock)?sock:eloop.max_sock;

		// Insert new element
		tmp = realloc_array(table->table, table->count + 1, sizeof(struct eloop_sock));
		if (tmp == NULL) return -1;
		tmp[table->count].sock = sock;
		tmp[table->count].eloop_data = eloop_data;
		tmp[table->count].user_data = user_data;
		tmp[table->count].handler = handler;
		table->count++;
		table->table = tmp;
		eloop.max_sock = new_max_sock;
		eloop.count++;
		table->changed = 1;

		return 0;
	}

/* Remove a eloop_sock element in table */
void eloop_sock_table_remove_sock(struct eloop_sock_table *table, int sock)
{
	int i;

	if (table == NULL || table->table == NULL || table->count == 0) return;

	// Search for element
	for (i = 0; i < table->count; i++) {
		if (table->table[i].sock == sock)
			break;
	}
	if (i == table->count) return;

	// found! simple memory translation
	if (i != table->count - 1) memmove(&table->table[i], &table->table[i + 1], (table->count - i - 1) * sizeof(struct eloop_sock));
	table->count--;
	eloop.count--;
	table->changed = 1;
}


struct eloop_sock_table *eloop_get_sock_table(eloop_event_type type) {
	switch (type) {
	case EVENT_TYPE_READ:
		return &eloop.readers;
	case EVENT_TYPE_WRITE:
		return &eloop.writers;
	case EVENT_TYPE_EXCEPTION:
		return &eloop.exceptions;
	}
	return NULL;
}

int eloop_register_sock(int sock, eloop_event_type type, eloop_sock_handler handler, void *eloop_data, void *user_data) {
	struct eloop_sock_table *table;
	assert(sock >= 0);
	table = eloop_get_sock_table(type);
	return eloop_sock_table_add_sock(table, sock, handler, eloop_data, user_data);
}

void eloop_unregister_sock(int sock, eloop_event_type type) {
	struct eloop_sock_table *table;
	table = eloop_get_sock_table(type);
	eloop_sock_table_remove_sock(table, sock);
}






int eloop_register_read_sock(int sock, eloop_sock_handler handler, void *eloop_data, void *user_data) {
	return eloop_register_sock(sock, EVENT_TYPE_READ, handler, eloop_data, user_data);
}

void eloop_unregister_read_sock(int sock) {
	eloop_unregister_sock(sock, EVENT_TYPE_READ);
}
