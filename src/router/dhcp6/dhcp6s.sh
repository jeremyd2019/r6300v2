#!/bin/sh
#
# dhcp6s        dhcp6s is an implementation of DHCPv6 server.
#               This shell script takes care of starting and stopping
#               dhcp6s.
#
# chkconfig: - 66 36
# description: dhcp6s supports server side of  Dynamic Host Configuration
#              Protocol for IPv6.
# processname: dhcp6s
# config: /etc/dhcp6s.conf
# config: /etc/server6_addr.conf
# config: /etc/sysconfig/dhcp6s

# Source function library.
. /etc/rc.d/init.d/functions

# Source networking configuration.
. /etc/sysconfig/network
. /etc/sysconfig/dhcp6s

# Check that networking is up.
[ ${NETWORKING} = "no" ] && exit 0

# Check that files exist
[ -f /usr/local/sbin/dhcp6s ] || exit 0
[ -f /etc/dhcp6s.conf ] || exit 0
[ -f /etc/server6_addr.conf ] || exit 0
[ ${DHCP6SIF} = "" ] && exit 0

RETVAL=0
prog="dhcp6s"

start() {
	# Start daemons.
	echo -n $"Starting $prog: "
	daemon /usr/local/sbin/dhcp6s -c /etc/dhcp6s.conf ${DHCP6SARGS} ${DHCP6SIF}
	RETVAL=$?
	echo
	[ $RETVAL -eq 0 ] && touch /var/lock/subsys/dhcp6s
	return $RETVAL
}

stop() {
	# Stop daemons.
	echo -n $"Shutting down $prog: "
	killproc dhcp6s
	RETVAL=$?
	echo
	[ $RETVAL -eq 0 ] && rm -f /var/lock/subsys/dhcp6s
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
	if [ -f /var/lock/subsys/dhcp6s ]; then
	    stop
	    start
	    RETVAL=$?
	fi
	;;
  status)
	status dhcp6s
	RETVAL=$?
	;;
  *)
	echo $"Usage: $0 {start|stop|restart|condrestart|status}"
	exit 1
esac

exit $RETVAL
