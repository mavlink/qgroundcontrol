# Video Overlay

When QGroundControl is recording a video stream to a file, it will also export a subtitle file with telemetry data that can be used to overlay the telemetry on the video during playback. Whichever telemetry values are selected for display in the telemetry [values widget](hud.md#instrument_panel) will also be exported to the overlay. Overlay değerleri 1Hz hızla güncellenecektir.

Seçilen değerler ekran kullanımını optimize etmek için 3 sütun halinde düzenlenmiştir.

## Live Overlay

**Telemetry Overlay** (under **Video Display** in the [video stream instrument page](camera_tools.md#video_instrument_page)
and in the video indicator's expanded page) shows the same values along the bottom of the video while you fly,
updated twice a second, and follows changes to the telemetry bar. It is off by default, and appears over the main
video stream or a USB camera. It isn't part of recordings, whose telemetry goes to the subtitle file.

## Oynatma

The [Video Recordings](../analyze_view/video_recordings.md) page in _Analyze Tools_ plays a recording with its
overlay. It can also be used with any player that [supports the SubStation Alpha](https://en.wikipedia.org/wiki/SubStation_Alpha#Players_and_renderers) subtitle format.
Çoğu oynatıcı videoyu oynatmayı denediğinizde iki dosyayı birden açacaktır. QGC tarafından oluşturuldukları gibi, iki dosyanında aynı dosyada aynı isimle olması gerekmektedir.

## Handbrake'i Kullanarak Kalıcı Video Altyazıları

Altyazılar [HandBrake](https://handbrake.fr/)'i kullanarak kalıcı olarak video dosyalarına eklenebilir.
Bu, altyazıları tüm oynatıcılar için kalıcı olarak görünür yapacaktır.

**HandBrake**'i açın, ana arayüzünü göreceksiniz.
**Open**'a tıklayın ve video dosyasını seçin.

Video dosyası yüklenirken, subtitles sekmesine geçin.
Altyazı dosyasını yüklemek için **Add**'e tıklayın.

**import SSA**'ı seçin ( [ASS](https://en.wikipedia.org/wiki/SubStation_Alpha#Advanced_SubStation_Alpha) SSA'nın bir uzantısıdır).

Videonuza karşılık gelen **.ass** dosyasını videonuza yükleyin ve **Burn into video**'u işaretleyin.

Yeni dosyanın nereye kaydedilmesini istediğinizi seçin ve **Start**'a tıklayın.

Bu, altyazıyı ve videoyu yeni bir dosyaya yazmaya başlayacaktır.
