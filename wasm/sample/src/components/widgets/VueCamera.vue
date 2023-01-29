<template>
    <div class="text-center">
        <div class="camera-container text-center">
            <video
ref="video" :width="width" :height="height" :src="source" :autoplay="autoplay"
                :playsinline="playsinline" style="display: none;" @canplay="canPlay"></video>

            <img v-if="visible" ref="preview" class="camera-image" />
        </div>
    </div>
</template>

<script lang="js">
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
            ctx: null
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
                let getUserMedia =
                    navigator.getUserMedia ||
                    navigator.webkitGetUserMedia ||
                    navigator.mozGetUserMedia ||
                    navigator.msGetUserMedia ||
                    navigator.oGetUserMedia;

                // some browsers just don't implement it, so return a rejected promise with an error to keep a consistent interface
                if (!getUserMedia) {
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
                        let deviceInfo = deviceInfos[i];

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

            let stream = videoElem.srcObject;
            let tracks = stream.getTracks();

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
        start() {
            Log.d("[Camera : start]");

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
            setInterval(() => {
                this.getCanvas();

                const imgData = this.ctx.getImageData(0, 0, this.canvas.width, this.canvas.height);
                const data = imgData.data;

                for (let i = 0; i < data.length; i += 4) {
                    const avg = (data[i] + data[i + 1] + data[i + 2]) / 3;
                    data[i] = avg;
                    data[i + 1] = avg;
                    data[i + 2] = avg;
                }

                this.ctx.putImageData(imgData, 0, 0);

                const preview = this.$refs.preview;
                preview.src = this.canvas.toDataURL();
            }, 16);
        },
        // test access
        testMediaAccess() {
            Log.d("[Camera : testMediaAccess]");

            let constraints = { video: true };

            if (this.resolution) {
                constraints.video = {};
                constraints.video.height = this.resolution.height;
                constraints.video.width = this.resolution.width;
            }

            navigator.mediaDevices
                .getUserMedia(constraints)
                .then(stream => {
                    // make sure to stop this media-stream
                    let tracks = stream.getTracks();

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
            let constraints = { video: { deviceId: { exact: device } } };

            if (this.resolution) {
                constraints.video.height = this.resolution.height;
                constraints.video.width = this.resolution.width;
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

            let video = this.$refs.video;

            if (!this.ctx) {
                let canvas = document.createElement("canvas");
                canvas.width = video.videoWidth;
                canvas.height = video.videoHeight;

                this.canvas = canvas;
                this.ctx = canvas.getContext("2d", {willReadFrequently : true});
            }

            const { ctx, canvas } = this;
            ctx.drawImage(video, 0, 0, canvas.width, canvas.height);

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
</style>
