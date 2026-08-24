<template>
    <div class="text-center">
        <div class="camera-container text-center">
            <video
                ref="video"
                :src="source"
                :autoplay="autoplay"
                :playsinline="playsinline"
                style="display: none"
                @canplay="canPlay"
            ></video>

            <img v-if="visible" ref="preview" class="camera-image" />
        </div>

        <div v-if="visible" class="camera-info">
            <p>{{ cameraInfo }}</p>
        </div>

        <canvas ref="canvas" style="display: none"></canvas>
    </div>
</template>

<script lang="ts">
    import { defineComponent, type PropType } from "vue";

    import { Client } from "@/xplpc/client/client";
    import { Param } from "@/xplpc/message/param";
    import { Request } from "@/xplpc/message/request";
    import { DataView } from "@/xplpc/type/data-view";
    import { Log } from "@/xplpc/util/log";

    interface CameraResolution {
        width: number;
        height: number;
    }

    interface CameraSurface {
        canvas: HTMLCanvasElement;
        ctx: CanvasRenderingContext2D;
    }

    export default defineComponent({
        name: "VueCamera",
        props: {
            width: {
                type: [Number, String],
                default: "100%",
            },
            height: {
                type: [Number, String],
                default: 500,
            },
            autoplay: {
                type: Boolean,
                default: true,
            },
            screenshotFormat: {
                type: String,
                default: "image/jpeg",
            },
            selectFirstDevice: {
                type: Boolean,
                default: true,
            },
            playsinline: {
                type: Boolean,
                default: true,
            },
            resolution: {
                type: Object as PropType<CameraResolution | null>,
                default: null,
                validator: (value: CameraResolution | null) => {
                    return (
                        value === null || Boolean(value.height && value.width)
                    );
                },
            },
        },
        emits: [
            "xplpc-camera-list",
            "xplpc-camera-not-supported",
            "xplpc-camera-changed",
            "xplpc-camera-stopped",
            "xplpc-camera-video-live",
            "xplpc-camera-started",
            "xplpc-camera-error",
        ],
        data() {
            return {
                source: undefined as string | undefined,
                canvas: null as HTMLCanvasElement | null,
                ctx: null as CanvasRenderingContext2D | null,
                camerasListEmitted: false,
                cameraList: [] as MediaDeviceInfo[],
                deviceId: "",
                visible: false,
                cameraInfo: "XPLPC",
                destWidth: 0,
                destHeight: 0,
                processTimer: undefined as
                    ReturnType<typeof setInterval> | undefined,
            };
        },
        watch: {
            deviceId: function (id: string) {
                Log.d("[Camera : watch] Device is changed: " + id);
                this.changeCamera(id);
            },
        },
        beforeUnmount() {
            Log.d("[Camera : beforeUnmount]");

            if (this.processTimer !== undefined) {
                clearInterval(this.processTimer);
                this.processTimer = undefined;
            }

            this.stop();
        },
        methods: {
            setupMedia() {
                Log.d("[Camera : setupMedia]");

                if (!navigator.mediaDevices?.getUserMedia) {
                    Log.e(
                        "[Camera : setupMedia] The method getUserMedia is not available in this browser",
                    );
                    this.$emit(
                        "xplpc-camera-not-supported",
                        new Error(
                            "The method getUserMedia is not available in this browser",
                        ),
                    );
                    return;
                }

                this.testMediaAccess();
            },
            loadCameraList() {
                Log.d("[Camera : loadCameras]");

                navigator.mediaDevices
                    .enumerateDevices()
                    .then((deviceInfos) => {
                        for (const deviceInfo of deviceInfos) {
                            if (deviceInfo.kind === "videoinput") {
                                this.cameraList.push(deviceInfo);
                            }
                        }
                    })
                    .then(() => {
                        if (this.camerasListEmitted) {
                            return;
                        }

                        if (
                            this.selectFirstDevice &&
                            this.cameraList.length > 0
                        ) {
                            this.setDeviceId(this.cameraList[0].deviceId);
                        }

                        this.$emit("xplpc-camera-list", this.cameraList);
                        this.camerasListEmitted = true;
                    })
                    .catch((error) =>
                        this.$emit("xplpc-camera-not-supported", error),
                    );
            },
            // Change to a different camera stream, like front and back camera on phones.
            changeCamera(deviceId: string) {
                Log.d("[Camera : changeCamera] Device: " + deviceId);

                this.stop();
                this.$emit("xplpc-camera-changed", deviceId);
                this.loadCamera(deviceId);
            },
            loadSrcStream(stream: MediaStream) {
                Log.d("[Camera : loadSrcStream]");

                const video = this.videoElement();

                if (!video) {
                    return;
                }

                video.srcObject = stream;

                video.onloadedmetadata = () => {
                    this.$emit("xplpc-camera-video-live", stream);
                };

                this.$emit("xplpc-camera-started", stream);
            },
            stopStreamedVideo(videoElem: HTMLVideoElement) {
                Log.d("[Camera : stopStreamedVideo]");

                const stream = videoElem.srcObject;

                if (!(stream instanceof MediaStream)) {
                    return;
                }

                for (const track of stream.getTracks()) {
                    track.stop();

                    this.$emit("xplpc-camera-stopped", stream);
                    videoElem.srcObject = null;
                    this.source = undefined;
                }
            },
            stop() {
                Log.d("[Camera : stop]");

                const video = this.videoElement();

                if (video?.srcObject) {
                    this.stopStreamedVideo(video);
                }
            },
            async start() {
                Log.d("[Camera : start]");

                await navigator.mediaDevices.getUserMedia({
                    audio: false,
                    video: true,
                });

                if (!this.deviceId) {
                    this.loadCameraList();
                }

                if (this.deviceId) {
                    this.setupMedia();
                    this.loadCamera(this.deviceId);
                }
            },
            pause() {
                Log.d("[Camera : pause]");

                const video = this.videoElement();

                if (video?.srcObject) {
                    video.pause();
                }
            },
            resume() {
                Log.d("[Camera : resume]");

                const video = this.videoElement();

                if (video?.srcObject) {
                    video.play();
                }
            },
            processImage() {
                this.processTimer = setInterval(async () => {
                    const surface = this.drawFrame();

                    if (!surface) {
                        return;
                    }

                    const { canvas, ctx } = surface;

                    const imgData = ctx.getImageData(
                        0,
                        0,
                        canvas.width,
                        canvas.height,
                    );
                    const dataView = DataView.createFromArrayBufferView(
                        imgData.data,
                    );

                    const imageSize = dataView.size;
                    const startTime = performance.now();

                    // A frame arrives every few milliseconds, so a block kept on a failed one leaks fast.
                    try {
                        const request = new Request(
                            "sample.image.grayscale.dataview",
                            new Param("dataView", dataView),
                        );

                        await Client.call(request);

                        DataView.withUint8ClampedArray(
                            dataView,
                            (processedData) => {
                                ctx.putImageData(
                                    new ImageData(
                                        processedData,
                                        canvas.width,
                                        canvas.height,
                                    ),
                                    0,
                                    0,
                                );
                            },
                        );

                        const preview = this.$refs.preview;

                        if (preview instanceof HTMLImageElement) {
                            preview.src = canvas.toDataURL();
                        }
                    } finally {
                        DataView.free(dataView);
                    }

                    const duration = performance.now() - startTime;

                    this.cameraInfo =
                        "Time to process: " +
                        (duration / 1000).toFixed(3) +
                        " seconds\nImage size: " +
                        imageSize / 1024 +
                        " kb";
                }, 16);
            },
            testMediaAccess() {
                Log.d("[Camera : testMediaAccess]");

                navigator.mediaDevices
                    .getUserMedia({ video: this.videoConstraints() })
                    .then((stream) => {
                        for (const track of stream.getTracks()) {
                            track.stop();
                        }

                        this.loadCameraList();
                    })
                    .catch((error) => this.$emit("xplpc-camera-error", error));
            },
            loadCamera(device: string) {
                Log.d("[Camera : loadCamera]");

                const video = {
                    ...this.videoConstraints(),
                    deviceId: { exact: device },
                };

                navigator.mediaDevices
                    .getUserMedia({ video })
                    .then((stream) => this.loadSrcStream(stream))
                    .catch((error) => this.$emit("xplpc-camera-error", error));
            },
            // The screenshot is empty while the surface is not ready.
            capture(): string {
                Log.d("[Camera : capture]");

                const surface = this.drawFrame();

                if (!surface) {
                    return "";
                }

                return surface.canvas.toDataURL(this.screenshotFormat);
            },
            // The current video frame is drawn and the surface holding it is returned.
            drawFrame(): CameraSurface | null {
                const video = this.videoElement();

                if (!video) {
                    return null;
                }

                const surface = this.surface(video);

                if (!surface) {
                    return null;
                }

                surface.ctx.drawImage(
                    video,
                    0,
                    0,
                    this.destWidth,
                    this.destHeight,
                );

                return surface;
            },
            // The canvas and its context are created on the first frame, sized to keep the video proportion.
            surface(video: HTMLVideoElement): CameraSurface | null {
                if (this.canvas && this.ctx) {
                    return { canvas: this.canvas, ctx: this.ctx };
                }

                const canvas = this.$refs.canvas;

                if (
                    !(canvas instanceof HTMLCanvasElement) ||
                    !video.videoWidth ||
                    !video.videoHeight
                ) {
                    return null;
                }

                this.destWidth = 240;
                this.destHeight =
                    (this.destWidth * video.videoHeight) / video.videoWidth;

                if (this.destHeight > 320) {
                    this.destHeight = 320;
                    this.destWidth =
                        (this.destHeight * video.videoWidth) /
                        video.videoHeight;
                }

                const ctx = canvas.getContext("2d", {
                    willReadFrequently: true,
                });

                if (!ctx) {
                    Log.e(
                        "[Camera : surface] Unable to create the 2d rendering context",
                    );
                    return null;
                }

                canvas.width = this.destWidth;
                canvas.height = this.destHeight;

                video.width = this.destWidth;
                video.height = this.destHeight;

                this.canvas = canvas;
                this.ctx = ctx;

                return { canvas, ctx };
            },
            // The template ref is only populated once the component is mounted.
            videoElement(): HTMLVideoElement | null {
                const video = this.$refs.video;

                return video instanceof HTMLVideoElement ? video : null;
            },
            videoConstraints(): MediaTrackConstraints {
                if (!this.resolution) {
                    return {};
                }

                return {
                    width: this.resolution.width,
                    height: this.resolution.height,
                };
            },
            setDeviceId(deviceId: string) {
                Log.d("[Camera : setDeviceId] Device: " + deviceId);
                this.deviceId = deviceId;
            },
            canPlay() {
                Log.d("[Camera : canPlay]");
                this.drawFrame();
                this.processImage();
                this.visible = true;
            },
        },
    });
</script>

<style>
    .camera-container {
        max-width: 240px;
        max-height: 320px;
        margin: 0 auto;
        background-color: #000000;
    }

    .camera-image {
        max-width: 240px;
        max-height: 320px;
    }

    .camera-info {
        background: #000000;
        color: #ffffff;
        padding: 6px;
        vertical-align: middle;
        border-radius: 30px;
        margin-top: 20px;
    }

    .camera-info p {
        margin: 0;
        white-space: pre-line;
    }
</style>
