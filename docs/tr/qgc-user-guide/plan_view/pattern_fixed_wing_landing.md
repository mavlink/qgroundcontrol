# Sabit Kanat İniş Yolu (Plan Şablonu)

_Fixed Wing Landing Pattern_ aracı, göreve sabit kanat iniş yolu eklemenize olanak tanır.
Hem ArduPilot hem de PX4'de desteklenir.

![Sabit Kanat İniş Yolu](../../../assets/plan/pattern/fixed_wing_landing_pattern.jpg)

Yolun ilk noktası, belirli bir yükseklikte oyalanacağı yer; ikincisi de iniş noktasıdır.
Araç, ilk noktada hedeflenen yüksekliğe erişene kadar oyalanacaktır, ardından iniş için belirlenen iniş noktasına doğru alçalmaya başlayacaktır.

Hem oyalanma hem de iniş noktaları, istenilen yeni noktalara sürüklenebilir ve ilişkili görev öğesinden bir takım başka ayarlar yapılabilir.

## Creating a Landing Pattern

İniş yolu oluşturmak için:

1. Open [PlanView](../plan_view/plan_view.md) _Plan Tools_.

2. _Plan Tools_ 'dan _Plan Tools_'u açın ve _Fixed Wing Landing Pattern_'i seçin.

   ![Sabit Kanat İniş Yolu](../../../assets/plan/pattern/fixed_wing_landing_pattern_menu.jpg)

   Bu, görev listesine (sağda) _Landing Pattern_ öğesi ekleyecektir.

   ![Sabit Kanat İniş Yolu](../../../assets/plan/pattern/fixed_wing_landing_pattern_mission_item_initial.jpg)

3. Click on the map to create both the loiter point and the landing point.
   Bu noktalar harita üzerinde hareket ettirilebilir.

Ek ayarlar bir sonraki bölümde ele alınmıştır.

## Ayarlar

İniş yolu, ilişkili görev öğesinde (Plan Görünümü'nün sağ tarafındaki görev öğesi listesinde) daha da yapılandırılabilir.

### İniş noktası

_Loiter Point_ ayarları, oyalanmanın yüksekliğini, yarı çapını ve yönünü ayarlamak için kullanılır.

![İniş Yolu - Oyalanma Noktası](../../../assets/plan/pattern/fixed_wing_landing_pattern_settings_loiter.jpg)

Ayarlanabilir seçenekler şunlardır:

- **Altitude** - Oyalanma yüksekliği.
- **Radius** - Oyalanma yarıçapı.
- **Loiter clockwise** - Oyalanmanın yönünün saat yönü olması için işaretleyin (varsayılan olarak yön, saat yönünün tersidir).

### Oyalanma Noktası

_Landing Point_ ayarları, iniş pozisyonunu ve yolunu ayarlamak için kullanılır.

![İniş Yolu - İniş Noktası](../../../assets/plan/pattern/fixed_wing_landing_pattern_settings_landing.jpg)

Ayarlanabilir seçenekler şunlardır:

- **Heading** - Oyalanma noktasından iniş noktasına yönelme.
- **Altitude** - İniş noktası için yükseklik (nominal olarak sıfır).
- _Radyo Butonları_
  - **Landing Dist** - Distance between loiter point and landing point.
  - **Glide Slope** - Glide slope between loiter point and landing point.
- **Altitudes relative to home** - Tüm yüksekliklerin ev konumuna bağlı olması için işaretleyin (varsayılan olarak deniz seviyesidir).

## Uygulama

Bu şablon 3 gören öğresi oluşturur:

- `DO_LAND_START` - Eğer inişi iptal ederseniz araca `DO_GO_AROUND` komutunu gönderir ki bu komut aracın tekrar bu noktaya gelerek inmeye çalışmasına neden olur.
- `NAV_LOITER_TO_ALT` - İniş için başlangıç noktası
- `NAV_LAND` - İniş için bitiş noktası

Araç, yazılım tarafından `NAV_LOITER_TO_ALT` ve `NAV_LAND` noktaları arasında oluşturulan yolu kullanarak inişe geçer.

Eğer bu 2 konum aracın iniş kısıtlamalarını ihlal ederse (ör. alçalma açısı çok dikse), araca bu geçersiz görev yüklendikten sonra bir hata ortaya çıkacaktır.

:::info
PX4'te, yükleme esnasında iniş kısıtlamalarının ihlali yer istasyonuna bir hata mesajı gönderir, ve otopilot görevi başlatmayı reddeder (bütünlük kontrolünde başarısız olacağı için).
:::
