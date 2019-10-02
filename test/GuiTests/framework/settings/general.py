import objectMap
import squish
import test

from object_utils import change_combo_value, get_combo


def change_distance_units(new_unit):
    test.log(f'[Settings][General] Change Distance units to "{new_unit}"')
    change_combo_value("distanceUnits", new_unit)


def get_distance_units():
    combo_real_name = objectMap.realName(get_combo("distanceUnits"))
    return str(squish.waitForObjectExists(combo_real_name).displayText)
