#!/bin/bash

cd src/optiboot/optiboot/bootloaders/optiboot
make clean
make $@
echo
mv -v *.hex *.lst $OLDPWD/bootloaders
make clean >/dev/null 2>&1
