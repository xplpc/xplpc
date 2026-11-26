import UIKit

extension UIView {
    var size: CGSize {
        get { frame.size }
        set { frame.size = newValue }
    }
}

extension UIView {
    @discardableResult
    func align(with view: UIView, constant: CGFloat = 0.0) -> [NSLayoutConstraint] {
        translatesAutoresizingMaskIntoConstraints = false
        let constraints = [
            leftAnchor.constraint(equalTo: view.leftAnchor, constant: constant),
            rightAnchor.constraint(equalTo: view.rightAnchor, constant: -constant),
            topAnchor.constraint(equalTo: view.topAnchor, constant: constant),
            bottomAnchor.constraint(equalTo: view.bottomAnchor, constant: -constant),
        ]
        NSLayoutConstraint.activate(constraints)
        return constraints
    }

    @discardableResult
    func alignLeadingTrailing(with view: UIView, constant: CGFloat = 0.0) -> [NSLayoutConstraint] {
        translatesAutoresizingMaskIntoConstraints = false
        let constraints = [
            leftAnchor.constraint(equalTo: view.leftAnchor, constant: constant),
            rightAnchor.constraint(equalTo: view.rightAnchor, constant: -constant),
        ]
        NSLayoutConstraint.activate(constraints)
        return constraints
    }

    @discardableResult
    func alignTop(to anchor: NSLayoutYAxisAnchor, constant: CGFloat = 0.0,
                  priority: UILayoutPriority = .required, isActive: Bool = true) -> NSLayoutConstraint
    {
        translatesAutoresizingMaskIntoConstraints = false
        let constraint = topAnchor.constraint(equalTo: anchor, constant: constant)
        constraint.priority = priority
        constraint.isActive = isActive
        return constraint
    }

    @discardableResult
    func alignTop(with view: UIView, constant: CGFloat = 0.0,
                  priority: UILayoutPriority = .required, isActive: Bool = true) -> NSLayoutConstraint
    {
        alignTop(to: view.topAnchor, constant: constant, priority: priority, isActive: isActive)
    }

    @discardableResult
    func alignBottom(to anchor: NSLayoutYAxisAnchor, constant: CGFloat = 0.0,
                     priority: UILayoutPriority = .required, isActive: Bool = true) -> NSLayoutConstraint
    {
        translatesAutoresizingMaskIntoConstraints = false
        let constraint = bottomAnchor.constraint(equalTo: anchor, constant: constant)
        constraint.priority = priority
        constraint.isActive = isActive
        return constraint
    }

    @discardableResult
    func alignBottom(with view: UIView, constant: CGFloat = 0.0,
                     priority: UILayoutPriority = .required, isActive: Bool = true) -> NSLayoutConstraint
    {
        alignBottom(to: view.bottomAnchor, constant: constant, priority: priority, isActive: isActive)
    }

    @discardableResult
    func alignLeading(to anchor: NSLayoutXAxisAnchor, constant: CGFloat = 0.0,
                      priority: UILayoutPriority = .required, isActive: Bool = true) -> NSLayoutConstraint
    {
        translatesAutoresizingMaskIntoConstraints = false
        let constraint = leadingAnchor.constraint(equalTo: anchor, constant: constant)
        constraint.isActive = isActive
        constraint.priority = priority
        return constraint
    }

    @discardableResult
    func alignLeading(with view: UIView, constant: CGFloat = 0.0,
                      priority: UILayoutPriority = .required, isActive: Bool = true) -> NSLayoutConstraint
    {
        alignLeading(to: view.leadingAnchor, constant: constant, priority: priority, isActive: isActive)
    }

    @discardableResult
    func alignTrailing(to anchor: NSLayoutXAxisAnchor, constant: CGFloat = 0.0,
                       priority: UILayoutPriority = .required, isActive: Bool = true) -> NSLayoutConstraint
    {
        translatesAutoresizingMaskIntoConstraints = false
        let constraint = trailingAnchor.constraint(equalTo: anchor, constant: constant)
        constraint.isActive = isActive
        constraint.priority = priority
        return constraint
    }

    @discardableResult
    func alignTrailing(with view: UIView, constant: CGFloat = 0.0,
                       priority: UILayoutPriority = .required, isActive: Bool = true) -> NSLayoutConstraint
    {
        alignTrailing(to: view.trailingAnchor, constant: constant, priority: priority, isActive: isActive)
    }

    @discardableResult
    func centerVertical(with view: UIView, constant: CGFloat = 0.0,
                        priority: UILayoutPriority = .required, isActive: Bool = true) -> NSLayoutConstraint
    {
        translatesAutoresizingMaskIntoConstraints = false
        let constraint = centerYAnchor.constraint(equalTo: view.centerYAnchor, constant: constant)
        constraint.isActive = isActive
        constraint.priority = priority
        return constraint
    }

    @discardableResult
    func centerHorizontal(with view: UIView, constant: CGFloat = 0.0,
                          priority: UILayoutPriority = .required, isActive: Bool = true) -> NSLayoutConstraint
    {
        translatesAutoresizingMaskIntoConstraints = false
        let constraint = centerXAnchor.constraint(equalTo: view.centerXAnchor, constant: constant)
        constraint.isActive = isActive
        constraint.priority = priority
        return constraint
    }

    @discardableResult
    func constrainHeight(_ value: CGFloat, priority: UILayoutPriority = .required,
                         isActive: Bool = true) -> NSLayoutConstraint
    {
        let constraint = heightAnchor.constraint(equalToConstant: value)
        constraint.isActive = isActive
        constraint.priority = priority
        return constraint
    }

    @discardableResult
    func constrainWidth(_ value: CGFloat, priority: UILayoutPriority = .required,
                        isActive: Bool = true) -> NSLayoutConstraint
    {
        let constraint = widthAnchor.constraint(equalToConstant: value)
        constraint.isActive = isActive
        constraint.priority = priority
        return constraint
    }

    @discardableResult
    func constrainWidth(to view: UIView, priority: UILayoutPriority = .required,
                        isActive: Bool = true) -> NSLayoutConstraint
    {
        let constraint = widthAnchor.constraint(equalTo: view.widthAnchor)
        constraint.isActive = isActive
        constraint.priority = priority
        return constraint
    }

    @discardableResult
    func constrainHeight(to view: UIView, priority: UILayoutPriority = .required,
                         isActive: Bool = true) -> NSLayoutConstraint
    {
        let constraint = heightAnchor.constraint(equalTo: view.heightAnchor)
        constraint.isActive = isActive
        constraint.priority = priority
        return constraint
    }
}

extension UIView {
    var layoutGuide: UILayoutGuide {
        safeAreaLayoutGuide
    }
}
