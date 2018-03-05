#!/bin/bash

mkdir -p tmp/{build,cache}
WD=$PWD/${0%/*}

/usr/share/arduino/arduino-builder \
    -compile -logger=machine -hardware /usr/share/arduino/hardware \
    -hardware /home/katelyn/.arduino15/packages -tools /usr/share/arduino/tools-builder \
    -tools /home/katelyn/.arduino15/packages -libraries /home/katelyn/Arduino/libraries \
    -fqbn=arduino:avr:uno:uploadspeed=19200 -ide-version=10805 -build-path $WD/tmp/build \
    -warnings=all -build-cache $WD/tmp/cache -prefs=build.warn_data_percentage=75 \
    -prefs=runtime.tools.arduinoOTA.path=/home/katelyn/.arduino15/packages/arduino/tools/arduinoOTA/1.1.1 \
    -prefs=runtime.tools.avrdude.path=/home/katelyn/.arduino15/packages/arduino/tools/avrdude/6.3.0-arduino9 \
    -prefs=runtime.tools.avr-gcc.path=/home/katelyn/.arduino15/packages/arduino/tools/avr-gcc/4.9.2-atmel3.5.4-arduino2 \
    -verbose $WD/led_controller.ino \
    | grep -v "^===info"

echo -e "\n==========================================\n"

if [[ "$?" != "0" ]];then
    echo "BUILD FAILED! Look above for why..."
    exit 1
fi

echo "Build successful."
cp -f $WD/tmp/build/led_controller.ino.hex $WD/led_controller.ino.hex
avr-size $WD/led_controller.ino.hex
