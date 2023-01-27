import UIKit

extension UINavigationBar {
    func addShadow(shadowColor: UIColor = UIColor.black, shadowOffset: CGSize = CGSize(width: 1.0, height: 2.0), shadowOpacity: Float = 0.4, shadowRadius: CGFloat = 3.0) {
        layer.shadowColor = shadowColor.cgColor
        layer.shadowOffset = shadowOffset
        layer.shadowOpacity = shadowOpacity
        layer.shadowRadius = shadowRadius
    }
}
