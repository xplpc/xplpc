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
import { XDataView } from "@/xplpc/type/data-view";

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

    test("transfer data view", () => {
        const dataView = XDataView.createFromArrayBuffer(new Uint8Array([
            255, 0, 0, 255, // red pixel
            0, 255, 0, 255, // green pixel
            0, 0, 255, 255, // blue pixel
            0, 0, 0, 0,     // transparent pixel
        ]));

        const request = new XRequest("sample.image.grayscale.dataview",
            new XParam("dataView", dataView)
        );

        XRemoteClient.call<string>(request).then((response: string | undefined) => {
            const processedData = XDataView.createUint8ClampedArrayFromPtr(dataView.ptr, dataView.size);

            expect("OK").toBe(response);

            expect(16).toBe(processedData.length);
            expect(processedData[0]).toBe(85);
            expect(processedData[4]).toBe(85);
            expect(processedData[8]).toBe(85);
            expect(processedData[12]).toBe(0);

            XDataView.free(dataView);
        });
    });

    test("transfer data view with await", async () => {
        const dataView = XDataView.createFromArrayBuffer(new Uint8Array([
            255, 0, 0, 255, // red pixel
            0, 255, 0, 255, // green pixel
            0, 0, 255, 255, // blue pixel
            0, 0, 0, 0,     // transparent pixel
        ]));

        const request = new XRequest("sample.image.grayscale.dataview",
            new XParam("dataView", dataView)
        );

        const response = await XRemoteClient.call<string>(request);
        const processedData = XDataView.createUint8ClampedArrayFromPtr(dataView.ptr, dataView.size);

        expect("OK").toBe(response);

        expect(16).toBe(processedData.length);
        expect(processedData[0]).toBe(85);
        expect(processedData[4]).toBe(85);
        expect(processedData[8]).toBe(85);
        expect(processedData[12]).toBe(0);

        XDataView.free(dataView);
    });
});
