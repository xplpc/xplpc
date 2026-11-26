import VueCamera from "@/components/widgets/VueCamera.vue";
import { flushPromises, mount } from "@vue/test-utils";

class FakeTrack {
    stopped = false;

    stop(): void {
        this.stopped = true;
    }
}

class FakeStream {
    private readonly tracks = [new FakeTrack()];

    getTracks(): FakeTrack[] {
        return this.tracks;
    }

    get live(): boolean {
        return this.tracks.some((track) => !track.stopped);
    }
}

// The browser hands out a stream per request, and the camera stays on for as long as any of them has a track that was not stopped.
const streams: FakeStream[] = [];
let refuse = false;

function liveStreams(): FakeStream[] {
    return streams.filter((stream) => stream.live);
}

function requestStream(): Promise<FakeStream> {
    if (refuse) {
        return Promise.reject(new Error("the permission was denied"));
    }

    const stream = new FakeStream();
    streams.push(stream);

    return Promise.resolve(stream);
}

describe("VueCamera", () => {
    beforeEach(() => {
        streams.length = 0;
        refuse = false;

        vi.stubGlobal("MediaStream", FakeStream);

        Object.defineProperty(navigator, "mediaDevices", {
            configurable: true,
            value: {
                getUserMedia: requestStream,
                enumerateDevices: () =>
                    Promise.resolve([
                        { kind: "videoinput", deviceId: "front" },
                    ]),
            },
        });
    });

    afterEach(() => {
        vi.unstubAllGlobals();
    });

    test("opening the camera leaves running only the stream it shows", async () => {
        const wrapper = mount(VueCamera);

        wrapper.vm.start();
        await flushPromises();

        const video = wrapper.find("video").element;

        expect(liveStreams()).toHaveLength(1);
        expect(video instanceof HTMLVideoElement && video.srcObject).toBe(
            liveStreams()[0],
        );

        wrapper.unmount();

        expect(liveStreams()).toHaveLength(0);
    });

    test("opening the camera again stops the stream it was showing", async () => {
        const wrapper = mount(VueCamera);

        wrapper.vm.start();
        await flushPromises();

        wrapper.vm.start();
        await flushPromises();

        expect(liveStreams()).toHaveLength(1);

        wrapper.unmount();
    });

    test("reopening the camera twice before either stream arrives leaves one running", async () => {
        const wrapper = mount(VueCamera);

        wrapper.vm.start();
        await flushPromises();

        wrapper.vm.start();
        wrapper.vm.start();
        await flushPromises();

        expect(liveStreams()).toHaveLength(1);

        wrapper.unmount();

        expect(liveStreams()).toHaveLength(0);
    });

    test("a stream that arrives once the camera went away is stopped", async () => {
        const wrapper = mount(VueCamera);

        wrapper.vm.start();
        await flushPromises();

        wrapper.vm.start();
        wrapper.unmount();
        await flushPromises();

        expect(liveStreams()).toHaveLength(0);
    });

    test("a refused permission is reported rather than left unhandled", async () => {
        refuse = true;

        const wrapper = mount(VueCamera);

        wrapper.vm.start();
        await flushPromises();

        expect(wrapper.emitted("xplpc-camera-error")).toHaveLength(1);

        wrapper.unmount();
    });
});
