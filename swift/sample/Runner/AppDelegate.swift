import UIKit
import xplpc

@main
class AppDelegate: UIResponder, UIApplicationDelegate {
    func application(_: UIApplication, didFinishLaunchingWithOptions _: [UIApplication.LaunchOptionsKey: Any]?) -> Bool {
        Mapping.initialize()

        return true
    }
}
