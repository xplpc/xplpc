import UIKit

@IBDesignable class RoundedImage: UIImageView {
    private var customCornerRadius = false

    override func layoutSubviews() {
        super.layoutSubviews()

        updateCornerRadius()
    }

    @IBInspectable var rounded: Bool = true {
        didSet {
            updateCornerRadius()
        }
    }

    @IBInspectable var cornerRadius: CGFloat = 0.1 {
        didSet {
            customCornerRadius = true
            updateCornerRadius()
        }
    }

    func updateCornerRadius() {
        if customCornerRadius {
            layer.cornerRadius = rounded ? cornerRadius : 0
        } else {
            layer.cornerRadius = rounded ? (size.height / 2) : 0
        }

        layer.masksToBounds = rounded ? true : false
    }
}

@IBDesignable class RoundedView: UIView {
    private var customCornerRadius = false

    override func layoutSubviews() {
        super.layoutSubviews()

        updateCornerRadius()
    }

    @IBInspectable var rounded: Bool = true {
        didSet {
            updateCornerRadius()
        }
    }

    @IBInspectable var cornerRadius: CGFloat = 0.1 {
        didSet {
            customCornerRadius = true
            updateCornerRadius()
        }
    }

    func updateCornerRadius() {
        if customCornerRadius {
            layer.cornerRadius = rounded ? cornerRadius : 0
        } else {
            layer.cornerRadius = rounded ? (size.height / 2) : 0
        }

        layer.masksToBounds = rounded ? true : false
    }
}

@IBDesignable class RoundedButton: UIButton {
    private var customCornerRadius = false

    override func layoutSubviews() {
        super.layoutSubviews()

        updateCornerRadius()
    }

    @IBInspectable var rounded: Bool = true {
        didSet {
            updateCornerRadius()
        }
    }

    @IBInspectable var cornerRadius: CGFloat = 0.1 {
        didSet {
            customCornerRadius = true
            updateCornerRadius()
        }
    }

    func updateCornerRadius() {
        if customCornerRadius {
            layer.cornerRadius = rounded ? cornerRadius : 0
        } else {
            layer.cornerRadius = rounded ? (size.height / 2) : 0
        }

        layer.masksToBounds = rounded ? true : false
    }
}
