# Güç Kurulumu

_Power Setup_ ekranı batarya parametrelerini düzenlemek için kullanılır ve ayrıca pervaneler hakkında gelişmiş ayarlar sunar.

![Batarya Kalibrasyonu](../../../assets/setup/px4_power.jpg)

## Batarya Voltaj/Akım Kalibrasyonu

Batarya/güç modülünüz için veri sayfasından verileri girin: hücre sayısı, hücre başına tam voltaj, hücre başına boş voltaj. Eğer varsa, voltaj bölücü ve volt başına amper bilgilerini de girin.

_QGroundControl_, ölçümlerden uygun voltaj bölücü ve volt başına amper değerlerini hesaplamak için kullanılabilir:

1. Bir multimetre kullanarak pilden gelen voltajı ölçün.
2. _Voltage divider_'ın yanındaki **Calculate** butonuna tıklayın. On the prompt that appears:
3. Ölçülen voltajı girin.
4. **Calculate**'e tıklayarak yeni bir voltaj bölücü değeri oluşturun.
5. Değeri ana forma kaydetmek için **Close**'a tıklayın.
6. Bataryadaki akımı ölçün.
7. _Amps per volt_'un yanındaki **Calculate** butonuna tıklayın. On the prompt that appears:
8. Ölçülen akımı girin.
9. **Calculate**'e tıklayarak yeni bir _volt başına akım_ değeri oluşturun.
10. Değeri ana forma kaydetmek için **Close**'a tıklayın.

## Gelişmiş Güç Ayarları

Gelişmiş güç ayarlarını özelleştirmek için **Show Advanced Settings** onay kutusuna tıklayın.

### Tam Yükte Voltaj Düşüşü

Bataryalar yüksek motor yüklemelerinde daha az voltaj gösterir. Motorlar boştayken ve tam kapasitede çalışırkenki volt farkını batarya hücrelerinin sayısına bölerek girin. Emin değilseniz varsayılan değer kullanılmalıdır!

:::warning
Eğer değer çok yüksekse batarya deep-discharged olabilir ve hasar görebilir.
:::

## ESC PWM Minimum ve Maksimum Kalibrasyonu

ESC'nin max/min PWM değerlerini kalibre etmek için:

1. Pervaneleri çıkarın.
2. Aracı QGC'ye USB (sadece) aracılığı ile bağlayın.
3. **Calibrate** butonuna basın.

::: warning
Never attempt ESC calibration with props on.

ESC kalibrasyonu sırasında motorlar dönmemelidir.
Bununla birlikte, bir ESC kalibrasyon sırasını doğru şekilde desteklemez/tespit etmezse, motoru maksimum hızda çalıştırarak PWM girişine yanıt verecektir.
:::

## Diğer Ayarlar

UAVCAN Veriyolu Yapılandırması ve motor indeksi ve yön ataması için ek ayarlara erişmek için **Show UAVCAN Settings** onay kutusunu seçin.
