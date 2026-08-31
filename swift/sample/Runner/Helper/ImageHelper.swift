import AVFoundation
import Foundation
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
        guard let cgImage = image.cgImage else {
            return []
        }

        let width = cgImage.width
        let height = cgImage.height
        let bytesPerRow = cgImage.bytesPerRow

        var imageData = [UInt8](repeating: 0, count: height * bytesPerRow)

        guard let dataProvider = cgImage.dataProvider,
              let data = dataProvider.data,
              let dataPointer = CFDataGetBytePtr(data)
        else {
            return []
        }

        for row in 0 ..< height {
            for col in 0 ..< width {
                let offset = bytesPerRow * row + col * 4
                imageData[offset] = dataPointer[offset]
                imageData[offset + 1] = dataPointer[offset + 1]
                imageData[offset + 2] = dataPointer[offset + 2]
                imageData[offset + 3] = dataPointer[offset + 3]
            }
        }

        return imageData
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
