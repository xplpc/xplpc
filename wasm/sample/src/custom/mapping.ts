import { MappingList } from "../xplpc/data/mapping-list";
import { MappingItem } from "../xplpc/map/mapping-item";
import { Callback } from "./callback";

export class Mapping {
    public static initialize() {
        MappingList.shared().add(
            "platform.battery.level",
            new MappingItem(Callback.batteryLevel),
        );
    }
}
