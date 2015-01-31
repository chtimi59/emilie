#!/bin/bash
MAC="e8:4e:06:00:00:$1"

echo "----------------------------"
echo "test: $MAC"
echo "----------------------------"

#release IP 
#dhclient -4 -r wlan0
ip addr flush dev wlan0 label wlan0:0

#spoof mac adress
ifconfig wlan0 down hw ether $MAC

#ap connection
iwconfig wlan0 essid EMILIE
ifconfig wlan0 up

#ap connection
V=`iwconfig wlan0 | grep 98:FC:11:3D:3C:DE`
N=1
if [ -z "$V" ]
then 
	sleep 1s
	#retry
	V=`iwconfig wlan0 | grep 98:FC:11:3D:3C:DE`
	echo "[ $N ] ** $V **"
	let "N=$N+1"
	if [ $N -gt 5 ]
	then 
		echo "timeout failed to connect"
		exit 1
	fi
fi

#dhcp request
dhclient -4 -v wlan0
#dhclient -4 -v  -sf dhclient-script wlan0

#test IP
V=`ifconfig wlan0 | grep inet`
if [ -z "$V" ]
then
	echo "DCHP failed"
	exit 1
fi	

echo "----------------------------"
echo "result: $V"
echo "----------------------------"

exit 0

