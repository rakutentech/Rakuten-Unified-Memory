#!/bin/sh

cd $RTE_SDK
sudo make config T=${RTE_TARGET} RTE_KERNEL_DIR=/lib/modules/$(uname -r)/build
sudo make T=${RTE_TARGET} RTE_KERNEL_DIR=/lib/modules/$(uname -r)/build
sudo make install T=${RTE_TARGET} RTE_KERNEL_DIR=/lib/modules/$(uname -r)/build DESTDIR=${RTE_TARGET}
cd -

