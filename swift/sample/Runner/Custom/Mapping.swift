import xplpc

class Mapping {
    static func initialize() {
        MappingData.shared.add(item: MappingItem(name: "platform.battery.level", target: Callback.batteryLevel))
    }
}
