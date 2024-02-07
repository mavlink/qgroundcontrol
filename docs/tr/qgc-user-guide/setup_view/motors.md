# Motor Kurulumu

Motor Kurulumu, ayrı motorları / servoları test etmek için kullanılır (örneğin, motorların doğru yönde döndüğünü doğrulamak için).

:::tip
Bu talimatlar PX4 ve ArduPilot'taki çoğu araç türü için geçerlidir.
Araca özel talimatlar, alt konular olarak sağlanır (örn. [ Motor Kurulumu (ArduSub) ](../setup_view/motors_ardusub.md)).
:::

![Motorların Testi](../../../assets/setup/Motors.png)

## Test Adımları

Motorları test etmek için:

1. Tüm pervaneleri çıkarın.

   > \*\* Uyarı \*\* Pervaneleri, motorları etkinleştirmeden önce çıkarmalısınız!
   > :::

2. (_ yalnızca PX4 _) güvenlik anahtarını etkinleştirin - varsa.

3. Motor kaydırıcılarını etkinleştirmek için onaylayıcıyı (_ Pervaneler çıkarıldı - Motor kaydırıcılarını etkinleştirin _) kaydırın.

4. Motorları döndürmek ve doğru yönde döndüklerini doğrulamak için tek tek kaydırıcıları ayarlayın.

   > \*\* Not \*\* Motorlar yalnızca kaydırıcıyı bıraktıktan sonra döner ve 3 saniye sonra otomatik olarak dönmeyi durdurur.
   > :::

## Ek Bilgi

- [Basic Configuration > Motor Setup](http://docs.px4.io/master/en/config/motors.html) (_PX4 User Guide_) -Buradan, PX4 için ek bilgiler bulabilirsiniz.
