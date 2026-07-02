import QtQuick
import QtTest

import "../../../../src/AutoPilotPlugins/PX4/FailureInjectionInstances.js" as Instances

/// Tests the pure instance-selection logic shared with FailureInjectionComponent.qml.
TestCase {
    id: testCase
    name: "FailureInjectionInstancesTest"

    function test_instanceSend_data() {
        return [
            { tag: "all", selected: [0], param3: 0, param4: 0, label: "all" },
            { tag: "single", selected: [3], param3: 3, param4: 0, label: "3" },
            { tag: "unsorted-single", selected: [3], param3: 3, param4: 0, label: "3" },
        ]
    }

    function test_instanceSend(data) {
        var send = Instances.instanceSend(data.selected)
        compare(send.param3, data.param3)
        compare(send.param4, data.param4)
        compare(send.label, data.label)
    }

    function test_instanceSend_multiIsBitmask() {
        var send = Instances.instanceSend([1, 3, 5])
        verify(isNaN(send.param3), "param3 is NaN when using the bitmask form")
        compare(send.param4, (1 << 0) | (1 << 2) | (1 << 4))
        compare(send.label, "1, 3, 5")
    }

    function test_instanceSend_sortsBeforeLabeling() {
        var send = Instances.instanceSend([5, 1, 3])
        compare(send.label, "1, 3, 5")
    }

    function test_toggleInstance_addsAndRemoves() {
        var sel = Instances.toggleInstance([1], 2)
        compare(sel, [1, 2])

        sel = Instances.toggleInstance(sel, 1)
        compare(sel, [2])
    }

    function test_toggleInstance_clearsAllSelection() {
        var sel = Instances.toggleInstance([0], 3)
        compare(sel, [3])
    }

    function test_unitName_found() {
        var units = [{ name: "GYRO", unit: 0 }, { name: "GPS", unit: 4 }]
        compare(Instances.unitName(units, 4), "GPS")
    }

    function test_unitName_fallsBackToNumber() {
        compare(Instances.unitName([], 99), "99")
    }

    function test_typeName_found() {
        var types = [{ name: "OK", type: 0 }, { name: "OFF", type: 1 }]
        compare(Instances.typeName(types, 1), "OFF")
    }

    function test_typeName_fallsBackToNumber() {
        compare(Instances.typeName([], 7), "7")
    }
}
