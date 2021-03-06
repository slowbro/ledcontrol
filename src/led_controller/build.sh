#!/bin/bash

mkdir -p tmp/{build,cache}
WD=$PWD/${0%/*}

OUT=`/usr/share/arduino/arduino-builder \
    -compile -logger=machine -hardware /usr/share/arduino/hardware \
    -hardware $HOME/.arduino15/packages -tools /usr/share/arduino/tools-builder \
    -tools $HOME/.arduino15/packages -libraries $HOME/Arduino/libraries \
    -fqbn=arduino:avr:uno:uploadspeed=19200 -ide-version=10805 -build-path $WD/tmp/build \
    -warnings=all -build-cache $WD/tmp/cache -prefs=build.warn_data_percentage=75 \
    -prefs=runtime.tools.arduinoOTA.path=$HOME/.arduino15/packages/arduino/tools/arduinoOTA/1.1.1 \
    -prefs=runtime.tools.avrdude.path=$HOME/.arduino15/packages/arduino/tools/avrdude/6.3.0-arduino9 \
    -prefs=runtime.tools.avr-gcc.path=$HOME/.arduino15/packages/arduino/tools/avr-gcc/4.9.2-atmel3.5.4-arduino2 \
    -verbose $WD/led_controller.ino`
RET=$?
echo "$OUT" | egrep -v '^===info'
echo -e "\n==========================================\n"

if [[ "$RET" != "0" ]];then

    echo -e "\e[31;1mBUILD FAILED! Look above for why...\e[0m"
    exit 1
fi

echo -e "\e[32;1mBuild successful.\e[0m"
cp -f $WD/tmp/build/led_controller.ino.hex $WD/led_controller.ino.hex
avr-size $WD/led_controller.ino.hex
