# Here is a simple little setup for RUM. Not all of this can be automated.

echo  "The first thing to do is enable hugepages for your machine. Add the following in /etc/default/grub.d/50-curtin-settings.cfg : \"default_hugepagesz=1G hugepagesz=1G hugepages=4 iommu=pt intel_iommu=on\""

# add this to get the right repos avaliable
sudo add-apt-repository universe

#now install some necessary things
sudo apt-get install libpcap-dev gcc make hugepages nim pkg-config libnuma-dev \
lua5.3 liblua5.3-dev libpcap-dev dpdk-igb-uio-dkms libssl-dev

#build the dpdk
sudo make config T=${RTE_TARGET} RTE_KERNELDIR=/lib/modules/$(shell uname -r)/build
sudo make T=${RTE_TARGET} RTE_KERNELDIR=/lib/modules/$(shell uname -r)/build
sudo make install T=${RTE_TARGET} RTE_KERNELDIR=/lib/modules/$(shell uname -r)/build DESTDIR=${RTE_TARGET}

#load the module
sudo modprobe igb_uio

# intereseting and useful commands

# list the avaliable devices
sudo python $RTE_SDK/usertools/dpdk-devbind.py --status-dev net

# replace a port with the custom driver
sudo python $RTE_SDK/usertools/dpdk-devbind.py -b igb_uio <pcie of the device>

