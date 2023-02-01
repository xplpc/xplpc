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

        DispatchQueue.global(qos: .background).async {
            self.captureSession.startRunning()
        }
    }

    func captureOutput(_: AVCaptureOutput, didOutput sampleBuffer: CMSampleBuffer, from _: AVCaptureConnection) {
        // device orientation
        guard let orientation else {
            debugPrint("[CameraViewController : captureOutput] Orientation is invalid")
            return
        }

        // single frame
        guard let frame = CMSampleBufferGetImageBuffer(sampleBuffer) else {
            debugPrint("[CameraViewController : captureOutput] Unable to get image from sample buffer")
            return
        }

        // image size
        #if DEBUG
            let size = CVPixelBufferGetDataSize(frame)
            print("[CameraViewController : captureOutput] Original image size is: " + String(size / 1024) + " kb")
        #endif

        // original image
        let originalImage = ImageHelper.imageFromSampleBuffer(sampleBuffer: sampleBuffer, orientation: orientation)
        var buffer = ImageHelper.getRGBABytes(from: originalImage)

        // image dimension
        let width = Int(originalImage.size.width)
        let height = Int(originalImage.size.height)

        // data view
        let dataView = DataView.createFromByteArray(&buffer)

        // convert
        let startTime = CFAbsoluteTimeGetCurrent()

        let request = Request(
            "sample.image.grayscale.dataview",
            Param("dataView", dataView)
        )

        RemoteClient.call(request) { (response: String?) in
            guard response != nil else {
                debugPrint("[CameraViewController : captureOutput] Unable to get response data")
                return
            }

            let elapsedTime = CFAbsoluteTimeGetCurrent() - startTime
            let duration = Float(elapsedTime)

            // draw image
            let finalImage = ImageHelper.rgbaBytesToUIImage(imageData: buffer, width: width, height: height)

            DispatchQueue.main.async {
                self.previewImage.image = finalImage
                self.lbOverlay.text = String(format: "Time to process: %.3f seconds\nImage size: %d kb", duration, size / 1024)
            }
        }
    }

    private func addCameraInput() {
        let device = AVCaptureDevice.default(for: .video)!
        let cameraInput = try! AVCaptureDeviceInput(device: device)

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
        orientation = UIApplication.shared.windows.first?.windowScene?.interfaceOrientation
    }

    override func viewWillTransition(to size: CGSize, with coordinator: UIViewControllerTransitionCoordinator) {
        super.viewWillTransition(to: size, with: coordinator)
        updateOrientation()
    }
}
