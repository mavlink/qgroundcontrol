# Plan Ekranı - Toparlanma Noktaları

Toparlanma noktaları, iniş ya da oyalanma noktalarına alternatif noktalardır.
Genellikle Geri Dönüş / RTL modunda ana konumdan daha güvenli veya daha uygun (örneğin daha yakın) bir varış noktası sağlamak için kullanılırlar.

:::info
_QGroundControl_ will not display the Rally Point options if they are not supported by the connected vehicle firmware.
:::

## Rally Point Usage

Toparlanma Noktası oluşturmak için:

1. Plan Ekranı'na gidin
2. Select the **Rally Points** layer using the [Layer Switcher](plan_view.md#layer_switcher) in the top-right area of the map (or expand the **Rally Points** section in the [Plan Editor Panel](plan_view.md#plan_editor_panel))
3. Haritanın neresinde toparlanma noktası olmasını istiyorsanız tıklayın.
   - Her biri için bir **R** işareti eklenir
   - The currently active marker has a different color (green) and its editor is expanded in the _Rally Points_ section.
4. Make any rally point active by selecting it on the map or in the _Rally Points_ section:
   - Move the active rally point by either dragging it on the map or editing the position fields in its editor.
   - Delete a rally point by pressing the **X** delete button on its editor.

## Upload Rally Points

Rally points are uploaded along with the rest of the plan using the **Upload** button in the [Plan Toolbar](plan_view.md#file).
