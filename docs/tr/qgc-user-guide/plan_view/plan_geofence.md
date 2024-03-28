# Plan Ekranı - Coğrafi Sınır

Coğrafi Sınırlar, aracınızın içinde uçmasına izin verilen ya da _izin verilmeyen_ sanal bölgeler oluşturmanıza olanak sağlar.
Ayrıca eğer izin verilen alanın dışına çıkıldığında yaplıacak eylemi de ayarlayabilirsiniz.

![Coğrafi Sınır'a geneş bakış](../../../assets/plan/geofence/geofence_overview.jpg)

:::info
**ArduPilot users:** Coğrafi Sınır sadece Rover 3.6 ve Copter3.7 ve üzeri sürümlerde desteklenir. Ek olarak günlük sürümlerin ya da stabil 3.6 sürümünün (erişilebilir olduğunda) kullanılmasını gerektirir.
Eğer bağlanan cihaz tarafından Coğrafi Sınır seçeneği desteklenmiyorsa _QGroundControl_ seçeneği göstermeyecektir.
:::

## Create a Geofence

Coğrafi Sınır Oluşturmak için:

1. Plan Ekranı'na gidin

2. Görev Komutları Listesi'nin üstünden _Geofence_'i seçin

   ![Select geofence radio button](../../../assets/plan/geofence/geofence_select.jpg)

3. Insert a circular or polygon region by pressing the **Circular Fence** or **Polygon Fence** button, respectively.
   Haritaya yeni bir bölge ve butonların altına sınırlarla ilgili yeni bir liste eklenecektir.

:::tip
::: tip
Butonlara birden çok kez basarak birden çok bölge oluşturabilirsiniz, böylece karmaşık coğrafi sınırlar oluşturulabilir.
:::

- Dairesel Bölge:

  ![Dairesel Coğrafi Sınır](../../../assets/plan/geofence/geofence_circular.jpg)

  - Merkezi noktayı kaydırarak bölgeyi haritada hareket ettirin
  - Resize the circle by dragging the dot on the edge of the circle (or you can change the radius value in the fence panel).

- Coğrafi Sınır Oluşturma

  Çokgen Bölge:
  ![Çokgen Coğrafi Sınır](../../../assets/plan/geofence/geofence_polygon.jpg)

  - İçi dolu noktaları sürükleyerek köşeleri hareket ettirin
  - İçi dolu noktaların arasındaki içi boş noktalara basarak yeni köşeler oluşturun.

1. Varsayılan olarak, _inclusion_ bölgeleri olarak yeni bölgeler oluşturulur (araçlar bölge içinde kalmalıdır).
   Sınır panelindeki _Inclusion_ onay kutusunun tikini kaldırarak, exclusion bölgelerine (aracın içinde uçamayacağı) dönüştürebilirsiniz.

## GeoFence Düzenleme/Silme

Coğrafi Sınır panelinde _Edit_ butonunu seçerek düzenlemek için bir coğrafi sınır bölgesi seçebilirsiniz.
Daha sonra, önceki bölümde anlatıldığı gibi haritadaki bölgeyi düzenleyebilirsiniz.

Regions can be deleted by pressing the associated **Del** button.

## Upload a GeoFence

GeoFence bir görevle aynı şekilde yüklenir, [Plan tools](../plan_view/plan_view.md)'dan **File**'ı kullanarak.

## Diğer Araçlar

The rest of the tools work exactly as they do while editing a Mission.
