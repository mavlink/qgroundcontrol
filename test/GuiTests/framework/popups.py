import global_object_map as names
import squish


def get_popup_text():
    return str(squish.waitForObject(names.systemMessageText).text)
