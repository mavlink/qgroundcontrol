## Running QGC or AGS on a stock CentOS:

### Install SDL2 and GStreamer
```
wget http://download-ib01.fedoraproject.org/pub/epel/7/x86_64/Packages/s/SDL2-2.0.9-1.el7.x86_64.rpm
sudo yum localinstall SDL2-2.0.9-1.el7.x86_64.rpm
sudo yum install gstreamer1*
```

### Add current user to the dialout group
```
sudo usermod -a -G dialout $USER
```

### Assuming the QGroundControl.tar.bz2 binary is in the current directory:
```
mkdir qgc
cd qgc
tar -xvjf ../QGroundControl.tar.bz2
cd qgroundcontrol
./qgroundcontrol-start.sh
```

## Building QGC or AGS on a stock CentOS:

### Basic dev tools
```
sudo yum update
sudo yum groupinstall "Development Tools"
sudo yum install mesa-libGL-devel
```
### SDL2 (from rpm)
```
wget http://download-ib01.fedoraproject.org/pub/epel/7/x86_64/Packages/s/SDL2-2.0.9-1.el7.x86_64.rpm
wget http://download-ib01.fedoraproject.org/pub/epel/7/x86_64/Packages/s/SDL2-devel-2.0.9-1.el7.x86_64.rpm

sudo yum localinstall SDL2-2.0.9-1.el7.x86_64.rpm
sudo yum localinstall SDL2-devel-2.0.9-1.el7.x86_64.rpm
```

### GStreamer
```
sudo yum install gstreamer1*
```
### Qt
* Install Qt 5.12.4 from the Qt installation script (download, run the script, install Qt 5.12.4 under ~/devel/Qt)

### Build QGC
```
mkdir devel
cd devel
git clone --recursive https://github.com/mavlink/qgroundcontrol.git
cd qgroundcontrol
../Qt/5.12.4/gcc_64/bin/qmake ../qgroundcontrol/qgroundcontrol.pro -spec linux-clang CONFIG+=qtquickcompiler CONFIG+=installer
make -j32
```


