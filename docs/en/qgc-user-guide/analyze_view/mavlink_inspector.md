# MAVLink Inspector

The _MAVLink Inspector_ provides real-time information and charting of MAVLink traffic received by _QGroundControl_.

::: warning
This feature is intended primarily for **autopilot developers**/**vehicle creators**.
It is only supported on desktop builds (Windows, Linux, Mac OS).
:::

![MAVLink inspector](../../../assets/analyze/mavlink_inspector/mavlink_inspector.jpg)

The inspector lists all received messages for the current vehicle, along with their source component id and update frequency.
You can drill down into individual messages to get the message id, source component id, and the values of all the individual fields.
You can also chart field values in real time, selecting multiple fields from multiple messages to display on one of two charts.

To use the _MAVLink Inspector_:

1. Open _Analyze View_ by selecting the _QGroundControl_ application menu ("Q" icon in top left corner) and then choosing the **Analyze Tools** button (from the _Select Tool_ popup).
   ![Analyze ](../../../assets/analyze/menu_analyze_tool.png)
1. Select the **MAVLink Inspector** from the sidebar.

   ![MAVLink inspector menu](../../../assets/analyze/mavlink_inspector/mavlink_inspector_menu.jpg)

   The view will start populating with messages as they are received.

1. Select a message to see its fields and their (dynamically updating) value:

   ![MAVLink inspector: message detail](../../../assets/analyze/mavlink_inspector/mavlink_inspector_message_details.jpg)

1. Add fields to charts by enabling the adjacent checkboxes (plot 1 is displayed below plot 2).

   ![MAVLink inspector: chart fields detail](../../../assets/analyze/mavlink_inspector/mavlink_inspector_plot1.jpg)

   - Fields can be added to only one chart.
   - A chart can have multiple fields, and fields from multiple messages (these are listed above each chart).
     Messages containing fields that are being charted are highlighted with an asterisk.

     ![MAVLink inspector: chart fields detail](../../../assets/analyze/mavlink_inspector/mavlink_inspector_charted_messages.jpg)

   - The _Scale_ and _Range_ are set to sensible values, but can be modified if needed.
