#!/bin/bash

SERIAL=/dev/ttyUSB0
if [ ! -e $SERIAL ]; then
echo "no serial device $SERIAL"
exit 1
fi

FILE=$1
if [ -z $FILE ]; then
FILE=bin/brcm47xx/openwrt-e2000_v1-squashfs.bin
fi

if [ ! -e $FILE ]; then
echo "File $FILE do not exists"
exit 1
fi

echo "------------"
echo $FILE
echo "-----------"

read -p "Power DOWN your device and press [return]"
echo "Now Power UP your device and wait"
stty -F $SERIAL ispeed 115200 ospeed 115200 -echo

#Try to connect to CEF bootloader
T=0
while [ $T -lt 50 ]
do 
T=$[$T+1]
echo $'\cc' > $SERIAL
sleep 0.1
let T2=($T/10)*10
[[ $T2 == $T ]] && printf "*"
done
echo

chat -t1 "" "" "CFE>" < $SERIAL >$SERIAL
if [ $? -ne 0 ]; then
echo "Entering in GEC mode failed (timeout)"
exit 1
fi

#time to get ETH PHY
sleep 1
ping -W 1 -w 1 192.168.1.1
if [ $? -ne 0 ]; then
echo "no response on 192.168.1.1"
exit 1
fi


#Start tftp server
echo "flash -ctheader : flash0.trx" > $SERIAL

#start tftp client
pushd `dirname $FILE`
FILENAME=`basename $FILE`
echo "start tftp client with $FILENAME"
pwd
tftp 192.168.1.1 << EOF
binary
rexmt 1
timeout 60
trace
put $FILE 
quit
EOF
echo
echo "Transfert end"

#flash end detection (wait 30 seconds for CEF prompt)
echo "Flashing in NAND wait (may take around 1-2 minutes)"
T=0
while [ $T -lt 100 ]
do
T=$[$T+1]
sleep 1
printf "*"
chat -t1 -e "" "" "CFE>" < $SERIAL >$SERIAL
[[ $? == 0 ]] && T=100
done
echo
if [[ $T != 100 ]]; then
echo "No CEF prompt (timeout)"
exit 1
fi

echo "success end, rebooting device"
echo "reboot" > $SERIAL
echo
echo "----------"
echo "cat $SERIAL"
echo "Use CTRL-C to stop serial prompt"
echo "----------" 
cat $SERIAL


