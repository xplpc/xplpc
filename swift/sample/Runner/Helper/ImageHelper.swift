import AVFoundation
import UIKit

class ImageHelper {
    static func imageFromSampleBuffer(sampleBuffer: CMSampleBuffer, orientation: UIInterfaceOrientation) -> UIImage? {
        // A sample buffer that carries no image and a context that fails to draw are both ordinary on a live camera.

        guard let imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer) else {
            return nil
        }

        CVPixelBufferLockBaseAddress(imageBuffer, CVPixelBufferLockFlags.readOnly)

        defer {
            CVPixelBufferUnlockBaseAddress(imageBuffer, CVPixelBufferLockFlags.readOnly)
        }

        let baseAddress = CVPixelBufferGetBaseAddress(imageBuffer)
        let bytesPerRow = CVPixelBufferGetBytesPerRow(imageBuffer)
        let width = CVPixelBufferGetWidth(imageBuffer)
        let height = CVPixelBufferGetHeight(imageBuffer)

        let colorSpace = CGColorSpaceCreateDeviceRGB()

        var bitmapInfo: UInt32 = CGBitmapInfo.byteOrder32Little.rawValue
        bitmapInfo |= CGImageAlphaInfo.premultipliedFirst.rawValue & CGBitmapInfo.alphaInfoMask.rawValue

        guard let context = CGContext(data: baseAddress, width: width, height: height, bitsPerComponent: 8, bytesPerRow: bytesPerRow, space: colorSpace, bitmapInfo: bitmapInfo) else {
            return nil
        }

        guard let quartzImage = context.makeImage() else {
            return nil
        }

        return CIImage(cgImage: quartzImage).orientationCorrectedImage(orientation: orientation)
    }

    static func getRGBABytes(from image: UIImage) -> [UInt8] {
        // The image is drawn into rows of exactly four bytes a pixel in rgba order, which is the layout the mapping works on and the one rgbaBytesToUIImage rebuilds from.

        guard let cgImage = image.cgImage else {
            return []
        }

        let width = cgImage.width
        let height = cgImage.height
        var imageData = [UInt8](repeating: 0, count: width * height * 4)

        let drawn = imageData.withUnsafeMutableBytes { bytes -> Bool in
            guard let context = CGContext(
                data: bytes.baseAddress,
                width: width,
                height: height,
                bitsPerComponent: 8,
                bytesPerRow: width * 4,
                space: CGColorSpaceCreateDeviceRGB(),
                bitmapInfo: CGImageAlphaInfo.premultipliedLast.rawValue
            ) else {
                return false
            }

            context.draw(cgImage, in: CGRect(x: 0, y: 0, width: width, height: height))

            return true
        }

        return drawn ? imageData : []
    }

    static func rgbaBytesToUIImage(imageData: [UInt8], width: Int, height: Int) -> UIImage? {
        let bitsPerComponent = 8
        let bitsPerPixel = 32
        let bytesPerRow = width * 4
        let colorSpace = CGColorSpaceCreateDeviceRGB()
        let bitmapInfo = CGBitmapInfo(rawValue: CGImageAlphaInfo.premultipliedLast.rawValue)

        guard let provider = CGDataProvider(data: Data(imageData) as CFData) else {
            return nil
        }

        guard let cgImage = CGImage(
            width: width,
            height: height,
            bitsPerComponent: bitsPerComponent,
            bitsPerPixel: bitsPerPixel,
            bytesPerRow: bytesPerRow,
            space: colorSpace,
            bitmapInfo: bitmapInfo,
            provider: provider,
            decode: nil,
            shouldInterpolate: true,
            intent: .defaultIntent
        ) else {
            return nil
        }

        return UIImage(cgImage: cgImage)
    }
}
