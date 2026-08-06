# Parametreler

_Parametreler_ ekranı, araçla ilişkili parametrelerden herhangi birini bulmanıza ve düzenlemenizi sağlar.

:::tip Having trouble?
If parameters fail to download, see [Parameter Download Failures](../troubleshooting/parameter_download.md).
:::

:::info
PX4 Pro ve ArduPilot farklı parametre setleri kullanır, ancak her ikisi de bu bölümde açıklandığı gibi yönetilir.
:::

## Bir Parametreyi Bulma

Parametreler gruplar halinde düzenlenmiştir. Select a group of parameters to view by clicking on the buttons to the left.

Ayrıca _ Search_ alanına bir terim girerek bir parametre için _ arama _ yapabilirsiniz. Bu size girilen alt diziyi içeren tüm parametre adlarının ve açıklamalarının bir listesini gösterecektir (aramayı sıfırlamak için \*\* Clear \*\* tuşuna basın).

## Changing a Parameter

To change the value of a parameter click on the parameter row in a group or search list. This will open a side dialog in which you can update the value (this dialog also provides additional detailed information about the parameter - including whether a reboot is required for the change to take effect).

:::info
**Save** butonuna basıldığında, parametre sessizce ve otomatik olarak bağlı cihaza yüklenir. Parametreye bağlı olarak, değişikliğin etki etmesi için uçuş kontrolcüsünü yeniden başlatmanız gerekebilir.
:::

## Araçlar

Ekranın sağ üstündeki **Tools** menüsünden ek seçenekler seçebilirsiniz.

**Refresh** <br /> Tüm parametreleri araçtan tekrar isteyerek yenileyin.

**Reset all to defaults** <br />Tüm parametleri varsayıla değerlerine sıfırlayın.

**Load from file / Save to file** <br />Parametreleri var olan bir dosyadan yükleyin veya mevcut parametre ayarlarınızı bir dosyaya kaydedin.

**Clear RC to Param** <br />Bu, RC verici kontrolleri ve parametreleri arasındaki tüm ilişkileri siler. For more information see: [Radio Setup > Param Tuning Channels](../setup_view/radio.md#param-tuning-channels-px4).

**Reboot Vehicle** <br />Aracı yeniden başlatın (bazı parametre değişimlerinden sonra gereklidir).
