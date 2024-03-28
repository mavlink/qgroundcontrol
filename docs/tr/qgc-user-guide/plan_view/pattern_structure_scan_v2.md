# Yapı Taraması (Plan Şablonu)

_Structure Scan_, çok köşeli (veya dairesel) zemin ayak izine sahip bir yapının _dikey yüzeyleri_ üzerinde (ör. duvarlar) görüntüler yakalayabilmek için ızgara şeklinde bir uçuş şablonu oluşturmanıza olanak sağlar.
Structure Scans are typically used for the visual inspection or creating 3D models of structures.

_Yapı Taramaları_, Plan ekranında **Pattern > Structure Scan** aracı kullanılarak bir görevin içine yerleştirilebilir.

:::info
_Structure Scan_'ın yeni versiyonu, eski _Structure Scan_ planlarını okuyamaz. Eskilerin tekar oluşturulmaları gerekir.
:::

:::warning
Bu özellik henüz ArduPilot yazılımı tarafından desteklenmemektedir.
PX4'de desteklenir.
:::

## Genel Bakış

Aşağıdaki resim, yapı taramasının bir ekran görüntüsünü göstermektedir.
Yeşil çokgen, yapının zemin ayak izini işaretlemek için kullanılırken, etrafındaki beyaz çizgi aracın uçuş yolunu gösterir.
Uçuş yolundaki yeşil numaralı daire, taramaya giriş/çıkış noktasıdır (taramanın başladığı yer).

![Yapı Taraması](../../../assets/plan/structure_scan_v2/structure_scan.jpg)

Tarama, yapıyı eşit olarak katmanlara ayırır; araç, yapının çevresinde belirli bir irtifada ve yapıya _ scan distance_'den uçar, ardından tüm yüzey taranana kadar işlemi her katmanda tekrarlar.

![Katman JPG](../../../assets/plan/structure_scan_v2/layers.jpg)

Kullanıcılar, yapının altındaki engellerden kaçınmak için _scan bottom altitude_ ve araç taramaya / taramaya giderken engellerden kaçınmak için _entrance/exit altitude_ ayarlayabilir.

## Creating a Scan

Tarama oluşturmak için:

1. **Plan View**'den **Pattern tool > Structure Scan**'ı seçin.

![Tarama JPG'si oluştur](../../../assets/plan/structure_scan_v2/create_scan.jpg)

1. Bu haritada basit kare bir yapı taraması oluşturacaktır.

   ![İlk Köşegen](../../../assets/plan/structure_scan_v2/initial_polygon_scan.jpg)

   Yeşil bölge, yapıyı kaplayacak şekilde düzenlenmelidir.

   - Haritadaki opak köşeleri yapının kenarlarına sürükleyin (yukarıda leylak rengi daire içine alınmış köşeler).
   - Yapının kapladığı alan basit bir kareden fazlaysa, yeni bir köşe noktası oluşturmak için köşeler arasındaki yarı saydam dairelere tıklayabilirsiniz.

2. Ayrıca merkezdeki "daire"ye (kırmızı ile işaretlenmiş) tıklayarak ve açılır menüden _Circle_ 'ı seçerek dairesel bir alana geçebilirsiniz.

   ![Dairesel Tarama](../../../assets/plan/structure_scan_v2/circle_scan.jpg).

   - Açılır menüden çokgen alana geri dönebilir ve taramanın yarıçapını ve/veya konumunu değiştirebilirsiniz.
   - Çemberin merkezini konumlandırmak için merkezdeki daireyi sürükleyin.

3. Geri kalan düzenlemeler, ekranın sağındaki _Structure Scan_ editörü kullanılarak halledilir.
   İlk olarak manuel tarama, belirli bir kamera kullanarak tarama veya özel bir kamera tanımı kullanarak tarama seçeneklerinden hangisini istediğinizi seçin.

   ::: info
   Modlar arasındaki temel fark, önceden tanımlanmış kameraların etkili bir katman yüksekliğini ve deklanşör mesafesini doğru bir şekilde hesaplamak için zaten ayarlanmış olmasıdır.
   :::

   Farklı modlar için seçenekler aşağıda gösterilmiştir.

   ![Yapı Taraması Editörü](../../../assets/plan/structure_scan_v2/editor_options.jpg)

Kullanıcı her zaman aşağıdaki ayarları yapılandırabilir:

- **Start scan from top/bottom:** Katmanların taranma yönü.
- \*\*Structure height:\*\*Taranan nesnenin yüksekliği.
- **Scan distance:** Distance from the structure to the flight path.
- **Entrance/Exit Alt:** Son/sonraki hedef noktası ile taranacak yapı arasındaki engellerden kaçınmak için bu ayarı kullanın.
  - Araç, bu irtifada _Entrance/Exit_ noktasına yükselecek ve ardından taramayı başlatmak için ilk katmana alçalacaktır.
  - Araç, taramayı tamamladıktan sonra bu irtifaya yükselecek ve ardından bir sonraki hedef noktaya geçecektir.
- \*\*Scan Bottom Alt:\*\*Yapının tabanı etrafındaki engellerden kaçınmak için bu ayarı kullanın.
  Bu ayar, yapının altını yerden yukarıda olacak şekilde ayarlar ve bu nedenle ilk tarama uçuşu yolunun yüksekliği (en alt katmanın yüksekliği) tarama istatistiklerinde _Bottom Layer Alt_ olarak gösterilir.
- **Rotate Entry Point:** Başlangıç/bitiş noktasını uçuş yolundaki bir sonraki köşeye/konuma taşıyın.

Kalan ayarlar _camera mode_'a bağlıdır:

- _Manuel Mod_ şunları ayarlamanıza olanak verir:
  - **Layer height:** Her katmanın yüksekliğini.
  - **Trigger Distance:** Her kamera çekimi arasındaki mesafe.
    Kamera sadece katman yolunda uçarken çekim yapar.
    It does not trigger images while transitioning from one layer to the next.
  - **Gimbal Pitch** -Tarama için kullanmak istediğiniz gimbal eğimi.

- _Bilinen/önceden tanımlanmış kameralar_ görüntü örtüşmesi için gerekli katman yüksekliklerini ve görüntü çekim aralıklarını otomatik olarak hesaplar ve tarama mesafesini değiştirmenize ve görüntü çözünürlüğü ayarlamanıza olanak tanır.
  It also ensures that the camera is pointed directly at the surface when it is capturing images (i.e. at a right angle rather than some tangent).
  Ayarlar şunlardır:

  - **Camera Orientation:** Dikey veya Yatay
  - _Örtüşme_:
    - **Front Lap:** Görüntünün yukardan aşağıya örtüşmesi.
    - **Side Lap:** Görüntü kenarlarda örtüşür.
  - **Scan distance:** Distance from the structure to the flight path.
  - **Ground Res:** Yüzey için gerekli görüntü çözünürlüğü/numune kalitesi.

- _Custom camera_ seçimi, kendi kamera özelliklerinizi girmenize olanak tanır, ancak aksi takdirde önceden tanımlanmış bir kamerayla aynı şekilde davranır.
