# Gövde Kurulumu (ArduPilot)

Gövde kurulumu, sınıfı ve modeli aracınızla eşleşen gövde seçiminde kullanılır

> **Note** Gövde Kurulumu sadece *ArduCopter* ve *ArduSub* araçları için mevcuttur (*ArduPilot* arazi araçları ya da uçaklar için gösterilmez).

## Arducopter Gövde Kurulumu

Copter'den gövde seçmek için:

1. İlk olarak üstteki araç çubuğundan **dişli** simgesini (Vehicle Setup), daha sonra kenar çubuğundan **Airframe**'i seçin.
    
    ![Gövde Yapılandırması](../../../assets/setup/airframe/arducopter.jpg)

2. Aracınız için uygun olan *Frame Class* 'ı seçin:
    
    ![Gövde Modeli](../../../assets/setup/airframe/arducopter_class.jpg)
    
    > **Note** Sınıf değişikliklerinin geçerli olması için aracı yeniden başlatmanız gerekecektir.

3. Aracınız için spesifik *Frame Type* 'ını seçin:
    
    ![Gövde Modeli](../../../assets/setup/airframe/arducopter_type.jpg)

## ArduSub Gövde Kurulumu {#ardusub}

Sub için gövde modelini seçmek için:

1. İlk olarak üstteki araç çubuğundan **dişli** simgesini (Vehicle Setup), daha sonra kenar çubuğundan **Frame**'i seçin.
2. Aracınız için uygun olan gövde tipini seçin (bir gövde seçmek, seçimi uygular).
3. Tüm ** yeşil ** iticilerin ** saat yönünde ** pervanelere ve tüm ** mavi ** iticilerin dede ** saat yönünün tersine ** pervanelere sahip olduğundan emin olun (veya tersi).
    
    ![Gövde Modeli Seçme](../../../assets/setup/airframe_ardusub.jpg)

- ArduSub için varsayılan parametre setini yüklemek için ** Load Vehicle Default Parameters** 'ye de tıklayabilirsiniz.
    
    ![Araç parametrelerini yükle](../../../assets/setup/airframe_ardusub_parameters.jpg)