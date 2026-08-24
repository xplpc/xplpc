import Foundation
import UIKit

extension CIImage {
    func orientationCorrectedImage(orientation: UIInterfaceOrientation?) -> UIImage? {
        var deviceOrientation = orientation

        if deviceOrientation == nil {
            deviceOrientation = UIApplication.shared.connectedScenes
                .compactMap { $0 as? UIWindowScene }
                .first?
                .interfaceOrientation
        }

        guard let deviceOrientation else {
            debugPrint("[CIImage : orientationCorrectedImage] No interface orientation is available")
            return nil
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
