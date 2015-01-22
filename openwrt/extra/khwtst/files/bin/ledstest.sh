#!/bin/sh

pattern="0x44A 0x45c 0x44A 0x45c 0x44C 0x45A 0x44C 0x45A"
while [ 1 ]
do
        for V in $pattern
        do
                R=`reg 0x18000060`
                let "$R & 0x20"
                if [ $? = 1 ]; then
                        reg 0x18000064 0x45A > /dev/null
                        echo "button pressed"
                        exit 0;
                fi
                reg 0x18000064 $V > /dev/null
                sleep 1
        done
done

