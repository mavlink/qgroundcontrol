# Motorların Kurulumu (ArduSub)

ArduSub'un düzgün çalışması için motorların doğru şekilde kurulması gerekir.

ROV'nuzu yeni monte ettiyseniz, önce \*\* Manuel Test \*\* bölümünde iticilerin doğru çıkışlara bağlandığından emin olun.
Her kaydırıcıyı sürükleyin ve görüntülenen gövdeye göre _ doğru motorun _ döndüğünden emin olun.

İticilerinin uygun çıkışlara bağlandığından emin olduktan sonra,_ doğru yönü _ (ileri / geri) kontrol etmek için [ otomatik yön algılama ](#automatic) (ArduSub 4.0'dan iibaren tavsiye edilir) veya [ manuel test](#manual) seçeneklerinden birini kullanabilirsiniz.

:::info
\*\* Note \*\* [ Manuel Test ](#manual) ArduSub tarafından 3.5 sürümüne kadar desteklenirken, ArduSub 4.0 hem [ Manuel Testi ](#manual) hem de [ otomatik yön algılamayı ](#automatic) destekler.
:::

## Manuel Test {#manual}

ArduSub motor kurulumu, motorları teker teker test etmenize olanak sağlar.
Kaydırıcılar, her motorun ileri veya geri modda döndürülmesine izin verir ve kaydırıcıların altındaki onay kutuları, tek tek iticilerin çalışmasını tersine çevrilmesine olanak tanır.

Sağdaki resim, her bir iticinin konumu ve yönü ile birlikte şu anda kullanımda olan gövdeyi gösterir.
Eğer gövde seçimi aracınıza uymuyorsa, önce [Frame ](../setup_view/airframe_ardupilot.md#ardusub) sekmesinden doğru gövdeyi seçin.

Motorları manuel olarak kurmak ve test etmek için sayfadaki talimatları okuyun ve uygulayın.

:::warning
Aracı devreye almak ve testi etkinleştirmek için anahtarı kaydırmadan önce motorların ve pervanelerin engellerden uzak olduğundan emin olun!
:::

![Ardusub Motorların Testi](../../../assets/setup/motors-sub.jpg)

## Otomatik Yön Algılama {#automatic}

Ardusub 4.0 ve daha yeni sürümler, motor yönlerinin otomatik olarak algılanmasını destekler.
Bu, her motora sinyal göndererek, gövdenin beklendiği gibi tepki verip vermediğini kontrol ederek ve gerekirse motoru ters çevirerek çalışır.
Bu işlem yaklaşık bir dakika sürer.

Otomatik motor yönü algılamasını gerçekleştirmek için \*\*Vehicle Setup->Motors \*\* sekmesine gidin, \*\* Auto-Detect Directions \*\* düğmesine tıklayın ve bekleyin.
İşlemle ilgili ek çıktı, işlem gerçekleşirken düğmenin yanında gösterilecektir.

:::warning
Bu prosedür hala motorların gövde görünümünde gösterildiği gibi _ doğru çıkışlara _ bağlanmasını gerektirir!
:::

![Ardusub Motorların Otomatik Kurulumu](../../../assets/setup/motors-sub-auto.jpg)
