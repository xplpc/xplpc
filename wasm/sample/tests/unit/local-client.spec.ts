import { XLocalClient } from "@/xplpc/client/local-client";
import { XConfig } from "@/xplpc/core/config";
import { XPLPC } from "@/xplpc/core/xplpc";
import { XMappingList } from "@/xplpc/data/mapping-list";
import { XMappingItem } from "@/xplpc/map/mapping-item";
import { XMessage } from "@/xplpc/message/message";
import { XParam } from "@/xplpc/message/param";
import { XRequest } from "@/xplpc/message/request";
import { IXWasmModule } from "@/xplpc/module/xplpc-module";
import { JsonSerializer } from "@/xplpc/serializer/json-serializer";

import Module from "@xplpc/build/wasm/bin/xplpc";

function batteryLevel(message: XMessage): Promise<string> {
    return new Promise<string>((resolve) => {
        const suffix = message.get("suffix");
        return resolve("100" + suffix);
    });
}

describe("LocalClient", () => {
    beforeAll(async () => {
        // initialize module
        const module: IXWasmModule = await Module();
        XPLPC.shared().initialize(module, new XConfig(new JsonSerializer()));

        // initialize local mappings
        XMappingList.shared().add("platform.battery.level", new XMappingItem(batteryLevel));
    });

    it("check if is initialized", () => {
        expect(XPLPC.shared().initialized).toBeTruthy();
    });

    it("sample platform", () => {
        const request = new XRequest("platform.battery.level", new XParam("suffix", "%"))

        XLocalClient.call<string>(request).then((response: string | undefined) => {
            expect(response).toBe("100%");
        });
    });

    it("sample platform with await", async () => {
        const request = new XRequest("platform.battery.level", new XParam("suffix", "%"))
        const response = await XLocalClient.call<string>(request);
        expect(response).toBe("100%");
    });
});
