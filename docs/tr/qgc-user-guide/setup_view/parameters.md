# Parametreler

_Parametreler_ ekranı, araçla ilişkili parametrelerden herhangi birini bulmanıza ve düzenlemenizi sağlar.

![Parametreler Ekranı](../../../assets/setup/parameters_px4.jpg)

:::info
PX4 Pro ve ArduPilot farklı parametre setleri kullanır, ancak her ikisi de bu bölümde açıklandığı gibi yönetilir.
:::

## Bir Parametreyi Bulma

Parametreler gruplar halinde düzenlenmiştir. Soldaki butonlara tıklayarak görüntülemek için bir parametre grubu seçin (yukarıdaki görüntüde _ Pil Kalibrasyonu _ grubu seçilir).

Ayrıca _ Search_ alanına bir terim girerek bir parametre için _ arama _ yapabilirsiniz. Bu size girilen alt diziyi içeren tüm parametre adlarının ve açıklamalarının bir listesini gösterecektir (aramayı sıfırlamak için \*\* Clear \*\* tuşuna basın).

![Parametreler Araması](../../../assets/setup/parameters_search.jpg)

## Bir Parametreyi Değiştirme

Bir parametrenin değerini değiştirmek için bir grup ya da arama listesindeki parametre satırına tıklayın. Bu size değeri güncellemenizi sağlayacak bir yan diyalog açar (bu diyalog ayrıca parametre hakkında ek detaylı bilgilerde gösterir - değişikliğin etki etmesi için yeniden başlatmanın gerekip gerekmediği bilgisi de dahil).

![Bir parametre değerini değiştirme](../../../assets/setup/parameters_changing.png)

:::info
**Save** butonuna basıldığında, parametre sessizce ve otomatik olarak bağlı cihaza yüklenir. Parametreye bağlı olarak, değişikliğin etki etmesi için uçuş kontrolcüsünü yeniden başlatmanız gerekebilir.
:::

## Araçlar

Ekranın sağ üstündeki **Tools** menüsünden ek seçenekler seçebilirsiniz.

![Araçlar Menüsü](../../../assets/setup/parameters_tools_menu.png)

**Refresh** <br /> Tüm parametreleri araçtan tekrar isteyerek yenileyin.

**Reset all to defaults** <br />Tüm parametleri varsayıla değerlerine sıfırlayın.

**Load from file / Save to file** <br />Parametreleri var olan bir dosyadan yükleyin veya mevcut parametre ayarlarınızı bir dosyaya kaydedin.

**Clear RC to Param** <br />Bu, RC verici kontrolleri ve parametreleri arasındaki tüm ilişkileri siler. Daha fazla bilgi için göz atın: [Radio Setup > Param Tuning Channels](../setup_view/Radio.md#param-tuning-channels-px4).

**Reboot Vehicle** <br />Aracı yeniden başlatın (bazı parametre değişimlerinden sonra gereklidir).
