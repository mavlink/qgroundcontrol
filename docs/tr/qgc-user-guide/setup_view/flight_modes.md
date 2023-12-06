# Uçuş Modları Kurulumu

Uçuş Modları \* bölümü, uçuş modlarını radyo kanal(lar)ına ve dolayısıyla radyo kontrol vericinizdeki anahtarlara eşlemenize olanak tanır.
Hem uçuş modu kurulumu hem de mevcut uçuş modları PX4 ve ArduPilot'ta farklıdır (ve ArduCopter ile ArduPlane arasında bazı farklılıklar vardır).

:::info
In order to set up flight modes you must already have already [configured your radio](../setup_view/radio.md), and [setup the transmitter](#transmitter-setup) (as shown below).
:::

Bu bölüme erişmek için, üstteki araç çubuğundan **dişli** simgesini (Vehicle Setup), daha sonra kenar çubuğundan **Flight Modes**'u seçin.

For more flight stack specific setup see:

- [ArduPilot Flight Modes Setup](../setup_view/flight_modes_ardupilot.md)
- [PX4 Flight Modes Setup](../setup_view/flight_modes_px4.md)

## Transmitter Setup

In order setup flight modes you will first need to configure your _transmitter_ to encode the physical positions of your mode switch(es) into a single channel.

On both PX4 and ArduPilot you can assign up to 6 different flight modes to a single channel of your transmitter It is common to use the positions of a 2- and a 3-position switch on the transmitter to represent the 6 flight modes.
Each combination of switches is then encoded as a particular PWM value that will be sent on a single channel.

:::info
The single channel is selectable on PX4 and ArduPlane, but is fixed to channel 5 on Copter.
:::

The process for this varies depending on the transmitter.
A number of setup examples are provided below.

### Taranis

These examples show several configurations for the _FrSky Taranis_ transmitter.

#### Map 3-way Switch to a Single Channel

If you only need to support selecting between two or three modes then you can map the modes to the positions just one 3-way switch.
Below we show how to map the Taranis 3-way "SD" switch to channel 5.

Open the Taranis UI **MIXER** page and scroll down to **CH5**, as shown below:

![Taranis - Map channel to switch](../../../assets/setup/flight_modes/taranis_single_channel_mode_selection_1.png)

Press **ENT(ER)** to edit the **CH5** configuration then change the **Source** to be the _SD_ button.

![Taranis - Configure channel](../../../assets/setup/flight_modes/taranis_single_channel_mode_selection_2.png)

That's it!
Channel 5 will now output 3 different PWM values for the three different **SD** switch positions.

#### Map Multiple Switches to a Single Channel

Most transmitters do not have 6-way switches, so if you need to be able to support more modes than the number of switch positions available (up to 6) then you will have to represent them using multiple switches.
Commonly this is done by encoding the positions of a 2- and a 3-position switch into a single channel, so that each switch position combination results in a different PWM value.

On the FrSky Taranis this process involves assigning a "logical switch" to each combination of positions of the two real switches.
Each logical switch is then assigned to a different PWM value on the same channel.

This video shows how this is done with the _FrSky Taranis_ transmitter: https\://youtu.be/TFEjEQZqdVA

<!-- @[youtube](https://youtu.be/BNzeVGD8IZI?t=427) - video showing how to set the QGC side - at about 7mins and 3 secs -->
