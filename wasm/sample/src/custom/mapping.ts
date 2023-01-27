import { XMappingList } from "../xplpc/data/mapping-list";
import { XMappingItem } from "../xplpc/map/mapping-item";
import { Callback } from "./callback";

export class Mapping {
    public static initialize() {
        XMappingList.shared().add("platform.battery.level", new XMappingItem(Callback.batteryLevel));
    }
}
