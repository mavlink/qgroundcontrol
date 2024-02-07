# Şablon

_Pattern tools_ ([PlanView](../plan_view/plan_view.md) _Plan Tools_'un içindeki), karmaşık uçuş şablonlarını basit bir grafik kullanıcı arayüzü kullanarak özelleştirmenize olanak tanır.
Uygun şablon araçları, araca (ve uçuş dizinindeki araç tipinin desteğine) bağlıdır.

![Şablon Aracı (Plan Araçları)](../../../assets/plan/pattern/pattern_tool.jpg)

| Şablon                                                          | Tanım                                                                                                                                                                                                                                        | Araçlar           |
| --------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ----------------- |
| [Yüzey](../plan_view/pattern_survey.md)                         | Çok köşeli bir alan üstünde ızgara şeklinde bir uçuş şablonu oluşturun. Alanı ve coğrafi etiketli görüntüler oluşturmak için uygun ızgara ve kamera ayarlarını belirleyebilirsiniz.                                                          | Tüm               |
| [Yapı Taraması](../plan_view/pattern_structure_scan_v2.md)      | Dikey yüzeyler üzerinde (çok köşeli ya da dairesel) görüntüler yakalayabilmek için ızgara şeklinde bir uçuş şablonu oluşturun. These are typically used for the visual inspection or creation of 3D models of structures. | MultiCopter, VTOL |
| [Koridor Taraması](../plan_view/pattern_corridor_scan.md)       | Çoklu çizgileri takip eden bir uçuş planı oluşturun (mesela, bir yolu gözlemlemek için).                                                                                                                                  | Tüm               |
| [Sabit Kanat İnişi](../plan_view/pattern_fixed_wing_landing.md) | Bir göreve sabit kanatlı araçlar için iniş yolu ekleyin.                                                                                                                                                                                     | Fixed Wing        |
