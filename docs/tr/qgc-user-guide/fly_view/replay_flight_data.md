# Uçuş Verilerini Yeniden Oynat

:::warning
Bu özellik, öncelikle \*\* otopilot geliştiricileri \*\* / \*\* araç tasarlayıcıları \*\* için tasarlanmıştır.
Bu özellik, sadece bilgisayar sürümlerinde desteklenmektedir (Windows, Linux, Mac OS).
:::

The _Replay Flight Data_ feature allows users to replay a telemetry log, enabling review of past or problematic flights.
Uçuş başlatılabilir, duraklatılabilir, durdurulabilir, yeniden başlatılabilir vb.

:::info
_QGroundControl_ uçuş tekrarını aktif bir bağlantı gibi görür.
Oynatmayı duraklattığınızda / durdurduğunuzda, yer istasyonu "İletişim Kaybı" olarak rapor edecek ve bağlantının kesilmesini veya daha fazla komut bekleyecektir.
:::

Bir uçuşu tekrar etmek için:

1. Tüm aktif bağlantıları kesin.

2. **Application Settings > General > Fly View**'i seçin

3. Ekranın altında uçuşu tekrar oynat butonunu görebilmek için **Show Telemetry Log Replay Status Bar**'ı işaretleyin.

   ![Uçuş Tekrarını Aç / Kapat](../../../assets/fly/flight_replay/flight_replay_toggle.jpg)

4. _file selection_ seçeneğine erişmek için **Load Telemetry Log**'a tıklayın.
   - Tekrar oynatılması için uygun telemetri kayıtlarından bir kayıt dosyası seçin.
   - QGroundControl \*, kayıtı hemen oynatmaya başlar.

5. Bir kayıt yüklendiğinde şunları kullanabilirsiniz:
   - Oynatmayı durdurmak ve yeniden başlatmak için **Pause/Play** butonuna basın.
   - Kayıtta yeni bir konuma ilerlemek için _Slider_.
   - _Rate_ selector to choose the playback speed.

6. To stop replay (i.e. to load a new file to replay), first pause the flight, and then select **Disconnect** (when it appears).
   After disconnecting, the **Load Telemetry Log** button will be displayed.

:::tip
[ MAVLink Inspector ](../analyze_view/mavlink_inspector.md) 'ı kullanarak devam eden tekrarı daha ayrıntılı olarak inceleyebilirsiniz.
:::
