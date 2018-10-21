/* Crystax provides the kevent/kqueue API, but it doesn't work properly: it gives a "bad
 * address" error. This is not specific to Python: it affects libzmq as well
 * (https://tracker.crystax.net/issues/1433). */
#error Header disabled
