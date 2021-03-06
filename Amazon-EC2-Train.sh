#!/bin/bash

# Connect
chmod 400 ./TestInstance2.pem
ssh -i ./TestInstance2.pem ubuntu@ec2-52-42-13-8.us-west-2.compute.amazonaws.com

scp -rp -i ./TestInstance2.pem ./* ubuntu@ec2-52-42-13-8.us-west-2.compute.amazonaws.com:~/Deeper/


# Add paging:
# https://stackoverflow.com/questions/17173972/how-do-you-add-swap-to-an-ec2-instance

# To add this extra space to your instance you type:
sudo /bin/dd if=/dev/zero of=/var/swap.1 bs=1M count=8192
sudo /sbin/mkswap /var/swap.1
sudo /sbin/swapon /var/swap.1

# To enable it by default after reboot, add this line to /etc/fstab:
#/var/swap.1 swap swap defaults 0 0

# Prepare the environment
sudo apt-get update -qq -y
sudo apt-get install -qq git cmake opencl-headers build-essential unzip

# Install Catalyst dependencies
sudo apt-get install -y cdbs dh-make dkms execstack dh-modaliases linux-headers-generic libqtgui4 xserver-xorg-dev-lts-trusty lib32gcc1 libxrandr2 libxcursor1 libgl1-mesa-glx libxinerama1

# From the http://wiki.cchtml.com/index.php/Ubuntu_Trusty_Installation_Guide#Installing_Proprietary_Drivers_a.k.a._Catalyst.2Ffglrx
# Download the latest Catalyst package
mkdir catalyst-14.6beta1.0jul11
pushd catalyst-14.6beta1.0jul11
    wget --referer='http://support.amd.com/en-us/download/desktop?os=Linux+x86' http://www2.ati.com/drivers/beta/linux-amd-catalyst-14.6-beta-v1.0-jul11.zip
    unzip linux-amd-catalyst-14.6-beta-v1.0-jul11.zip
    # Create and install .deb packages
    pushd fglrx-14.20
        sudo ./amd-driver-installer-14.20-x86.x86_64.run --buildpkg Ubuntu/trusty
        sudo apt-get -f install -y
        sudo dpkg -i fglrx*.deb
    popd
popd

# Build and test TinyRNN
git clone https://github.com/peterrudenko/TinyRNN.git
pushd TinyRNN
    git submodule init && git submodule update && git submodule status

    # Fetch OpenCL C++ binding
    mkdir -p ./Source/OpenCL
    pushd ./Source/OpenCL
        wget -w 1 -r -np -nd -nv -A h,hpp https://www.khronos.org/registry/cl/api/2.1/cl.hpp
    popd

    #cmake -HProjects/CMake -BBuild -DCMAKE_BUILD_TYPE=Release -DTINYRNN_OPENCL_ACCELERATION=0
    cmake -HProjects/CMake -BBuild -DCMAKE_BUILD_TYPE=Release
    pushd Build
        make
        ctest -V
    popd
popd

# JUCE modules' dependencies
sudo apt-get -f install
sudo apt-get install -y libfreetype6-dev libxrandr-dev libxinerama-dev libasound-dev

# Build GoDeeper trainer
git clone -b develop https://github.com/peterrudenko/GoDeeper.git
pushd GoDeeper
    git checkout develop
    git submodule init && git submodule update && git submodule status

    # Fetch OpenCL C++ binding
    mkdir -p ./Source/Playground/OpenCL
    pushd ./Source/Playground/OpenCL
        wget -w 1 -r -np -nd -nv -A h,hpp https://www.khronos.org/registry/cl/api/2.1/cl.hpp
    popd

    pushd Projects/LinuxMakefile/GoDeeper
        export CONFIG=Release && make clean && make
        sudo rm -r /usr/bin/deeper
        sudo ln -s $PWD/build/GoDeeper /usr/bin/deeper
    popd
popd

# Train!
deeper init Arper 64 128 256 128 64
# copy the generated source and rebuild GoDeeper


deeper train Arper targets="Targets"
