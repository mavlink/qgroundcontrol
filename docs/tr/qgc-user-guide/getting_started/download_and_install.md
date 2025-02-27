# İndirme ve Kurulum

The sections below can be used to download the [current stable release](../releases/release_notes.md) of _QGroundControl_ for each platform.

:::tip
See [Troubleshooting QGC Setup](../troubleshooting/qgc_setup.md) if _QGroundControl_ doesn't start and run properly after installation!
:::

## Sistem Gereksinimleri

QGC tüm güncel bilgisayar ya da mobil cihazlarda iyi bir şekilde çalışacaktır. taraf uygulamalara ve mevcut sistem kaynaklarına bağlıdır.
Daha yüksek kapasiteli donanım daha iyi bir deneyim sunacaktır.
En az 8 Gb Ram'e, SSD'ye, Nvidia ya da AMD ekran kartına ve i5 veya daha iyi bir işlemciye sahip bir bilgisayar çoğu uygulama için yeterince iyi olacaktır.

En iyi deneyim ve uyumluluk için size işletim sisteminizin en yeni sürümünü kullanmanızı öneriyoruz.

## Windows {#windows}

_QGroundControl_ Windows'un 64 bit versiyonlarına kurulabilir:

1. Download [QGroundControl-installer.exe](https://d176tv9ibo4jno.cloudfront.net/latest/QGroundControl-installer.exe).
2. exe'ye çift tıklayın.

:::info
Windows kurulum programı 3 kısayol oluşturur: **QGroundControl**, **GPU Compatibility Mode**, **GPU Safe Mode**.
Eğer başlatma veya video işleme sorunları yaşamıyorsanız ilk kısayolu kullanın.
For more information see [Troubleshooting QGC Setup > Windows: UI Rendering/Video Driver Issues](../troubleshooting/qgc_setup.md#opengl_troubleshooting).
:::

:::info
4.0'dan itibaren önceki _QGroundControl_ sürümleri sadece 64 bittir.
Manuel olarak 32 bit sürümler oluşturmak mümkündür (bu, geliştirici ekip tarafından desteklenmez).
:::

## Mac OS X {#macOS}

_QGroundControl_ can be installed on macOS 10.11 or later: <!-- match version using https://dev.qgroundcontrol.com/master/en/getting_started/#native-builds -->

<!-- match version using https://docs.qgroundcontrol.com/master/en/qgc-dev-guide/getting_started/#native-builds -->

<!-- usually based on Qt macOS dependency -->

1. Download [QGroundControl.dmg](https://d176tv9ibo4jno.cloudfront.net/latest/QGroundControl.dmg).
2. .dmg dosyasına çift tıklayın, ardından çıkan ekranda _QGroundControl_'ü _Application_ dosyasına sürükleyin.

::: info
QGroundControl continues to not be signed which causes problem on Catalina. To open QGC app for the first time:

- QGC uygulama ikonuna sağ tıklayın, menüden Aç'ı seçin. Karşınıza yalnızca İptal Et seçeneği çıkacaktır. İptal Et'i seçin.
- QGC uygulama ikonuna tekrar sağ tıklayın, menüden Aç'ı seçin. Bu sefer Aç seçeneği de size sunulacaktır.
  :::

## Ubuntu Linux {#ubuntu}

_QGroundControl_ can be installed/run on Ubuntu LTS 20.04 (and later).

Ubuntu, bir seri bağlantı noktasının (veya USB serisinin) robotikle ilgili kullanımına müdahale eden bir seri modem yöneticisi ile birlikte gelir.
_ QGroundControl _ 'ü kurmadan önce modem yöneticisini kaldırmalı ve seri bağlantı noktasına erişim için kendinize izin vermelisiniz.
Ayrıca video akışını desteklemek için _ GStreamer _ 'ı da yüklemeniz gerekmektedir.

QGroundControl \* 'ı ilk kez kurmadan önce:

1. On the command prompt enter:
   ```sh
   sudo usermod -a -G dialout $USER
   sudo apt-get remove modemmanager -y
   sudo apt install gstreamer1.0-plugins-bad gstreamer1.0-libav gstreamer1.0-gl -y
   sudo apt install libfuse2 -y
   sudo apt install libxcb-xinerama0 libxkbcommon-x11-0 libxcb-cursor-dev -y
   ```
   <!-- Note, remove install of libqt5gui5 https://github.com/mavlink/qgroundcontrol/issues/10176 fixed -->
2. Logout and login again to enable the change to user permissions.

&nbsp; _ QGroundControl _ yüklemek için:

1. Download [QGroundControl.AppImage](https://d176tv9ibo4jno.cloudfront.net/latest/QGroundControl.AppImage).
2. Install (and run) using the terminal commands:
   ```sh
   Aşağıdaki terminal komutlarını kullanarak kurun (ve çalıştırın):
      sh
      chmod +x ./QGroundControl.AppImage
      ./QGroundControl.AppImage (or double click)
   ```

:::info
There are known [video steaming issues](../troubleshooting/qgc_setup.md#dual_vga) on Ubuntu 18.04 systems with dual adaptors.
:::

:::info
4.0'dan itibaren önceki _QGroundControl_ sürümleri Ubuntu 16.04'te çalıştırılamaz.
Bu versiyonları Ubuntu 16.04'te çalıştırabilmek için [build QGroundControl from source without video libraries](https://dev.qgroundcontrol.com/en/getting_started/).
:::

## Android {#android}

- [Android 32 bit APK](https://qgroundcontrol.s3-us-west-2.amazonaws.com/latest/QGroundControl32.apk)
- [Android 64 bit APK](https://qgroundcontrol.s3-us-west-2.amazonaws.com/latest/QGroundControl64.apk)

## Old Stable Releases

Old stable releases can be found on <a href="https://github.com/mavlink/qgroundcontrol/releases/" target="_blank">GitHub</a>.

## Daily Builds

Daily builds can be [downloaded from here](../releases/daily_builds.md).
