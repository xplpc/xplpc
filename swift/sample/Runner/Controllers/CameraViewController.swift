import AVFoundation
import os
import UIKit
import xplpc

class CameraViewController: UIViewController, AVCaptureVideoDataOutputSampleBufferDelegate {
    private let camera = CameraSession()
    private let videoOutput = AVCaptureVideoDataOutput()

    /// The orientation is written on the main thread and read for every frame on the capture queue, so it is guarded.
    private nonisolated let orientation = OSAllocatedUnfairLock<UIInterfaceOrientation?>(initialState: nil)

    private lazy var previewImage: UIImageView = {
        let iv = UIImageView()
        iv.contentMode = .scaleAspectFill
        iv.translatesAutoresizingMaskIntoConstraints = false
        return iv
    }()

    private lazy var lbOverlay: UILabel = {
        let lb = UILabel()
        lb.translatesAutoresizingMaskIntoConstraints = false
        lb.textAlignment = .center
        lb.textColor = .white
        lb.backgroundColor = UIColor(rgb: 0x000000).withAlphaComponent(0.8)
        lb.font = UIFont.systemFont(ofSize: 16)
        lb.layer.cornerRadius = 44 / 2.0
        lb.layer.masksToBounds = true
        lb.numberOfLines = 0
        return lb
    }()

    override func viewDidLoad() {
        super.viewDidLoad()

        updateOrientation()

        addCameraInput()
        addPreviewLayer()
        addVideoOutput()
    }

    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)

        camera.start()
    }

    override func viewWillDisappear(_ animated: Bool) {
        super.viewWillDisappear(animated)

        camera.stop()
    }

    nonisolated func captureOutput(_: AVCaptureOutput, didOutput sampleBuffer: CMSampleBuffer, from _: AVCaptureConnection) {
        guard let orientation = orientation.withLock({ $0 }) else {
            debugPrint("[CameraViewController : captureOutput] Orientation is invalid")
            return
        }

        guard let frame = CMSampleBufferGetImageBuffer(sampleBuffer) else {
            debugPrint("[CameraViewController : captureOutput] Unable to get image from sample buffer")
            return
        }

        let size = CVPixelBufferGetDataSize(frame)

        guard let originalImage = ImageHelper.imageFromSampleBuffer(sampleBuffer: sampleBuffer, orientation: orientation) else {
            debugPrint("[CameraViewController : captureOutput] Unable to build an image from the sample buffer")
            return
        }

        var buffer = ImageHelper.getRGBABytes(from: originalImage)

        let width = Int(originalImage.size.width)
        let height = Int(originalImage.size.height)

        let startTime = CFAbsoluteTimeGetCurrent()

        // The bytes are registered only while this closure runs, so the frame is processed with a call that answers before it returns, and the mapping changes the buffer in place.
        let processed = DataView.withByteArray(&buffer) { dataView in
            let request = Request(
                "sample.image.grayscale.dataview",
                Param("dataView", dataView)
            )

            let response: String? = Client.callSync(request)

            return response == "OK"
        }

        guard processed else {
            debugPrint("[CameraViewController : captureOutput] Unable to process the frame")
            return
        }

        let duration = Float(CFAbsoluteTimeGetCurrent() - startTime)
        let finalImage = ImageHelper.rgbaBytesToUIImage(imageData: buffer, width: width, height: height)

        Task { @MainActor in
            self.previewImage.image = finalImage
            self.lbOverlay.text = String(format: "Time to process: %.3f seconds\nImage size: %d kb", duration, size / 1024)
        }
    }

    private func addCameraInput() {
        guard let device = AVCaptureDevice.default(for: .video) else {
            debugPrint("[CameraViewController : addCameraInput] No video device found")
            return
        }

        guard let cameraInput = try? AVCaptureDeviceInput(device: device) else {
            debugPrint("[CameraViewController : addCameraInput] The video device could not be opened")
            return
        }

        camera.session.sessionPreset = AVCaptureSession.Preset.medium
        camera.session.addInput(cameraInput)
    }

    private func addPreviewLayer() {
        view.addSubview(previewImage)
        view.addSubview(lbOverlay)

        previewImage.align(with: view)

        lbOverlay.alignLeadingTrailing(with: view, constant: 16)
        lbOverlay.alignBottom(to: view.layoutGuide.bottomAnchor)
        lbOverlay.constrainHeight(88)
    }

    private func addVideoOutput() {
        videoOutput.videoSettings = [kCVPixelBufferPixelFormatTypeKey as NSString: NSNumber(value: kCVPixelFormatType_32BGRA)] as [String: Any]
        videoOutput.setSampleBufferDelegate(self, queue: DispatchQueue(label: "xplpc.camera.queue"))

        camera.session.addOutput(videoOutput)
    }

    private func updateOrientation() {
        let current = UIApplication.shared.connectedScenes
            .compactMap { $0 as? UIWindowScene }
            .first?
            .interfaceOrientation

        orientation.withLock { $0 = current }
    }

    override func viewWillTransition(to size: CGSize, with coordinator: UIViewControllerTransitionCoordinator) {
        super.viewWillTransition(to: size, with: coordinator)

        // The scene reports the new orientation only once the rotation finished, so it is read after the transition rather than before it.
        coordinator.animate(alongsideTransition: nil) { _ in
            self.updateOrientation()
        }
    }
}
