# İndirme ve Kurulum

:::tip
These are **daily build** download links with the latest features. If you are looking for the last stable release, see the [stable docs](https://docs.qgroundcontrol.com/Stable_V5.0/en/qgc-user-guide/getting_started/download_and_install.html).
:::

:::tip
See [Troubleshooting QGC Setup](../troubleshooting/qgc_setup.md) if _QGroundControl_ doesn't start and run properly after installation!
:::

## Sistem Gereksinimleri

QGC tüm güncel bilgisayar ya da mobil cihazlarda iyi bir şekilde çalışacaktır. taraf uygulamalara ve mevcut sistem kaynaklarına bağlıdır.
Daha yüksek kapasiteli donanım daha iyi bir deneyim sunacaktır.
En az 8 Gb Ram'e, SSD'ye, Nvidia ya da AMD ekran kartına ve i5 veya daha iyi bir işlemciye sahip bir bilgisayar çoğu uygulama için yeterince iyi olacaktır.

En iyi deneyim ve uyumluluk için size işletim sisteminizin en yeni sürümünü kullanmanızı öneriyoruz.

## Windows {#windows}

Supported versions: Windows 10 (1809 or later), Windows 11:

1. Download the installer:
   - [x86_64](https://d176tv9ibo4jno.cloudfront.net/builds/master/QGroundControl-installer-AMD64.exe)
   - [Arm64](https://d176tv9ibo4jno.cloudfront.net/builds/master/QGroundControl-installer-ARM64.exe)
2. exe'ye çift tıklayın.

:::info
Windows kurulum programı 3 kısayol oluşturur: **QGroundControl**, **GPU Compatibility Mode**, **GPU Safe Mode**.
Eğer başlatma veya video işleme sorunları yaşamıyorsanız ilk kısayolu kullanın.
For more information see [Troubleshooting QGC Setup > Windows: UI Rendering/Video Driver Issues](../troubleshooting/qgc_setup.md#opengl_troubleshooting).
:::

## Mac OS {#macOS}

Supported versions: macOS 13 (Ventura) or later:

<!-- match version using https://docs.qgroundcontrol.com/master/en/qgc-dev-guide/getting_started/#native-builds -->

<!-- usually based on Qt macOS dependency -->

1. Download [QGroundControl.dmg](https://d176tv9ibo4jno.cloudfront.net/builds/master/QGroundControl.dmg).
2. .dmg dosyasına çift tıklayın, ardından çıkan ekranda _QGroundControl_'ü _Application_ dosyasına sürükleyin.

:::info
QGroundControl continues to not be signed. You will not to allow permission for it to install based on your macOS version.
:::

## Ubuntu Linux {#ubuntu}

Supported versions: Ubuntu 22.04, 24.04:

Ubuntu, bir seri bağlantı noktasının (veya USB serisinin) robotikle ilgili kullanımına müdahale eden bir seri modem yöneticisi ile birlikte gelir.
_ QGroundControl _ 'ü kurmadan önce modem yöneticisini kaldırmalı ve seri bağlantı noktasına erişim için kendinize izin vermelisiniz.
Ayrıca video akışını desteklemek için _ GStreamer _ 'ı da yüklemeniz gerekmektedir.

**Before installing _QGroundControl_ for the first time:**

1. Enable serial-port access
   Add your user to the dialout group so you can talk to USB devices without root:

```
sudo usermod -aG dialout "$(id -un)"
```

:::info
At login, your shell takes a snapshot of your user and group memberships. Because you just changed groups, you need a fresh login shell to pick up “dialout” access. Logging out and back in reloads that snapshot, so you get the new permissions.
:::

1. (Optional) Disable ModemManager
   On some Ubuntu-based systems, ModemManager can claim serial ports that QGC needs. If you don't use it elsewhere, mask or remove it.

```
# preferred: stop and mask the service
sudo systemctl mask --now ModemManager.service

# or, if you’d rather remove the package
sudo apt remove --purge modemmanager
```

1. On the command prompt, enter:

```sh
sudo apt install gstreamer1.0-plugins-bad gstreamer1.0-libav gstreamer1.0-gl -y
sudo apt install python3-gi python3-gst-1.0 -y
sudo apt install libfuse2 -y
sudo apt install libxcb-xinerama0 libxkbcommon-x11-0 libxcb-cursor-dev -y
```

**To install _QGroundControl_:**

1. Download the AppImage for your architecture:
   - [Linux x86_64](https://d176tv9ibo4jno.cloudfront.net/builds/master/QGroundControl-x86_64.AppImage)
   - [Linux aarch64](https://d176tv9ibo4jno.cloudfront.net/builds/master/QGroundControl-aarch64.AppImage)

2. Make the AppImage executable

```
chmod +x QGroundControl-<arch>.AppImage
```

1. Run QGroundControl
   Either double-click the AppImage in your file manager or launch it from a terminal:

```
./QGroundControl-<arch>.AppImage
```

## Android {#android}

Supported versions: Android 9 (API 28) or later (arm 32/64):

- [Android APK](https://d176tv9ibo4jno.cloudfront.net/builds/master/QGroundControl.apk)

:::important
The version of Qt used by QGroundControl requires Android 9 (API 28) as the minimum supported version. It is not possible to support older Android releases. This means that some integrated controllers running older versions of Android are no longer compatible with current builds of QGroundControl. QGroundControl 5.0 stable is the last release that supports these older devices. Note that 5.0 may not fully support firmware versions released after it, so users on older controllers may experience limited compatibility with newer autopilot firmware.
:::

## Old Stable Releases

Old stable releases can be found on <a href="https://github.com/mavlink/qgroundcontrol/releases/" target="_blank">GitHub</a>.
