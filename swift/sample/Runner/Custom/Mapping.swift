import xplpc

class Mapping {
    static func initialize() {
        MappingList.shared.add(name: "platform.battery.level", item: MappingItem(target: Callback.batteryLevel))
    }
}
