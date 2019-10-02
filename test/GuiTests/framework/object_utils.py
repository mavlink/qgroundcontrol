import global_object_map as names
import squish


def get_combo(combo_name):
    all_combos = squish.findAllObjects(names.o_FactComboBox)
    for combo in all_combos:
        if combo.fact.name == combo_name:
            return combo
    x = f'There is no combobox with "{combo_name}" name'
    raise LookupError(x)


def change_combo_value(combo_name, new_value):
    combo = get_combo(combo_name)
    squish.mouseClick(squish.waitForObject(combo))
    squish.mouseClick(
        squish.waitForObject(
            {
                "container": names.o_QQuickApplicationWindow,
                "text": new_value,
                "type": "Text",
                "unnamed": 1,
                "visible": True,
            }
        )
    )
