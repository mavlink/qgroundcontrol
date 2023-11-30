# Güç Kurulumu

*Power Setup* ekranı batarya parametrelerini düzenlemek için kullanılır ve ayrıca pervaneler hakkında gelişmiş ayarlar sunar.

![Batarya Kalibrasyonu](../../../assets/setup/PX4Power.jpg)

## Batarya Voltaj/Akım Kalibrasyonu

Batarya/güç modülünüz için veri sayfasından verileri girin: hücre sayısı, hücre başına tam voltaj, hücre başına boş voltaj. Eğer varsa, voltaj bölücü ve volt başına amper bilgilerini de girin.

*QGroundControl*, ölçümlerden uygun voltaj bölücü ve volt başına amper değerlerini hesaplamak için kullanılabilir:

1. Bir multimetre kullanarak pilden gelen voltajı ölçün.
2. *Voltage divider*'ın yanındaki **Calculate** butonuna tıklayın. Gelen pencerede: 
    1. Ölçülen voltajı girin.
    2. **Calculate**'e tıklayarak yeni bir voltaj bölücü değeri oluşturun.
    3. Değeri ana forma kaydetmek için **Close**'a tıklayın. 
3. Bataryadaki akımı ölçün.
4. *Amps per volt*'un yanındaki **Calculate** butonuna tıklayın. Gelen pencerede: 
    1. Ölçülen akımı girin.
    2. **Calculate**'e tıklayarak yeni bir *volt başına akım* değeri oluşturun.
    3. Değeri ana forma kaydetmek için **Close**'a tıklayın. 

## Gelişmiş Güç Ayarları

Gelişmiş güç ayarlarını özelleştirmek için **Show Advanced Settings** onay kutusuna tıklayın.

### Tam Yükte Voltaj Düşüşü

Bataryalar yüksek motor yüklemelerinde daha az voltaj gösterir. Motorlar boştayken ve tam kapasitede çalışırkenki volt farkını batarya hücrelerinin sayısına bölerek girin. Emin değilseniz varsayılan değer kullanılmalıdır!

> **Warning** Eğer değer çok yüksekse batarya deep-discharged olabilir ve hasar görebilir.

## ESC PWM Minimum ve Maksimum Kalibrasyonu

ESC'nin max/min PWM değerlerini kalibre etmek için:

1. Pervaneleri çıkarın. 
2. Aracı QGC'ye USB (sadece) aracılığı ile bağlayın. 
3. **Calibrate** butonuna basın.

> **Warning** Pervaneler takılıyken hiçbir zaman ESC kalibrasyonunu denemeyin.
> 
> ESC kalibrasyonu sırasında motorlar dönmemelidir. Bununla birlikte, bir ESC kalibrasyon sırasını doğru şekilde desteklemez/tespit etmezse, motoru maksimum hızda çalıştırarak PWM girişine yanıt verecektir.

## Diğer Ayarlar

UAVCAN Veriyolu Yapılandırması ve motor indeksi ve yön ataması için ek ayarlara erişmek için **Show UAVCAN Settings** onay kutusunu seçin.