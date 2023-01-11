interface IXNativePlatformProxyShared {
    shared: () => IXNativePlatformProxy;
}

export interface IXNativePlatformProxy extends IXNativePlatformProxyShared {
    createFromPtr(proxy: IXNativePlatformProxy): void;
    createDefault(): void;
    initialize(): void;
    hasProxy(): boolean;
    extend(name: string, obj: unknown): unknown;
}

interface XWebPlatformProxyCallback {
    exec(data: string): void;
}

const XWebPlatformProxy = {
    sleep: function (time: number) {
        return new Promise((resolve) => setTimeout(resolve, time));
    },
    onRemoteProxyCall: function (data: string): string {
        // TODO: XPLPC - NEED REAL IMPLEMENTATION
        console.log("[XWebPlatformProxy : onRemoteProxyCall] Called: " + data);
        return '{"r": "sync-test-ok"}';
    },
    // TODO: XPLPC - WHAT IS THE CORRECT METHOD SIGNATURE FOR ASYNC AND CALL THE CALLBACK?
    onRemoteProxyCallAsync: async function (data: string, callback: XWebPlatformProxyCallback) {
        // TODO: XPLPC - THIS CODE IS NEVER CALLED
        // TODO: XPLPC - NEED REAL IMPLEMENTATION
        console.log("[XWebPlatformProxy : onRemoteProxyCallAsync] Called: " + data);

        await this.sleep(3000);

        console.log("[XWebPlatformProxy : onRemoteProxyCallAsync] After Sleep");

        callback.exec('{"r": "async-test-ok"}');
        // the above line ^ throw error: "Cannot pass deleted object as a pointer of type XVoidFunctorString const*"
        //
        // which is:
        //
        // EMSCRIPTEN_BINDINGS(xplpc_std_function)
        // {
        //     em::class_<std::function<void(const std::string &)>>("XVoidFunctorString")
        //         .constructor<>()
        //         .function("exec", &std::function<void(const std::string &)>::operator());
        // }
        //
        // what is wrong?
    }
}

export default XWebPlatformProxy;
