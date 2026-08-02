# Plan Ekranı - Coğrafi Sınır

Coğrafi Sınırlar, aracınızın içinde uçmasına izin verilen ya da _izin verilmeyen_ sanal bölgeler oluşturmanıza olanak sağlar.
Ayrıca eğer izin verilen alanın dışına çıkıldığında yaplıacak eylemi de ayarlayabilirsiniz.

:::info
_QGroundControl_ will not display the GeoFence options if they are not supported by the connected vehicle firmware.
:::

## Create a GeoFence

Coğrafi Sınır Oluşturmak için:

1. Plan Ekranı'na gidin

2. Select the **GeoFence** layer using the [Layer Switcher](plan_view.md#layer_switcher) in the top-right area of the map (or expand the **GeoFence** section in the [Plan Editor Panel](plan_view.md#plan_editor_panel))

3. Insert a circular or polygon region by pressing the **Circular Fence** or **Polygon Fence** button in the GeoFence section.
   Haritaya yeni bir bölge ve butonların altına sınırlarla ilgili yeni bir liste eklenecektir.

:::tip
::: tip
Butonlara birden çok kez basarak birden çok bölge oluşturabilirsiniz, böylece karmaşık coğrafi sınırlar oluşturulabilir.
:::

- Dairesel Bölge:

  - Merkezi noktayı kaydırarak bölgeyi haritada hareket ettirin
  - Resize the circle by dragging the dot on the edge of the circle (or you can change the radius value in the fence panel).

- Coğrafi Sınır Oluşturma

  - İçi dolu noktaları sürükleyerek köşeleri hareket ettirin
  - İçi dolu noktaların arasındaki içi boş noktalara basarak yeni köşeler oluşturun.

1. Varsayılan olarak, _inclusion_ bölgeleri olarak yeni bölgeler oluşturulur (araçlar bölge içinde kalmalıdır).
   Sınır panelindeki _Inclusion_ onay kutusunun tikini kaldırarak, exclusion bölgelerine (aracın içinde uçamayacağı) dönüştürebilirsiniz.

Depending on the firmware, the GeoFence section may also show fence parameters (e.g. breach action) and a **Breach Return Point** that the vehicle will fly to if it breaches the fence.

## GeoFence Düzenleme/Silme

You can select a GeoFence region to edit by selecting its _Edit_ radio button in the GeoFence section.
Daha sonra, önceki bölümde anlatıldığı gibi haritadaki bölgeyi düzenleyebilirsiniz.

Regions can be deleted by pressing the associated **Del** button.

## Upload a GeoFence

The GeoFence is uploaded along with the rest of the plan using the **Upload** button in the [Plan Toolbar](plan_view.md#file).
