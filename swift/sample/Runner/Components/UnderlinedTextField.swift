import UIKit

class UnderlinedTextField: UITextField {
    private let defaultUnderlineColor = UIColor.black
    private let bottomLine = UIView()

    override init(frame: CGRect) {
        super.init(frame: frame)

        borderStyle = .none
        bottomLine.translatesAutoresizingMaskIntoConstraints = false
        bottomLine.backgroundColor = defaultUnderlineColor

        addSubview(bottomLine)
        bottomLine.topAnchor.constraint(equalTo: bottomAnchor, constant: 1).isActive = true
        bottomLine.leadingAnchor.constraint(equalTo: leadingAnchor).isActive = true
        bottomLine.trailingAnchor.constraint(equalTo: trailingAnchor).isActive = true
        bottomLine.heightAnchor.constraint(equalToConstant: 1).isActive = true
    }

    @available(*, unavailable)
    required init?(coder _: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    public func setUnderlineColor(color: UIColor = .red) {
        bottomLine.backgroundColor = color
    }

    public func setDefaultUnderlineColor() {
        bottomLine.backgroundColor = defaultUnderlineColor
    }
}
