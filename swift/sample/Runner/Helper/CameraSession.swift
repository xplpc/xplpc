import AVFoundation

/// The session is configured before it first starts and is only started and stopped on its own serial queue afterwards, which is what the unchecked conformance asserts.
final class CameraSession: @unchecked Sendable {
    let session = AVCaptureSession()

    private let queue = DispatchQueue(label: "xplpc.camera.session")

    func start() {
        // Starting blocks until the camera is running, so it never happens on the thread that drew the screen.
        queue.async {
            guard !self.session.isRunning else {
                return
            }

            self.session.startRunning()
        }
    }

    func stop() {
        queue.async {
            guard self.session.isRunning else {
                return
            }

            self.session.stopRunning()
        }
    }
}
