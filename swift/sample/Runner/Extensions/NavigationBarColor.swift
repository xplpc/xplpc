import UIKit

extension UINavigationBar {
    func setBarColor(textColor: UIColor, backgroundColor: UIColor) {
        let navBarAppearance = UINavigationBarAppearance()
        navBarAppearance.configureWithOpaqueBackground()
        navBarAppearance.titleTextAttributes = [.foregroundColor: textColor]
        navBarAppearance.largeTitleTextAttributes = [.foregroundColor: textColor]
        navBarAppearance.backgroundColor = backgroundColor
        standardAppearance = navBarAppearance
        scrollEdgeAppearance = navBarAppearance
    }
}
