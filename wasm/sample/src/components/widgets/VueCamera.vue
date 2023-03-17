<template>
    <div class="text-center">
        <div class="camera-container text-center">
            <video
ref="video" :src="source" :autoplay="autoplay" :playsinline="playsinline" style="display: none;"
                @canplay="canPlay"></video>

            <img v-if="visible" ref="preview" class="camera-image" />
        </div>

        <div v-if="visible" class="camera-info">
            <p>{{ cameraInfo }}</p>
        </div>

        <canvas ref="canvas" style="display: none;"></canvas>
    </div>
</template>

<script lang="js">
import { XClient } from '@/xplpc/client/client';
import { XParam } from '@/xplpc/message/param';
import { XRequest } from '@/xplpc/message/request';
import { XDataView } from '@/xplpc/type/data-view';
import { Log } from '@/xplpc/util/log';

export default {
    name: "VueCamera",
    props: {
        width: {
            type: [Number, String],
            default: "100%"
        },
        height: {
            type: [Number, String],
            default: 500
        },
        autoplay: {
            type: Boolean,
            default: true
        },
        screenshotFormat: {
            type: String,
            default: "image/jpeg"
        },
        selectFirstDevice: {
            type: Boolean,
            default: true
        },
        playsinline: {
            type: Boolean,
            default: true
        },
        resolution: {
            type: Object,
            default: null,
            validator: value => {
                return value.height && value.width;
            }
        }
    },
    emits: ['xplpc-camera-list', 'xplpc-camera-not-supported', 'xplpc-camera-changed', 'xplpc-camera-stopped', 'xplpc-camera-video-live', 'xplpc-camera-started', 'xplpc-camera-error'],
    data() {
        return {
            source: null,
            canvas: null,
            camerasListEmitted: false,
            cameraList: [],
            deviceId: "",
            visible: false,
            ctx: null,
            cameraInfo: "XPLPC",
            destWidth: 0,
            destHeight: 0
        };
    },
    watch: {
        deviceId: function (id) {
            Log.d("[Camera : watch] Device is changed: " + id);
            this.changeCamera(id);
        }
    },
    beforeUnmount() {
        Log.d("[Camera : beforeUnmount]");
        this.stop();
    },
    methods: {
        // get user media
        legacyGetUserMediaSupport() {
            Log.d("[Camera : legacyGetUserMediaSupport]");

            return constraints => {
                // first get ahold of the legacy getUserMedia if present
                const getUserMedia =
                    navigator.getUserMedia ||
                    navigator.webkitGetUserMedia ||
                    navigator.mozGetUserMedia ||
                    navigator.msGetUserMedia ||
                    navigator.oGetUserMedia;

                // some browsers just don't implement it, so return a rejected promise with an error to keep a consistent interface
                if (!getUserMedia) {
                    Log.d("[Camera : legacyGetUserMediaSupport] The method getUserMedia is not implemented in this browser");

                    return Promise.reject(
                        new Error("[Camera : legacyGetUserMediaSupport] The method getUserMedia is not implemented in this browser")
                    );
                }

                // otherwise, wrap the call to the old navigator.getUserMedia with a promise
                return new Promise(function (resolve, reject) {
                    getUserMedia.call(navigator, constraints, resolve, reject);
                });
            };
        },
        // setup media
        setupMedia() {
            Log.d("[Camera : setupMedia]");

            if (navigator.mediaDevices === undefined) {
                navigator.mediaDevices = {};
            }

            if (navigator.mediaDevices.getUserMedia === undefined) {
                navigator.mediaDevices.getUserMedia = this.legacyGetUserMediaSupport();
            }

            this.testMediaAccess();
        },
        // load available cameras
        loadCameraList() {
            Log.d("[Camera : loadCameras]");

            navigator.mediaDevices
                .enumerateDevices()
                .then(deviceInfos => {
                    for (let i = 0; i !== deviceInfos.length; ++i) {
                        const deviceInfo = deviceInfos[i];

                        if (deviceInfo.kind === "videoinput") {
                            this.cameraList.push(deviceInfo);
                        }
                    }
                })
                .then(() => {
                    if (!this.camerasListEmitted) {
                        if (this.selectFirstDevice && this.cameraList.length > 0) {
                            this.setDeviceId(this.cameraList[0].deviceId);
                        }

                        this.$emit("xplpc-camera-list", this.cameraList);
                        this.camerasListEmitted = true;
                    }
                })
                .catch(error => this.$emit("xplpc-camera-not-supported", error));
        },
        // change to a different camera stream, like front and back camera on phones
        changeCamera(deviceId) {
            Log.d("[Camera : changeCamera] Device: " + deviceId);

            this.stop();
            this.$emit("xplpc-camera-changed", deviceId);
            this.loadCamera(deviceId);
        },
        // load the stream
        loadSrcStream(stream) {
            Log.d("[Camera : loadSrcStream]");

            if ("srcObject" in this.$refs.video) {
                // new browsers api
                this.$refs.video.srcObject = stream;
            } else {
                // old broswers
                this.source = window.HTMLMediaElement.srcObject(stream);
            }

            // emit video start/live event
            this.$refs.video.onloadedmetadata = () => {
                this.$emit("xplpc-camera-video-live", stream);
            };

            this.$emit("xplpc-camera-started", stream);
        },
        // stop the selected streamed video to change camera
        stopStreamedVideo(videoElem) {
            Log.d("[Camera : stopStreamedVideo]");

            const stream = videoElem.srcObject;
            const tracks = stream.getTracks();

            tracks.forEach(track => {
                // stops the video track
                track.stop();

                this.$emit("xplpc-camera-stopped", stream);
                this.$refs.video.srcObject = null;
                this.source = null;
            });
        },
        // stop the video
        stop() {
            Log.d("[Camera : stop]");

            if (this.$refs.video && this.$refs.video.srcObject) {
                this.stopStreamedVideo(this.$refs.video);
            }
        },
        // start the video
        async start() {
            Log.d("[Camera : start]");

            await navigator.mediaDevices.getUserMedia({ audio: false, video: true })

            if (!this.deviceId) {
                this.loadCameraList();
            }

            if (this.deviceId) {
                this.setupMedia();
                this.loadCamera(this.deviceId);
            }
        },
        // pause the video
        pause() {
            Log.d("[Camera : pause]");

            if (this.$refs.video && this.$refs.video.srcObject) {
                this.$refs.video.pause();
            }
        },
        // resume the video
        resume() {
            Log.d("[Camera : resume]");

            if (this.$refs.video && this.$refs.video.srcObject) {
                this.$refs.video.play();
            }
        },
        // process image
        processImage() {
            setInterval(async () => {
                this.getCanvas();

                const imgData = this.ctx.getImageData(0, 0, this.canvas.width, this.canvas.height);
                const dataView = XDataView.createFromArrayBuffer(imgData.data);

                const startTime = performance.now();

                const request = new XRequest(
                    "sample.image.grayscale.dataview",
                    new XParam("dataView", dataView),
                );

                await XClient.call(request);

                const elapsedTime = performance.now();
                const duration = elapsedTime - startTime;

                const processedData = XDataView.createUint8ClampedArrayFromPtr(dataView.ptr, dataView.size);
                this.ctx.putImageData(new ImageData(processedData, this.canvas.width, this.canvas.height), 0, 0);

                const preview = this.$refs.preview;
                preview.src = this.canvas.toDataURL();

                XDataView.free(dataView);

                this.cameraInfo = "Time to process: " + (duration / 1000).toFixed(3) + " seconds\nImage size: " + (dataView.size / 1024) + " kb";
            }, 16);
        },
        // test access
        testMediaAccess() {
            Log.d("[Camera : testMediaAccess]");

            const constraints = { video: true };

            if (this.resolution) {
                constraints.video = {};
                constraints.video.width = this.resolution.width;
                constraints.video.height = this.resolution.height;
            }

            navigator.mediaDevices
                .getUserMedia(constraints)
                .then(stream => {
                    // make sure to stop this media-stream
                    const tracks = stream.getTracks();

                    tracks.forEach(track => {
                        track.stop();
                    });

                    this.loadCameraList();
                })
                .catch(error => this.$emit("xplpc-camera-error", error));
        },
        // load the camera passed as index
        loadCamera(device) {
            Log.d("[Camera : loadCamera]");
            const constraints = { video: { deviceId: { exact: device } } };

            if (this.resolution) {
                constraints.video.width = this.resolution.width;
                constraints.video.height = this.resolution.height;
            }

            navigator.mediaDevices
                .getUserMedia(constraints)
                .then(stream => this.loadSrcStream(stream))
                .catch(error => this.$emit("xplpc-camera-error", error));
        },
        // capture screenshot
        capture() {
            Log.d("[Camera : capture]");
            return this.getCanvas().toDataURL(this.screenshotFormat);
        },
        // get canvas
        getCanvas() {
            // rmeove comment below for debug only
            // Log.d("[Camera : getCanvas]");

            const video = this.$refs.video;

            if (!this.ctx) {
                // calculate proportion
                this.destWidth = 240;
                this.destHeight = this.destWidth * video.videoHeight / video.videoWidth;

                if (this.destHeight > 320) {
                    this.destHeight = 320;
                    this.destWidth = this.destHeight * video.videoWidth / video.videoHeight;
                }

                // create canvas
                const canvas = this.$refs.canvas;
                canvas.width = this.destWidth;
                canvas.height = this.destHeight;
                this.canvas = canvas;

                video.width = this.destWidth;
                video.height = this.destHeight;

                // create context
                this.ctx = this.canvas.getContext("2d", { willReadFrequently: true });
            }

            const { ctx, canvas } = this;

            // draw video frame
            ctx.drawImage(video, 0, 0, this.destWidth, this.destHeight);

            return canvas;
        },
        // set device id
        setDeviceId(deviceId) {
            Log.d("[Camera : setDeviceId] Device: " + deviceId);
            this.deviceId = deviceId;
        },
        // on can play media
        canPlay() {
            Log.d("[Camera : canPlay]");
            this.getCanvas();
            this.processImage();
            this.visible = true;
        }
    }
};
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
