import AVFoundation
import UIKit
import xplpc

class CameraViewController: UIViewController, AVCaptureVideoDataOutputSampleBufferDelegate {
    private let captureSession = AVCaptureSession()
    private let videoOutput = AVCaptureVideoDataOutput()
    private var orientation: UIInterfaceOrientation?

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

        guard !captureSession.isRunning else {
            return
        }

        DispatchQueue.global(qos: .background).async {
            self.captureSession.startRunning()
        }
    }

    override func viewWillDisappear(_ animated: Bool) {
        super.viewWillDisappear(animated)

        guard captureSession.isRunning else {
            return
        }

        DispatchQueue.global(qos: .background).async {
            self.captureSession.stopRunning()
        }
    }

    func captureOutput(_: AVCaptureOutput, didOutput sampleBuffer: CMSampleBuffer, from _: AVCaptureConnection) {
        guard let orientation else {
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

        // The buffer is processed in place while the view is alive.
        let startTime = CFAbsoluteTimeGetCurrent()
        var processedBytes: [UInt8] = []

        DataView.withByteArray(&buffer) { dataView in
            let request = Request(
                "sample.image.grayscale.dataview",
                Param("dataView", dataView)
            )

            Client.call(request) { (response: String?) in
                guard response != nil else {
                    debugPrint("[CameraViewController : captureOutput] Unable to get response data")
                    return
                }

                processedBytes = ByteArrayHelper.createFromDataView(dataView)
            }
        }

        guard !processedBytes.isEmpty else {
            return
        }

        let duration = Float(CFAbsoluteTimeGetCurrent() - startTime)
        let finalImage = ImageHelper.rgbaBytesToUIImage(imageData: processedBytes, width: width, height: height)

        DispatchQueue.main.async {
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

        captureSession.sessionPreset = AVCaptureSession.Preset.medium
        captureSession.addInput(cameraInput)
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

        captureSession.addOutput(videoOutput)
    }

    private func updateOrientation() {
        orientation = UIApplication.shared.connectedScenes
            .compactMap { $0 as? UIWindowScene }
            .first?
            .interfaceOrientation
    }

    override func viewWillTransition(to size: CGSize, with coordinator: UIViewControllerTransitionCoordinator) {
        super.viewWillTransition(to: size, with: coordinator)
        updateOrientation()
    }
}
