from qgroundcontrol import QGroundControlPO
import squish
import test
import object
import copy


class Table:
    def __init__(self, table):
        self.table = copy.deepcopy(table)

        self.header_row = {
            "container": self.table,
            "id": "tableHeader",
            "type": "Item",
            "unnamed": 1,
            "visible": True,
        }

        self.row = {
            "container": {
                "container": self.table,
                "id": "rowitem",
                "rowIndex": 0,
                "type": "FocusScope",
                "unnamed": 1,
                "visible": True,
            },
            "id": "itemrow",
            "type": "Row",
            "unnamed": 1,
            "visible": True,
        }

    def get_row_count(self):
        return squish.waitForObject(self.table).rowCount

    def get_headers(self):
        squish.waitForObject(self.header_row).headerRow
        headers = squish.findAllObjects(
            {
                "container": self.header_row,
                "type": "Text",
                "unnamed": 1,
                "visible": True,
            }
        )
        return [header.text for header in headers]

    def get_value(self, header, row_number):
        column = self.get_headers().index(header)
        return self.get_cells_from_row(row_number)[column].text

    def get_headers_sizes(self):
        header_row = squish.waitForObject(self.header_row).headerRow
        return [
            (header.width, header.height)
            for header in object.children(header_row)
            if header.id == "headerRowDelegate"
        ]

    def get_row(self, position=0):
        row = copy.deepcopy(self.row)
        row["container"]["rowIndex"] = position
        return squish.waitForObject(row)

    def get_cells_from_row(self, position=0):
        item_row = self.get_row(position)
        return [
            squish.waitForObject(cell.item)
            for cell in object.children(item_row)
            if "TableViewItemDelegateLoader" in squish.className(cell)
        ]

    def get_columns_sizes(self, position=0):
        return [
            (cell.width, cell.height)
            for cell in self.get_cells_from_row(position)
            if "TableViewItemDelegateLoader" in squish.className(cell)
        ]

    def get_texts_sizes(self, position=0):
        return [
            (cell.item.implicitWidth, cell.item.implicitHeight)
            for cell in self.get_cells_from_row(position)
            if "TableViewItemDelegateLoader" in squish.className(cell)
        ]

    def select_row(self, position=0):
        squish.mouseClick(self.get_row(position))


class Slider(QGroundControlPO):
    slider_drag_area = {
        "container": QGroundControlPO.application_window,
        "id": "sliderDragArea",
        "type": "QGCMouseArea",
        "unnamed": 1,
        "visible": True,
    }

    @staticmethod
    def confirm():
        test.log("Confirm with a slider")
        squish.mouseDrag(
            squish.waitForObject(Slider.slider_drag_area),
            150,
            23,
            300,
            23,
            squish.Qt.NoModifier,
            squish.Qt.LeftButton,
        )


class ComboBox(QGroundControlPO):
    @staticmethod
    def get_combo(combo_name):
        all_combos = squish.findAllObjects(
            {
                "container": QGroundControlPO.application_window,
                "type": "FactComboBox",
                "unnamed": 1,
                "visible": True,
            }
        )
        for combo in all_combos:
            if combo.fact.name == combo_name:
                return combo
        x = f'There is no combobox with "{combo_name}" name'
        raise LookupError(x)

    @staticmethod
    def change_combo_value(combo_name, new_value):
        combo = ComboBox.get_combo(combo_name)
        squish.mouseClick(squish.waitForObject(combo))
        squish.mouseClick(
            squish.waitForObject(
                {
                    "container": QGroundControlPO.application_window,
                    "text": new_value,
                    "type": "Text",
                    "unnamed": 1,
                    "visible": True,
                }
            )
        )


class Popup(QGroundControlPO):
    systemMessageText = {
        "container": QGroundControlPO.o_Overlay,
        "id": "systemMessageText",
        "type": "TextEdit",
        "unnamed": 1,
        "visible": True,
    }

    @staticmethod
    def get_text():
        message_box_text = squish.waitForObject(Popup.systemMessageText)
        return str(message_box_text.text)


class SliderPO(QGroundControlPO):
    sliderDragArea_QGCMouseArea = {
        "container": QGroundControlPO.application_window,
        "id": "sliderDragArea",
        "type": "QGCMouseArea",
        "unnamed": 1,
        "visible": True,
    }


class Slider:
    @staticmethod
    def confirm():
        test.log("Confirm with a slider")
        squish.mouseDrag(
            squish.waitForObject(SliderPO.sliderDragArea_QGCMouseArea),
            150,
            23,
            300,
            23,
            squish.Qt.NoModifier,
            squish.Qt.LeftButton,
        )
