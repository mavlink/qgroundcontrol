# Uçuş Ekranı

Uçuş Ekranı, aracı uçarken izlemek ve araca komut vermek için kullanılır.

Şunları yapmak için kullanabilirsiniz:

- Otomatik şekilde [pre-flight checklist](#preflight_checklist) çalıştırın.
- Görevleri kontrol edin: [start](#start_mission), [continue](#continue_mission), [pause](#pause), ve [resume](#resume_mission).
- Aracı [arm](#arm)/[disarm](#disarm)/[emergency stop](#emergency_stop), [takeoff](#takeoff)/[land](#land), [change altitude](#change_altitude), belirli bir konuma [go to](#goto) veya [orbit](#orbit), ve [return/RTL](#rtl) için yönlendirin.
- Harita görünümü ile video görünümü (eğer varsa) arasında geçiş yapın
- Mevcut araç için video, görev, telemetri ve diğer bilgileri görüntüleyin ve ayrıca bağlı araçlar arasında geçiş yapın.

![Uçuş Ekranı](../../../assets/fly/fly_view_overview.jpg)

## İzleme

Yukarıdaki ekran görüntüsü, uçuş ekranının ana öğelerini gösterir:

- **Harita:** Bağlı tüm araçların pozisyonlarını ve mevcut aracın görevini gösterir.
  - Haritada gezinmek için haritayı sürükleyebilirsiniz (harita belirli bir süre sonra otomatik olarak yeniden ortalanır).
  - Kalkıştan sonra, haritaya tıklayarak bir konuma [Go to](#goto) veya [Orbit at](#orbit) ayarlayabilirsiniz.
- **Uçuş Araç Çubuğu:** Sensörler (GPS, pil, RC kontrolü) ve araç durumu (Uçuş modu, Etkin (armed) / Devre Dışı (disarmed) durumu) hakkında anahtar bilgiler.
  - Daha fazla ayrıntı görmek için sensör göstergelerini seçin.
  - Yeni bir mod seçmek için _ Flight mode _ metnine (ör. "Hold") tıklayın. Tüm modlar mevcut olmayabilir.
  - Aracın uçuşa hazır olma durumunu değiştirmek için _Armed/Disarmed_ metnine tıklayın. While flying you can press this text for _Emergency Stop_.
- **Uçuş araçları:** Şunları yapmak için kullanabilirsiniz:
  - Kalkış/iniş arasında geçiş yapın.
  - Mevcut işlemi durdur/tekrar başlat (ör. iniş, ya da görev).
  - Güvenli geri dönüş (RTL ya da Return olarakta bilinir).
  - _Action_ butonu mevcut durum için diğer uygun seçenekleri sunar (bunlar _Confirmation Slider_'ın üstünde gözükür). İşlemler, yüksekliği değiştirmeyi veya bir göreve devam etmeyi içerir.
  - [preflight checklist](#preflight_checklist)'i etkinleştirin (varsayılan olarak devre dışıdır).
- **[Bilgi Paneli](#instrument_panel):** Telemetri, kamera, video, sistem durumu ve titreşim dahil olmak üzere araç bilgilerini görüntüleyebileceğiniz çok sekmeli widget.
- **[Video/Harita](#video_switcher):** Bir pencerede video ile harita arasında geçiş yapın.
  - _Video_ ya da _Map_'i ön plana almak için öne almak istediğinize tıklayın.
  - _QGroundControl_, aracınızın UDP bağlantısı üzerinden RTP ve RTSP video yayını yapmanızı destekler. It also supports directly connected UVC devices. QGC'nin video desteği hakkında daha ayrıntılı bilgiyi [Video README](https://github.com/mavlink/qgroundcontrol/blob/master/src/VideoStreaming/README.md)'de bulabilirsiniz.
  - Bir [Telemetry Overlay](../fly_view/video_overlay.md) dosyası otomatik olarak oluşturulacaktır
- **Kaydırmalı Onay Butonu:** İstenen işlemi gerçekleştirmek için onay butonu. Operasyonu başlatmak için kaydırın. İptal etmek için **X** 'e basın.

Ayrıca varsayılan olarak görüntülenmeyen/ belirli koşullarda görüntülenebilen bazı öğelerde var. Mesela, çoklu-araç seçici sadece birden çok aracınız varsa veya uçuş öncesi kontrol listesi eğer etkinleştirilmişse görüntülenir.

## Bilgi Paneli {#instrument_panel}

Bilgi Paneli, telemetri, kamera, video, sistem durumu ve titreşim bilgileri dahil olmak üzere mevcut araç hakkında bilgileri görüntüleyebileceğiniz çok sekmeli bir widgettır.

The default page displays vehicle telemetry - use the drop down menu on the top right to select the other options.

### Değerler (Telemetri)

Değerler sayfası telemetri bilgilerini gösterir; varsayılan olarak rakım (ev konumuna göre) ve yer hızı.

![Bilgi Paneli - değerler ve telemetri için](../../../assets/fly/instrument_page_values.jpg)

Panelin sol üst köşesindeki küçük dişli simgesine basarak hangi bilgilerin görüntüleneceğini ayarlayabilirsiniz. Her değeri ister normal isterse "büyük" boyutta görüntülenebilir (normal boyutta sayfadaki her satırda 2 değer varken, büyük boyutta sadece 1 değer olur).

![Bilgi Paneli - değerlerin ayarları](../../../assets/fly/instrument_page_values_settings.jpg)

### Kamera {#camera_instrument_page}

Kamera sayfası, kamerayı ayarlamak ve kontrol etmek için kullanılır. Uçuş kontrolcüsüne direkt bağlı bir kamera için sadece kamerayı başlatma ayarı vardır:

![Bilgi Paneli - Kamera için](../../../assets/fly/instrument_page_camera.jpg)

[MAVLink Camera Protocol](https://mavlink.io/en/services/camera.html)'ü destekleyen bir kameraya bağlandığınızda ek olarak geçerli hale gelen diğer kamera özelliklerini ayarlayıp kullanabilirsiniz. Örnek olarak, eğer kameranız video modunu destekliyorsa, fotoğraf ve video modları arasında geçiş yapabilir, kaydı başlatıp/durdurabilirsiniz.

![Bilgi Paneli - Kameranın MAVLink ayarları](../../../assets/fly/instrument_page_camera_mavlink.jpg)

Gelişmiş ayarlara sayfanın sol üstündeki dişliye tıklayarak ulaşabilirsiniz.

![Bilgi Paneli - Kameranın MAVLink ayarları](../../../assets/fly/instrument_page_camera_mavlink_settings.jpg)

::: info
Gösterilen ayarların çoğu kameraya bağlıdır (ayarlar [MAVLink Camera Definition File](https://mavlink.io/en/services/camera_def.html)'da tanımlanmıştır). Sondaki bir kaç ortak ayar sabit olarak kodlanmıştır: Fotoğraf Modu (Tekli/Photolapse), Fotoğraf Aralığı (Timelapse için), Kamera Ayarlarını Varsayılana Sıfırla (kameraya bir sıfırlama komutu gönderir), Format (depolama)
:::

### Video Akışı {#video_instrument_page}

Video sayfası video akışını etkinleştirmek ve devre dışı bırakmak için kullanılır. Etkinleştirildiğinde, video akışını durdurup başlatabilir, ızgara çizgilerini açabilir, görüntünün ekrana nasıl sığacağını değiştirebilir ve videoyu yerel olarak QGC ile kaydedebilirsiniz.

![Bilgi Paneli - Video Akışı](../../../assets/fly/instrument_page_video_stream.jpg)

### Durum

Durum sayfası aracınızdaki sistemin durumunu gösterir. _QGroundControl_ will switch to this page automatically if any system changes to unhealthy.

![Bilgi Paneli - Cihazın Durumu İyi](../../../assets/fly/instrument_page_health_good.jpg) ![Bilgi Paneli - Cihazın Durumu Kötü](../../../assets/fly/instrument_page_health_bad.jpg)

### Titreşim

Titreşim sayfası mevcut titreşim seviyelerini ve klips sayılarını gösterir.

![Bilgi Paneli - Titreşim Klipsleri](../../../assets/fly/instrument_page_vibration.jpg)

## İşlemler/Görevler

Aşağıdaki bölümler, Uçuş Ekranı'nda genel işlemlerin / görevlerin nasıl gerçekleştirileceğini açıklamaktadır.

::: info
Mevcut seçeneklerin çoğu, hem aracın türüne hem de mevcut durumuna bağlıdır.
:::

### Uçuş Öncesi Kontrol Listesi {#preflight_checklist}

Aracın doğru ayarlandığını ve uçmak için güvenli olduğu gösteren standart kontrollerin yapılması için otomatik bir uçuş öncesi kontrol listesi kullanılabilir.

To view the checklist, first enable the tool by navigating to [Application Settings > General > Fly View](../settings_view/general.md) and selecting the **Use preflight checklist** checkbox. Liste _Flight Tools_'a eklenecektir. Kontrol listesini oradan açabilirsiniz:

![Uçuş Öncesi Kontrol Listesi](../../../assets/fly/pre_flight_checklist.jpg)

Once you have performed each test, select it on the UI to mark it as complete.

### Aracı Devreye Alma {#arm}

::: tip
Genel olarak _ QGroundControl _ aracı manuel olarak devreye almanızı gerektirmez; Bir göreve veya kalkışa başlarsanız bu sizin için yapılır.
:::

Bir aracı devreye almak, kalkışa hazırlık için motorları çalıştırır.

Aracı devreye almak için, _Fly Toolbar_'dan **Disarmed**'ı seçin ve kaydırmalı onay butonuyla onaylayın.

![Aracı Devreye Alma](../../../assets/fly/arm.jpg)

::: info
Eğer bir kaç saniye içinde kalkmazlarsa araçlar genellikle otomatik olarak kendilerini devre dışı bırakırlar.
:::

### Aracı Devre Dışı Bırakma {#disarm}

Aracı devre dışı bırakmak motorları durdurur (aracı güvenli hale getirir). Aracı devre dışı bırakmak için araç **landed** halindeyken _Fly Toolbar_ 'dan **Armed**'ı seçin.

![Aracı Devre Dışı Bırakma](../../../assets/fly/disarm.jpg)

::: info
Aracı uçarken devre dışı bırakmak [Emergency Stop](#emergency_stop) olarak adlandırılır
:::

### Acil Durdurma {#emergency_stop}

Emergency stop is effectively the same as disarming the vehicle while it is flying. Aracınız çarpacaktır/çakılacaktır!

Aracı devre dışı bırakmak için uçuş sırasında _Fly Toolbar_'dan **Armed**'ı seçin.

![Acil Durdurma](../../../assets/fly/emergency_stop.jpg)

### Kalkış {#takeoff}

::: tip
If you are starting a mission for a multicopter, _QGroundControl_ will automatically perform the takeoff step.
:::

Kalkmak için (iniş yapıldığında):

1. _Fly Tools_'dan **Takeoff** butonuna basın (kalkıştan sonra bu buton **Land** butonu olarak değişecektir).
2. İsterseniz kalkış yüksekliğini sağda bulunan dikey kaydırıcıyla ayarlayabilirsiniz.
3. Kaydırmalı onay butonunu kaydırarak kalkışı onaylayın.

![kalkış](../../../assets/fly/takeoff.jpg)

### İniş {#land}

Uçuş sırasında istediğiniz zaman mevcut konuma inebilirsiniz:

1. _Fly Tools_'dan **Land** butonuna basın (kalkıştan sonra bu buton **Takeoff** butonu olarak değişecektir).
2. Kaydırmalı onay butonunu kaydırarak inişi onaylayın.

![iniş](../../../assets/fly/land.jpg)

### RTL/Geri Dönüş

Return to a "safe point" at any time while flying:

1. _Fly Tools_'dan **RTL** butonuna basın.
2. Kaydırmalı onay butonunu kaydırarak RTL'i onaylayın.

![rtl](../../../assets/fly/rtl.jpg)

::: info
Vehicles commonly return to the "home" (takeoff) location and land. This behaviour depends on the vehicle type and configuration. For example, rally points or mission landings may be used as alternative return targets.
:::

### Yükleklik Değiştirme {#change_altitude}

Uçuş esnasında eğer bir görevde değilse aracın yükseliği değiştirilebilir:

1. _Fly Tools_'dan **Action** butonuna basın
2. İletişim kutusundan _Change Altitude_'ı seçin.

![Göreve Devam Etme/Yüksekliği Değiştirme ](../../../assets/fly/continue_mission_change_altitude_action.jpg)

3. Dikey kaydırıcıyla istenilen yüksekliği ayarlayın, ardından kaydırmalı onay butonu ile onaylayın.

![Yüksekliği Değiştirme](../../../assets/fly/change_altitude.jpg)

### Hedef Konum {#goto}

Kalkıştan sonra belirli bir konuma uçmayı ayarlayabilirsiniz.

1. Left click/Press on the map where you want the vehicle to move and select **Go to location** on the popup.

![Hedefe gitme veya yörünge](../../../assets/fly/goto_or_orbit.jpg)

2. Konum harita üzerinde kaydırmalı onay butonuyla birlikte gözükecektir.

![Hedefe gitme onayı](../../../assets/fly/goto.jpg)

3. Hazır olduğunuzda kaydırarak onay verin (ya da **X**'e basarak iptal edin).

::: info
Hedef konum aracın 1 km çevresinde seçilmelidir (QGC'de sabit olarak ayarlanmıştır).
:::

### Yörünge Konumu {#orbit}

Kalkıştan sonra belirli bir konumun çevresinde yörüngede dolaşmasını ayarlayabilirsiniz.

1. Left click/Press on the map (near the centre of your desired orbit) and select **Orbit at location** on the popup.

![Hedefe gitme veya yörünge](../../../assets/fly/goto_or_orbit.jpg)

2. Önerile yörünge kaydırmalı onay butonu ile harita üzerinde belirecektir.

![Yörünge Onayı](../../../assets/fly/orbit.jpg)

- Yörüngenin konumunu değiştirmek için merkezini seçin ve kaydırın.
- Yörüngenin yarıçapını, çemberin dışındaki noktayı seçip sürükleyerek ayarlayabilirsiniz.

3. Hazır olduğunuzda kaydırarak onay verin (ya da **X**'e basarak iptal edin).

### Duraklatma

You can pause most operations, including taking off, landing, RTL, mission execution, orbit at location. Duraklatıldığında aracın davranışı aracın tipine bağlıdır, genellikle multikopterler havada asılı kalırken sabit kanatlar çember çizer.

::: info
Bir _Goto location_ operasyonunu durduramazsınız.
:::

Durdurmak için:

1. _Fly Tools_'dan **Pause** butonuna basın.
2. İsterseniz yeni bir yüksekliği sağda bulunan dikey kaydırıcıyla ayarlayabilirsiniz.
3. Kaydırmalı onay butonunu kaydırarak durdurmayı onaylayın.

![duraklatma](../../../assets/fly/pause.jpg)

### Görevler

#### Görevi Başlatma {#start_mission}

Araç inmiş durumdayken bir görevi başlatabilirsiniz (görevi başlatma onay kaydıracı varsayılan olarak çoğu kez gösterilir).

Yerdeki bir aracın görevini başlatmak için:

1. _Fly Tools_'dan **Action** butonuna basın
2. İletişim kutusundan _Start Mission_'ı seçin.

![Görevi Başlatma Eylemi](../../../assets/fly/start_mission_action.jpg)

      (kaydırmalı onay çubuğunu görüntülemek için)


3. Onay kaydıracı belirdiğinde görevi başlatmak için kaydırın.

![Görevi Başlatma](../../../assets/fly/start_mission.jpg)

#### Göreve Devam Etme (Continue Mission) {#continue_mission}

Göreve _sıradaki_ hedef noktanızdan _devam_ edebilirsiniz (_Continue Mission_ kaydırmalı onay butonu kalkıştan sonra çoğu kez varsayılan olarak gösterilir).

::: info
Continue ve [Resume mission](#resume_mission) farklıdır! Continue komutu durdurulmuş bir görevi tekrar başlatmak veya zaten kalkmış bir araç için yani kalkış görevi komutunu kaçırmış durumdaysanız kullanılır. Resume mission is used when you've used a RTL or landed midway through a mission (e.g. for a battery change) and then wish to continue the next mission item (i.e. it takes you to where you were up to in the mission, rather than continuing from your place in the mission).
:::

Görevinize aşağıdakileri yaparak devam edebilirsiniz (hali hazırda bir görevde değilseniz!):

1. _Fly Tools_'dan **Action** butonuna basın
2. İletişim kutusundan _Continue Mission_'ı seçin.

![Göreve Devam Etme/Yüksekliği Değiştirme](../../../assets/fly/continue_mission_change_altitude_action.jpg)

3. Kaydırmalı onay butonunu kaydırarak göreve devam edin.

![Göreve Devam Etme (Continue Mission)](../../../assets/fly/continue_mission.jpg)

#### Göreve Devam Etme (Resume Mission) {#resume_mission}

_Resume Mission_, bir görevin içinde [RTL/Return](#rtl) ya da [Land](#land) gerçekleştirdikten sonra (ör. batarya değiştirmek için) kullanılır.

::: info
Eğer bataryayı değiştiricekseniz, bataryanın bağlantısını kestikten sonra QGC ile aracın bağlantısını **kesmeyin**. Yeni bataryayı yerleştirdikten sonra _QGroundControl_ cihazı tekrar tespit edip, bağlantıyı otomatik olarak yeniden kuracaktır.
:::

İnişten sonra size görev planını cihazdan kaldırma, cihazda bırakma ya da göreve kaldığı yerden devam etme seçeneklerini sunan bir _Flight Plan complete_ iletişim kutusu karşınıza çıkacaktır.

![Göreve Devam Etme (Resume Mission)](../../../assets/fly/resume_mission.jpg)

Eğer göreve devam etmeyi düşünüyorsanız, _QGroundControl_ görevi yapılandırıp araca yükleyecektir. Ardından _Start Mission_ kaydıracıyla göreve devam edebilirsiniz.

Aşağıdaki görüntü inişten sonra yeniden yapılandırılan bir görevi göstermektedir.

![Yapılandırılmış Göreve Devam Etme](../../../assets/fly/resume_mission_rebuilt.jpg)

::: info
Bir görev, basit bir şekilde aracın son gerçekleştirdiği görev aşamasından devam edemez, çünkü son yapılan aşamada görevin sonraki aşamalarına etki edebilecek birden fazla etken olabilir (ör. hız komutları ya da kameranın kontrol komutları). Basit bir şekilde devam etmek yerine, _QGroundControl_ son gerçekleştirilen görev aşamasından başlayarak, ilgili komutları da dikkate alarak görevi tekrar yapılandırır.
:::

#### İnişten Sonra Görevi Kaldırma {#resume_mission_prompt}

Görev bittikten sonra aracın inişi ve devre dışı bırakılmasının ardından görevi araçtan kaldırmanız istenecektir. Bunun amacı farkında olmadan araçta bırakılan görevlerin potansiyel olarak beklenmeyen davranışlar sergilemesinin önüne geçmektir.

### Videoyu Görüntüleme {#video_switcher}

Video akışı etkinleştirildiğinde, _QGroundControl_ haritanın sol altında bulunan "videoya geçiş penceresi"nde videoyu göstermeye başlıyacaktır. You can press the switcher anywhere to toggle _Video_ and _Map_ to foreground (in the image below, the video is shown in the foreground).

![Video Akışı Kaydı](../../../assets/fly/video_record.jpg)

::: info
video akışı [Application Settings > General tab > Video](../settings_view/general.md#video)'dan etkinleştirilebilir/düzenlenebilir.
:::

Video görüntüsünü geçiş penceresindeki kontrolleri kullanararak daha da düzenleyebilirsiniz:

    ![Video Pop](../../../assets/fly/video_pop.jpg)

- Resize the switcher by dragging the icon in the top right corner.
- Geçiş penceresini sol alt köşede bulunan ikona basarak gizleyebilirsiniz.
- Sol üst köşedeki ikona basarak video geçiş pencersini ayırabilirsiniz. (ayrıldığında, işletim sisteminizdeki her hangi bir pencere gibi hareket ettirip tekrar boyutlandırabilirsiniz). Eğer ayrılan pencereyi kapatırsanız, geçiş penceresi tekrardan QGC uçuş görünüme sabitlenecektir.

### Video Kaydı

Eğer araç ve kamera tarafından destekleniyorsa, _QGroundControl_ kendiliğinden video kaydını başlatıp durdurabilirsiniz. Ayrıca _QGroundControl_ videoyu yayınlayıp, yerel dosyalara kaydedebilir.

::: tip
Kameranın kendisinde kaydedilen video daha kaliteli olacak olsa da, yüksek ihtimalle yer istasyonunun daha büyük kayıt kapasitesi olacaktır.
:::

#### Video Akışını Kaydetme (GCS'de)

Video akışı kaydı [video stream instrument page](#video_instrument_page)'dan kontrol edilir. Yeni bir video kaydı başlatmak için kırmızı daireye tıklayın (daireye her tıklanıldığında yeni bir video dosyası oluşturulur); kayıt devam ederken çember kırmızı bir kareye dönüşecektir.

![Video Akışı Kaydı](../../../assets/fly/video_record.jpg)

Video akışı kaydı [Application Settings > General tab](../settings_view/general.md)'dan düzenlenir:

- [Video Recording](../settings_view/general.md#video-recording) - kayıt dosyası formatını ve depolama sınırlarını belirtir.

  ::: info
  Videolar varsayılan olarak Matroska format (.mkv) olarak kaydedilir. Bu format, hata durumunda bozulmaya karşı nispeten dayanıklıdır.
  :::

- [Miscellaneous](../settings_view/general.md#miscellaneous) - Video **Application Load/Save Path**'e kaydedilir.

::: tip
Kaydedilen video, sadece video akışının kendisini içerir. QGroundControl uygulamasının öğeleriyle videoyu kaydetmek için başka bir ekran kaydı uygulaması kullanmanız gerekir.
:::

#### Videoyu Kamerada Kaydetme

_Kameranın kendisinde_ video kaydını durdurup/başlatmak için [camera instrument page](#camera_instrument_page)'i kullanabilirsiniz. İlk olarak video moduna geçiş yapın, ardından kırmızı butona basarak kaydı başlatabilirsiniz.

![Bilgi Paneli - Kameranın MAVLink ayarları](../../../assets/fly/instrument_page_camera_mavlink.jpg)
