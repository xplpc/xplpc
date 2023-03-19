import UIKit
import xplpc

@main
class AppDelegate: UIResponder, UIApplicationDelegate {
    var window: UIWindow?

    var controller: MainViewController?
    var nav: UINavigationController?

    func application(_: UIApplication, didFinishLaunchingWithOptions _: [UIApplication.LaunchOptionsKey: Any]?) -> Bool {
        initializeXPLPC()

        UIDevice.current.isBatteryMonitoringEnabled = true

        controller = MainViewController()
        nav = UINavigationController(rootViewController: controller!)

        window = UIWindow(frame: UIScreen.main.bounds)
        window?.rootViewController = nav
        window?.makeKeyAndVisible()

        return true
    }

    private func initializeXPLPC() {
        Mapping.initialize()
    }
}
