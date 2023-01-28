import Foundation
import UIKit

extension CIImage {
    func orientationCorrectedImage(orientation: UIInterfaceOrientation?) -> UIImage? {
        var deviceOrientation = orientation

        if deviceOrientation == nil {
            guard let currentOrientation = UIApplication.shared.windows.first?.windowScene?.interfaceOrientation else {
                #if DEBUG
                    fatalError("[CIImage : orientationCorrectedImage] Could not obtain UIInterfaceOrientation from a valid windowScene")
                #else
                    return nil
                #endif
            }

            deviceOrientation = currentOrientation
        }

        guard let deviceOrientation else {
            #if DEBUG
                fatalError("[CIImage : orientationCorrectedImage] Orientation is invalid")
            #else
                return nil
            #endif
        }

        var imageOrientation = UIImage.Orientation.up

        switch deviceOrientation {
        case UIInterfaceOrientation.portrait:
            imageOrientation = UIImage.Orientation.right
        case UIInterfaceOrientation.landscapeLeft:
            imageOrientation = UIImage.Orientation.down
        case UIInterfaceOrientation.landscapeRight:
            imageOrientation = UIImage.Orientation.up
        case UIInterfaceOrientation.portraitUpsideDown:
            imageOrientation = UIImage.Orientation.left
        default:
            break
        }

        var w = extent.size.width
        var h = extent.size.height

        if imageOrientation == .left || imageOrientation == .right || imageOrientation == .leftMirrored || imageOrientation == .rightMirrored {
            swap(&w, &h)
        }

        UIGraphicsBeginImageContext(CGSize(width: w, height: h))
        UIImage(ciImage: self, scale: 1.0, orientation: imageOrientation).draw(in: CGRect(x: 0, y: 0, width: w, height: h))

        let uiImage: UIImage? = UIGraphicsGetImageFromCurrentImageContext()
        UIGraphicsEndImageContext()

        return uiImage
    }
}
