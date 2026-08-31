import UIKit

extension UINavigationBar {
    func setBarColor(textColor: UIColor, backgroundColor: UIColor) {
        let backItemAppearance = UIBarButtonItemAppearance()
        backItemAppearance.normal.titleTextAttributes = [.foregroundColor: UIColor.white]

        let backItemImage = UIImage(systemName: "chevron.backward")?.withTintColor(.white, renderingMode: .alwaysOriginal) // fix indicator color

        let navBarAppearance = UINavigationBarAppearance()
        navBarAppearance.configureWithOpaqueBackground()
        navBarAppearance.titleTextAttributes = [.foregroundColor: textColor]
        navBarAppearance.largeTitleTextAttributes = [.foregroundColor: textColor]
        navBarAppearance.backgroundColor = backgroundColor
        navBarAppearance.backButtonAppearance = backItemAppearance
        navBarAppearance.setBackIndicatorImage(backItemImage, transitionMaskImage: backItemImage)

        standardAppearance = navBarAppearance
        scrollEdgeAppearance = navBarAppearance
    }
}
