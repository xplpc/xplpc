import { XRemoteClient } from "@/xplpc/client/remote-client";
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

// eslint-disable-next-line @typescript-eslint/no-unused-vars
function reverseResponse(message: XMessage): Promise<string> {
    return new Promise<string>((resolve) => {
        return resolve("ok");
    });
}

describe("RemoteClient", () => {
    beforeAll(async () => {
        // initialize module
        const module: IXWasmModule = await Module();
        XPLPC.shared().initialize(module, new XConfig(new JsonSerializer()));

        // initialize local mappings
        XMappingList.shared().add("platform.reverse.response", new XMappingItem(reverseResponse));
    });

    it("check if is initialized", () => {
        expect(XPLPC.shared().initialized).toBeTruthy();
    });

    it("get logged value", () => {
        const request = new XRequest("sample.login",
            new XParam("username", "paulo"),
            new XParam("password", "123456"),
            new XParam("remember", true),
        );

        XRemoteClient.call<string>(request).then((response: string | undefined) => {
            expect(response).toBe("LOGGED-WITH-REMEMBER");
        });
    });

    it("get logged value with await", async () => {
        const request = new XRequest("sample.login",
            new XParam("username", "paulo"),
            new XParam("password", "123456"),
            new XParam("remember", true),
        );

        const response = await XRemoteClient.call<string>(request);
        expect(response).toBe("LOGGED-WITH-REMEMBER");
    });

    it("get reverse value", () => {
        const request = new XRequest("sample.reverse");

        XRemoteClient.call<string>(request).then((response: string | undefined) => {
            expect(response).toBe("response-is-ok");
        });
    });

    it("get reverse value with await", async () => {
        const request = new XRequest("sample.reverse");

        const response = await XRemoteClient.call<string>(request);
        expect(response).toBe("response-is-ok");
    });
});
