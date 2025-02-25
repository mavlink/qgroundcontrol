# Motor Kurulumu

Motor Kurulumu, ayrı motorları / servoları test etmek için kullanılır (örneğin, motorların doğru yönde döndüğünü doğrulamak için).

:::tip
Bu talimatlar PX4 ve ArduPilot'taki çoğu araç türü için geçerlidir.
Araca özel talimatlar, alt konular olarak sağlanır (örn. [ Motor Kurulumu (ArduSub) ](../setup_view/motors_ardusub.md)).
:::

![Motorların Testi](../../../assets/setup/Motors.png)

## Test Adımları

Motorları test etmek için:

1. Tüm pervaneleri çıkarın.

   > \*\* Uyarı \*\* Pervaneleri, motorları etkinleştirmeden önce çıkarmalısınız!
   > :::

2. (_ yalnızca PX4 _) güvenlik anahtarını etkinleştirin - varsa.

3. Slide the switch to enable motor slider and buttons (labeled: _Propellers are removed - Enable slider and motors_).

4. Adjust the slider to select the power for the motors.

5. Press the button corresponding to the motor and confirm it spin in the correct direction.

   ::: info
   The motors will automatically stop spinning after 3 seconds.
   You can also stop the motor by pressing the 'Stop' button.
   If no motors turn, raise the “Throttle %” and try again.
   :::

## Ek Bilgi

- [Basic Configuration > Motor Setup](http://docs.px4.io/master/en/config/motors.html) (_PX4 User Guide_) -Buradan, PX4 için ek bilgiler bulabilirsiniz.
- [ESCS and Motors](https://ardupilot.org/copter/docs/connect-escs-and-motors.html#motor-order-diagrams) - This is the Motor order diagrams for all frames
