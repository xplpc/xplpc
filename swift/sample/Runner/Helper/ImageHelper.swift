import AVFoundation
import Foundation
import UIKit

class ImageHelper {
    static func imageFromSampleBuffer(sampleBuffer: CMSampleBuffer, orientation: UIInterfaceOrientation) -> UIImage {
        // get a sample buffer's for the media data
        let imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer)

        // lock the base address of the pixel buffer
        CVPixelBufferLockBaseAddress(imageBuffer!, CVPixelBufferLockFlags.readOnly)

        // get the number of bytes per row for the pixel buffer
        let baseAddress = CVPixelBufferGetBaseAddress(imageBuffer!)

        // get the number of bytes per row for the pixel buffer
        let bytesPerRow = CVPixelBufferGetBytesPerRow(imageBuffer!)

        // get the pixel buffer width and height
        let width = CVPixelBufferGetWidth(imageBuffer!)
        let height = CVPixelBufferGetHeight(imageBuffer!)

        // create a device-dependent RGB color space
        let colorSpace = CGColorSpaceCreateDeviceRGB()

        // create a bitmap graphics context with the sample buffer data
        var bitmapInfo: UInt32 = CGBitmapInfo.byteOrder32Little.rawValue
        bitmapInfo |= CGImageAlphaInfo.premultipliedFirst.rawValue & CGBitmapInfo.alphaInfoMask.rawValue

        let context = CGContext(data: baseAddress, width: width, height: height, bitsPerComponent: 8, bytesPerRow: bytesPerRow, space: colorSpace, bitmapInfo: bitmapInfo)

        // create a quartz image from the pixel data in the bitmap graphics context
        let quartzImage = context?.makeImage()

        // unlock the pixel buffer
        CVPixelBufferUnlockBaseAddress(imageBuffer!, CVPixelBufferLockFlags.readOnly)

        // create an image object from the Quartz image
        return CIImage(cgImage: quartzImage!).orientationCorrectedImage(orientation: orientation)!
    }

    static func getRGBABytes(from image: UIImage) -> [UInt8] {
        // convert the UIImage to a CGImage
        guard let cgImage = image.cgImage else {
            return []
        }

        // get the width, height and bytesPerRow of the image
        let width = cgImage.width
        let height = cgImage.height
        let bytesPerRow = cgImage.bytesPerRow

        // create a uint8_t array to store the image data
        var imageData = [UInt8](repeating: 0, count: height * bytesPerRow)

        // get the image data from the CGImage
        let dataProvider = cgImage.dataProvider!
        let data = dataProvider.data!
        let dataPointer = CFDataGetBytePtr(data)

        // copy the RGBA bytes into the uint8_t array
        for row in 0 ..< height {
            for col in 0 ..< width {
                let offset = bytesPerRow * row + col * 4
                imageData[offset] = dataPointer![offset]
                imageData[offset + 1] = dataPointer![offset + 1]
                imageData[offset + 2] = dataPointer![offset + 2]
                imageData[offset + 3] = dataPointer![offset + 3]
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
        let provider = CGDataProvider(data: Data(imageData) as CFData)
        let cgImage = CGImage(width: width, height: height, bitsPerComponent: bitsPerComponent, bitsPerPixel: bitsPerPixel, bytesPerRow: bytesPerRow, space: colorSpace, bitmapInfo: bitmapInfo, provider: provider!, decode: nil, shouldInterpolate: true, intent: .defaultIntent)

        return UIImage(cgImage: cgImage!)
    }
}
