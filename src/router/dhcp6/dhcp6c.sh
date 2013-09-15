#!/bin/sh
#
# dhcp6c        dhcp6c is an implementation of DHCPv6 client.
#               This shell script takes care of starting and stopping
#               dhcp6c.
#
# chkconfig: - 67 37
# description: dhcp6c supports client side of Dynamic Host Configuration
#              Protocol for IPv6.
# processname: dhcp6c
# config: /etc/dhcp6c.conf
# config: /etc/sysconfig/dhcp6c

# Source function library.
. /etc/rc.d/init.d/functions

# Source networking configuration.
. /etc/sysconfig/network
. /etc/sysconfig/dhcp6c

# Check that networking is up.
[ ${NETWORKING} = "no" ] && exit 0

# Check that files exist
[ -f /usr/local/sbin/dhcp6c ] || exit 0
[ -f /etc/dhcp6c.conf ] || exit 0
[ ${DHCP6CIF} = "" ] && exit 0

RETVAL=0
prog="dhcp6c"

start() {
	# Start daemons.
	echo -n $"Starting $prog: "
	daemon /usr/local/sbin/dhcp6c -c /etc/dhcp6c.conf ${DHCP6CARGS} ${DHCP6CIF}
	RETVAL=$?
	echo
	[ $RETVAL -eq 0 ] && touch /var/lock/subsys/dhcp6c
	return $RETVAL
}

stop() {
	# Stop daemons.
	echo -n $"Shutting down $prog: "
	killproc dhcp6c
	RETVAL=$?
	echo
	[ $RETVAL -eq 0 ] && rm -f /var/lock/subsys/dhcp6c
	return $RETVAL
}

release() {
	# Release addresses.
	echo -n $"Releasing assigned addresses $prog: "
	daemon /usr/local/sbin/dhcp6c ${DHCP6CARGS} -r all ${DHCP6CIF}
	RETVAL=$?
	echo
	return $RETVAL
}

# See how we were called.
case "$1" in
  start)
	start
	;;
  stop)
	stop
	;;
  restart|reload)
	stop
	start
	RETVAL=$?
	;;
  condrestart)
	if [ -f /var/lock/subsys/dhcp6c ]; then
	    stop
	    start
	    RETVAL=$?
	fi
	;;
  status)
	status dhcp6c
	RETVAL=$?
	;;
  release)
	[ ! -f /var/lock/subsys/dhcp6c ] || exit 0
	release dhcp6c
	RETVAL=$?
	;;
  *)
	echo $"Usage: $0 {start|stop|restart|condrestart|status|release}"
	exit 1
esac

exit $RETVAL
