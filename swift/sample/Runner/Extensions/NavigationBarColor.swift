import UIKit

extension UINavigationBar {
    func setBarColor(textColor: UIColor, backgroundColor: UIColor) {
        let backItemAppearance = UIBarButtonItemAppearance()
        backItemAppearance.normal.titleTextAttributes = [.foregroundColor: UIColor.white]

        // The back indicator is drawn as an image, so it is tinted here rather than following the text color.
        let backItemImage = UIImage(systemName: "chevron.backward")?.withTintColor(.white, renderingMode: .alwaysOriginal)

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
