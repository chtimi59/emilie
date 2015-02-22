#ifndef ELOOP_H
#define ELOOP_H

/**
* eloop_event_type - eloop socket event type for eloop_register_sock()
* @EVENT_TYPE_READ: Socket has data available for reading
* @EVENT_TYPE_WRITE: Socket has room for new data to be written
* @EVENT_TYPE_EXCEPTION: An exception has been reported
*/
typedef enum {
	EVENT_TYPE_READ = 0,
	EVENT_TYPE_WRITE,
	EVENT_TYPE_EXCEPTION
} eloop_event_type;



/**
* eloop_sock_handler - eloop socket event callback type
* @sock: File descriptor number for the socket
* @eloop_ctx: Registered callback context data (eloop_data)
* @sock_ctx: Registered callback context data (user_data)
*/
typedef void(*eloop_sock_handler)(int sock, void *eloop_ctx, void *sock_ctx);

/**
* eloop_timeout_handler - eloop timeout event callback type
* @eloop_ctx: Registered callback context data (eloop_data)
* @sock_ctx: Registered callback context data (user_data)
*/
typedef void(*eloop_timeout_handler)(void *eloop_data, void *user_ctx);


/**
* eloop_signal_handler - eloop signal event callback type
* @sig: Signal number
* @signal_ctx: Registered callback context data (user_data from
* eloop_register_signal(), eloop_register_signal_terminate(), or
* eloop_register_signal_reconfig() call)
*/
typedef void(*eloop_signal_handler)(int sig, void *signal_ctx);




/**
* eloop_register_read_sock - Register handler for read events
* @sock: File descriptor number for the socket
* @handler: Callback function to be called when data is available for reading
* @eloop_data: Callback context data (eloop_ctx)
* @user_data: Callback context data (sock_ctx)
* Returns: 0 on success, -1 on failure
*
* Register a read socket notifier for the given file descriptor. The handler
* function will be called whenever data is available for reading from the
* socket. The handler function is responsible for clearing the event after
* having processed it in order to avoid eloop from calling the handler again
* for the same event.
*/
int eloop_register_read_sock(int sock, eloop_sock_handler handler,
	void *eloop_data, void *user_data);

/**
* eloop_unregister_read_sock - Unregister handler for read events
* @sock: File descriptor number for the socket
*
* Unregister a read socket notifier that was previously registered with
* eloop_register_read_sock().
*/
void eloop_unregister_read_sock(int sock);






/**
* eloop_register_timeout - Register timeout
* @secs: Number of seconds to the timeout
* @usecs: Number of microseconds to the timeout
* @handler: Callback function to be called when timeout occurs
* @eloop_data: Callback context data (eloop_ctx)
* @user_data: Callback context data (sock_ctx)
* Returns: 0 on success, -1 on failure
*
* Register a timeout that will cause the handler function to be called after
* given time.
*/
int eloop_register_timeout(unsigned int secs, unsigned int usecs,
	eloop_timeout_handler handler,
	void *eloop_data, void *user_data);








/**
* eloop_register_signal - Register handler for signals
* @sig: Signal number (e.g., SIGHUP)
* @handler: Callback function to be called when the signal is received
* @user_data: Callback context data (signal_ctx)
* Returns: 0 on success, -1 on failure
*
* Register a callback function that will be called when a signal is received.
* The callback function is actually called only after the system signal
* handler has returned. This means that the normal limits for sighandlers
* (i.e., only "safe functions" allowed) do not apply for the registered
* callback.
*/
int eloop_register_signal(int sig, eloop_signal_handler handler,
	void *user_data);

/**
* eloop_register_signal_terminate - Register handler for terminate signals
* @handler: Callback function to be called when the signal is received
* @user_data: Callback context data (signal_ctx)
* Returns: 0 on success, -1 on failure
*
* Register a callback function that will be called when a process termination
* signal is received. The callback function is actually called only after the
* system signal handler has returned. This means that the normal limits for
* sighandlers (i.e., only "safe functions" allowed) do not apply for the
* registered callback.
*
* This function is a more portable version of eloop_register_signal() since
* the knowledge of exact details of the signals is hidden in eloop
* implementation. In case of operating systems using signal(), this function
* registers handlers for SIGINT and SIGTERM.
*/
int eloop_register_signal_terminate(eloop_signal_handler handler,
	void *user_data);







/**
* eloop_init() - Initialize global event loop data
* Returns: 0 on success, -1 on failure
*
* This function must be called before any other eloop_* function.
*/
int eloop_init(void);

/**
* eloop_run - Start the event loop
*
* Start the event loop and continue running as long as there are any
* registered event handlers. This function is run after event loop has been
* initialized with event_init() and one or more events have been registered.
*/
void eloop_run(void);

/**
* eloop_terminate - Terminate event loop
*
* Terminate event loop even if there are registered events. This can be used
* to request the program to be terminated cleanly.
*/
void eloop_terminate(void);


#endif