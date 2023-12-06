# Gövde Kurulumu (ArduPilot)

Gövde kurulumu, sınıfı ve modeli aracınızla eşleşen gövde seçiminde kullanılır

:::info
Gövde Kurulumu sadece _ArduCopter_ ve _ArduSub_ araçları için mevcuttur (_ArduPilot_ arazi araçları ya da uçaklar için gösterilmez).
:::

## Arducopter Gövde Kurulumu

Copter'den gövde seçmek için:

1. İlk olarak üstteki araç çubuğundan **dişli** simgesini (Vehicle Setup), daha sonra kenar çubuğundan **Airframe**'i seçin.

   ![Gövde Yapılandırması](../../../assets/setup/airframe/arducopter.jpg)

2. Aracınız için uygun olan _Frame Class_ 'ı seçin:

   ![Gövde Modeli](../../../assets/setup/airframe/arducopter_class.jpg)

   ::: info
   Sınıf değişikliklerinin geçerli olması için aracı yeniden başlatmanız gerekecektir.
   :::

3. Aracınız için spesifik _Frame Type_ 'ını seçin:
   ![Gövde Modeli](../../../assets/setup/airframe/arducopter_type.jpg)

   ![Gövde Modeli Seçme](../../../assets/setup/airframe_ardusub.jpg)

## ArduSub Gövde Kurulumu {#ardusub}

Sub için gövde modelini seçmek için:

1. İlk olarak üstteki araç çubuğundan **dişli** simgesini (Vehicle Setup), daha sonra kenar çubuğundan **Frame**'i seçin.
2. Aracınız için uygun olan gövde tipini seçin (bir gövde seçmek, seçimi uygular).
3. Tüm \*\* yeşil \*\* iticilerin \*\* saat yönünde \*\* pervanelere ve tüm \*\* mavi \*\* iticilerin dede \*\* saat yönünün tersine \*\* pervanelere sahip olduğundan emin olun (veya tersi).

   ![Select airframe type](../../../assets/setup/airframe_ardusub.jpg)

   - ArduSub için varsayılan parametre setini yüklemek için \*\* Load Vehicle Default Parameters\*\* 'ye de tıklayabilirsiniz.

     ![Araç parametrelerini yükle](../../../assets/setup/airframe_ardusub_parameters.jpg)
