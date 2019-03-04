# First time

Add the right repository form [here](https://launchpad.net/~beineri)

For instance, on Ubuntu Trusty (14.04)

    sudo add-apt-repository ppa:beineri/opt-qt596-trusty -y

whilst for Ubuntu Xenial (16.04)

    sudo add-apt-repository ppa:beineri/opt-qt596-xenial -y

Then, run:

    sudo apt-get update
    sudo apt-get install qt59base qt59svg -y

Download the latest version of [LinuxDeployQt](https://github.com/probonopd/linuxdeployqt) and make it executable with __chmod__:

    wget "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage" -O ~/linuxdeployq.AppImage
    chmod +x ~/linuxdeployq.AppImage

# Build the AppImage

In the root folder of Groot:

    mkdir build; cd build
    source /opt/qt59/bin/qt59-env.sh
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=install ..
    make -j$(nproc) install

    export VERSION=$(git describe --abbrev=0 --tags); echo $VERSION
    unset QTDIR; unset QT_PLUGIN_PATH ; unset LD_LIBRARY_PATH
    ~/linuxdeployq.AppImage ./install/share/applications/Groot.desktop  -appimage


