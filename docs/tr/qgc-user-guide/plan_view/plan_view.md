# Plan Ekranı

_Plan View_, aracınız için _ otonom görevler _ planlamak ve onları araca yüklemek için kullanılır. Görev [planlanıp](#plan_mission) araca gönderildiğinde, görevi gerçekleştirmek için [Uçuş Ekranı](../fly_view/fly_view.md)'na geçillir.

Ayrıca eğer yazılım tarafından destekleniyorsa [GeoFence](plan_geofence.md) ve [Rally Points](plan_rally_points.md)'leri ayalarmak için kullanılır.

<span id="plan_screenshot"></span>
![Plan Ekranı](../../../assets/plan/plan_view_overview.jpg)

## Kullanıcı Arayüzü'ne Genel Bakış {#ui_overview}

Yukarıdaki [ ekran görüntüsü ](#plan_screenshot), [ Planlanan Ev ](#planned_home) konumundan (H) kalkışla başlayan basit bir görev planını gösterir, üç hedef noktadan geçer ve ardından son hedef noktaya (yani hedef noktası 3) iner.

Arayüzün temel elemanları şunlardır:

- **Map:** [ Planlanan Ev ](#planned_home) konumu dahil olmak üzere mevcut görev için numaralandırılmış konumları görüntüler.
  Noktaları seçmek için tıklayın (düzenlemek için) ya da konumlarını değiştirmek için sürükleyin.
- **Plan Araçları:** Önceki hedef noktaya göre halihazırda seçili olan hedef nokta için durum bilgisi ve tüm görevin istatistikleri (örn. Yatay mesafe ve görev süresi).
  - `Max telem dist`, [Planlanan Ev](#planned_home) konumu ile en uzak hedef nokta arasındaki mesafedir.
  - Bir cihaza bağlanıldığında bir **Upload** butonu da belirir ve planı araca yüklemek için kullanılabilir.
- **[Plan Araçları](#plan_tools):** Görevleri oluşturmak ve yönetmek çin kullanılır.
- **[Mission Command List/Overlay](#mission_command_list):** Mevcut görevin öğelerinin listesini görüntüler (öğeleri [düzenlemek](#mission_command_editors) için seçin).
- **Terrain Altitude Overlay:** Her görev komutunun göreceli yüksekliğini gösterir.

Size o anda seçili olan hedef noktasıyla ilgili bilgilerin yanı sıra tüm görevin istatistiklerini gösterir.

## Görev Planlama {#plan_mission}

At very high level, the steps to create a mission are:

1. Change to _Plan View_.
2. Göreve hedef noktalar veya komutlar ekleyin, gerektiği şekilde düzenleyin.
3. Upload the mission to the vehicle.
4. Change to _Fly View_ and fly the mission.

Aşağıdaki bölümler, ekrandaki bazı ayrıntıları açıklamaktadır.

## Planlanmış Ev Konumu {#planned_home}

The _Planned Home_ shown in _Plan View_ is used to set the approximate start point when planning a mission (i.e. when a vehicle may not even be connected to QGC).
QGC tarafından görev sürelerini tahmin etmek ve hedef noktalar arası çizgileri çizmek için kullanılır.

![Planlanmış Ev Konumu](../../../assets/plan/mission/mission_settings_planned_home.jpg)

Planlanan ev konumunu yaklaşık olarak kalkış yapmayı planladığınız konuma taşımanız/sürüklemeniz gerekir.
Planlanan ana konumun yüksekliği, [ Mission Settings ](#mission_settings) panelinde ayarlanır.

<img src="../../../assets/plan/mission/mission_settings_planned_home_position_section.jpg" style="width: 200px;"/>

:::tip
The Fly View displays the _actual_ home position set by the vehicle firmware when it arms (this is where the vehicle will return in Return/RTL mode).
:::

## Plan Araçları {#plan_tools}

Plan araçları, ara noktalar eklemek, karmaşık yerler için görev oluşturmayı kolaylaştırmak, görevleri yüklemek/indirmek/kaydetmek/geri yüklemek ve haritada gezinmek için kullanılır. Ana araçlar aşağıda açıklanmıştır.

:::info
**Center map**, **Zoom In**, **Zoom Out** araçlar kullanıcıların daha iyi görüntü almasına ve _Plan Ekranı_'ndaki haritada gezinmelerine yardımcı olur (araca gönderilen görev komutlarını etkilemezler).
:::

### Hedef Noktası Ekle

**Add Waypoint** aracına tıklayarak aktive edin. Aktifken haritaya tıklandığında, tıklanan noktaya yeni bir hedef konum eklenecektir.
Tekrar tıklayana kadar araç aktif kalacaktır.
Bir hedef nokta ekledikten sonra, konumunu değiştirmek için onu seçebilir ve sürükleyebilirsiniz.

### Dosya (Senkronizasyon) {#file}

Dosya araçları \*, görevleri yer istasyonu ile araç arasında taşımak ve bunları dosyalara kaydetmek/dosyalardan geri yüklemek için kullanılır.
Araçlar, araca göndermediğiniz görev değişiklikleri olduğunu belirtmek için bir \`!

:::info
Bir görevi gerçekleştirmeden önce görevi araca yüklemeniz gerekmektedir.
:::

_Dosya araçları_ aşağıdaki fonksiyonları sağlar:

- Yükle (Araca göndermek)
- İndir (Araçtan yüklemek)
- KML dosyası dahil olmak üzere Dosyaya Kaydet/Farklı Kaydet.
- Dosyadan Yükle
- Tümünü Kaldır (tüm görev hedef noktalarını _ Plan ekranından_ ve araçtan kaldırır)

### Şablon

[Pattern](Pattern.md) aracı, [gözlem](../plan_view/pattern_survey.md) ve [yapı taramaları](../plan_view/pattern_structure_scan_v2.md) da dahil olmak üzere karmaşık şekillerin uçulması için görevlerin oluşturulmasını basitleştirir.

## Görev Komutları Listesi {#mission_command_list}

Mission commands for the current mission are listed on the right side of the view.
En üstte görev, coğrafi sınır ve toparlanma noktaları arasında geçiş yapmak için bir dizi seçenek vardır.
Listede, değerlerini düzenlemek için görev öğelerini ayrı ayrı seçebilirsiniz.

![Görev Komutları Listesi](../../../assets/plan/mission/mission_command_list.jpg)

### Görev Komutları Düzenleyicisi {#mission_command_editors}

Düzenleyicisini görüntülemek için listedeki bir görev komutuna tıklayın (buradan komut özellikerini ayarlayabilir/değiştirebilirsiniz).

Komut adına tıklayarak komutun \*\* tipini \*\* değiştirebilirsiniz (örneğin: _Waypoint_).
Bu, aşağıda gösterilen _ Select Mission Command_ diyaloğunu görüntüler.
Varsayılan olarak bu sadece "Temel Komutlar" görüntülenir, daha fazlasını görüntülemek için \*\* Category\*\* açılır menüsünü kullanabilirsiniz (örneğin tüm seçenekleri görmek için \*\* All commands \*\* 'ı seçin).

<img src="../../../assets/plan/mission/mission_commands.jpg" style="width: 200px;"/>

Her komut adının sağında, _ Ekle _ ve _ Sil _ gibi ek seçeneklere erişmek için tıklayabileceğiniz bir menü bulunur.

:::info
Kullanılabilir komutların listesi aracın yazılımına ve türüne bağlıdır.
Örnek olarak şunlar verilebilir: Hedef nokta, Görüntü yakalamayı başlat, Öğeye atla (görevi tekrarlamak için) ve diğer komutlar.
:::

### Görev Ayarları {#mission_settings}

_Mission Start_ paneli [ görev komut listesinde ](#mission_command_list) görünen ilk öğedir.
Görevin başlangıcını veya sonunu etkileyebilecek bir takım varsayılan ayarı düzenlemek için kullanılabilir.

![Görev Komutları Listesi - Görev Ayarlarını Gösterme](../../../assets/plan/mission_start.png)

![Görev Ayarları](../../../assets/plan/mission/mission_settings.png)

#### Görevin Varsayılan Ayarları

##### Waypoint alt

Bir plana eklenen ilk görev öğesi için varsayılan irtifayı ayarlayın (sonraki öğeler, önceki öğeden ilk irtifayı alır).
Bu aynı zamanda bir plandaki tüm öğelerin yüksekliğni aynı değere ayarlamak için de kullanılabilir; planda öğeler varken eğer değeri değiştirirseniz bu seçenek size sorulacaktır.

##### Uçuş Hızı

Görev için varsayılan görev hızından farklı bir uçuş hızı belirleyin.

#### Görevin Sonu

##### Görev bittiğinde kalkış yerine dön

Aracınızın son görev öğesinden sonra Geri Dönmesini/RTL istiyorsanız bunu işaretleyin.

#### Planlanmış Ev Konumu

[Planned Home Position ](#planned_home) bölümü, bir görev planlarken aracın ev konumunu simüle etmenizi sağlar.
Bu, kalkıştan görevin tamamlanmasına kadar aracınızın hedef noktalar arası rotasını görüntülemenizi sağlar.

![Görev Ayarları Planlanmış Ev Konumu Bölümü](../../../assets/plan/mission/mission_settings_planned_home_position_section.jpg)

:::info
Bu yalnızca _ planlanan _ ev konumudur ve aracı çalıştırmayı planladığınız yere konumlandırılmalıdır.
Görevin gerçekleşmesinde gerçek bir etkisi yoktur.
Asıl ev konumu, araç tarafından devreye alınırken ayarlanır.
:::

This section allows you to set the **Altitude** and **Set Home to Map Centre** (you can move it to another position by dragging it on the map).

#### Kamera

Kamera bölümü, gerçekleştirilecek bir kamera eylemi belirlemenizi, gimbali kontrol etmenizi ve kameranızı fotoğraf veya video moduna ayarlamanızı sağlar.

![Görev Ayarları Kamera Bölümü](../../../assets/plan/mission/mission_settings_camera_section.jpg)

Mevcut kamera eylemleri şunlardır:

- Değişiklik yok (mevcut eyleme devam et)
- Fotoğraf çek (zaman aralıklı)
- Fotoğraf Çek (mesafe aralıklı)
- Fotoğraf çekimini durdur
- Video çekmeye başla
- Video çekimi durdur

#### Araç Bilgisi

Araç için uygun görev komutları, aracın yazılımına ve türüne bağlıdır.

Bir araca bağlıyken \* bir görev planlıyorsanız aracın yazılımı ve türü araçtan belirlenir.
Bu bölüm, bir araca bağlı değilken aracın donanımını yazılımını/türünü belirlemenize olanak tanır.

![Görev Ayarları Araç Bilgisi Bölümü](../../../assets/plan/mission/mission_settings_vehicle_info_section.jpg)

Bir görev planlarken belirtilebilecek ek değer, aracın uçuş hızıdır.
Bu değer belirtilerek, bir araca bağlı olmasa bile toplam görev veya anket süreleri yaklaşık olarak tahmin edilebilir.

## Sorun Giderme

### Görev (Plan) Yükleme/İndirme Hataları {#plan_transfer_fail}

Plan yükleme ve indirme, kötü bir iletişim bağlantısında hata verebilir (görevleri, coğrafi sınırları ve toparlanma noktalarını etkiler).
Bir arıza meydana gelirse, QGC kullanıcı arayüzünde aşağıdakine benzer bir durum mesajı görmelisiniz:

> (Mission transfer failed.) (Retry transfer.) (Error: Mission write mission count failed, maximum retries exceeded.)

Bağlantınız için kayıp oranı [ Settings View > MAVLink ](../settings_view/mavlink.md) 'de görüntülenebilir.
Kayıp oranı düşük tek haneli değerlerde olmalıdır (yani maksimum 2 veya 3):

- Yüksek tek haneli bir kayıp oranı, aralıklı arızalara neden olabilir.
- Daha yüksek kayıp oranları genellikle% 100 başarısızlığa neden olur.

Hatalar çok küçük bir ihtimalle QGC'deki ya da uçuş modlarındaki buglardan dolayı ortaya çıkabilir.
Bu olasılığı analiz etmek için, Plan yükleme/indirme için [ Console Logging ](../settings_view/console_logging.md) 'i etkinleştirebilir ve protokol mesaj trafiğini gözden geçirebilirsiniz.

## Daha Fazla Bilgi

- [QGC v3.2 sürümü](../releases/stable_v3.2_long.md#plan_view) için yeni plan ekranı özellikleri
- [QGC v3.3 sürümü](../releases/stable_v3.3_long.md#plan_view) için yeni plan ekranı özellikleri
