import UIKit

class RoundedButton: UIButton {
    override func layoutSubviews() {
        super.layoutSubviews()

        layer.cornerRadius = size.height / 2
        layer.masksToBounds = true
    }
}
