import UIKit
import xplpc

class MainViewController: UIViewController, UITextFieldDelegate {
    private lazy var scrollContainer: UIScrollView = {
        let view = UIScrollView()
        view.translatesAutoresizingMaskIntoConstraints = false
        return view
    }()

    private lazy var container: UIView = {
        let view = UIView()
        view.translatesAutoresizingMaskIntoConstraints = false
        return view
    }()

    private lazy var lbTitleBatteryLevel: UILabel = {
        let lb = UILabel()
        lb.text = "BATTERY LEVEL"
        lb.translatesAutoresizingMaskIntoConstraints = false
        lb.textAlignment = .center
        lb.textColor = .black
        lb.font = UIFont.boldSystemFont(ofSize: 20)
        return lb
    }()

    private lazy var lbBatteryLevelMessage: UILabel = {
        let lb = UILabel()
        lb.text = "Press Button To Execute"
        lb.translatesAutoresizingMaskIntoConstraints = false
        lb.textAlignment = .center
        lb.textColor = .black
        lb.font = UIFont.systemFont(ofSize: 14)
        return lb
    }()

    private lazy var btSubmitForBatteryLevel: RoundedButton = {
        let bt = RoundedButton(type: .system)
        bt.setTitle("SUBMIT", for: .normal)
        bt.setTitleColor(.white, for: .normal)
        bt.translatesAutoresizingMaskIntoConstraints = false
        bt.backgroundColor = UIColor(rgb: 0x1565C0)
        bt.titleLabel?.font = UIFont.systemFont(ofSize: 14)
        bt.addTarget(self, action: #selector(btSubmitForBatteryLevelClicked), for: .touchUpInside)
        return bt
    }()

    private lazy var divider1: UIView = {
        let view = UIView()
        view.translatesAutoresizingMaskIntoConstraints = false
        view.size.height = 1
        view.backgroundColor = UIColor(rgb: 0xDDDDDD)
        return view
    }()

    private lazy var lbTitleLogin: UILabel = {
        let lb = UILabel()
        lb.text = "LOGIN"
        lb.translatesAutoresizingMaskIntoConstraints = false
        lb.textAlignment = .center
        lb.textColor = .black
        lb.font = UIFont.boldSystemFont(ofSize: 20)
        return lb
    }()

    private lazy var edUsername: UnderlinedTextField = {
        let ed = UnderlinedTextField()
        ed.translatesAutoresizingMaskIntoConstraints = false
        ed.textColor = .black
        ed.font = UIFont.systemFont(ofSize: 16)
        ed.attributedPlaceholder = NSAttributedString(string: "Your username", attributes: [NSAttributedString.Key.foregroundColor: UIColor(rgb: 0xCCCCCC)])
        ed.setUnderlineColor(color: UIColor(rgb: 0xCCCCCC))
        ed.autocapitalizationType = .none
        return ed
    }()

    private lazy var edPassword: UnderlinedTextField = {
        let ed = UnderlinedTextField()
        ed.translatesAutoresizingMaskIntoConstraints = false
        ed.textColor = .black
        ed.font = UIFont.systemFont(ofSize: 16)
        ed.attributedPlaceholder = NSAttributedString(string: "Your password", attributes: [NSAttributedString.Key.foregroundColor: UIColor(rgb: 0xCCCCCC)])
        ed.setUnderlineColor(color: UIColor(rgb: 0xCCCCCC))
        ed.isSecureTextEntry = true
        return ed
    }()

    private lazy var swRemember: UISwitch = {
        let sw = UISwitch()
        sw.translatesAutoresizingMaskIntoConstraints = false
        return sw
    }()

    private lazy var lbRememberHint: UILabel = {
        let lb = UILabel()
        lb.text = "Remember"
        lb.translatesAutoresizingMaskIntoConstraints = false
        lb.textAlignment = .left
        lb.textColor = UIColor(rgb: 0x888888)
        lb.font = UIFont.systemFont(ofSize: 14)
        return lb
    }()

    private lazy var lbFormMessage: UILabel = {
        let lb = UILabel()
        lb.text = "Press Button To Execute"
        lb.translatesAutoresizingMaskIntoConstraints = false
        lb.textAlignment = .center
        lb.textColor = .black
        lb.font = UIFont.systemFont(ofSize: 14)
        return lb
    }()

    private lazy var btSubmitForLogin: RoundedButton = {
        let bt = RoundedButton(type: .system)
        bt.setTitle("SUBMIT", for: .normal)
        bt.setTitleColor(.white, for: .normal)
        bt.translatesAutoresizingMaskIntoConstraints = false
        bt.backgroundColor = UIColor(rgb: 0x1565C0)
        bt.titleLabel?.font = UIFont.systemFont(ofSize: 14)
        bt.addTarget(self, action: #selector(btSubmitForLoginClicked), for: .touchUpInside)
        return bt
    }()

    private lazy var divider2: UIView = {
        let view = UIView()
        view.translatesAutoresizingMaskIntoConstraints = false
        view.size.height = 1
        view.backgroundColor = UIColor(rgb: 0xDDDDDD)
        return view
    }()

    private lazy var lbTitleCamera: UILabel = {
        let lb = UILabel()
        lb.text = "CAMERA"
        lb.translatesAutoresizingMaskIntoConstraints = false
        lb.textAlignment = .center
        lb.textColor = .black
        lb.font = UIFont.boldSystemFont(ofSize: 20)
        return lb
    }()

    private lazy var btSubmitForCamera: RoundedButton = {
        let bt = RoundedButton(type: .system)
        bt.setTitle("OPEN", for: .normal)
        bt.setTitleColor(.white, for: .normal)
        bt.translatesAutoresizingMaskIntoConstraints = false
        bt.backgroundColor = UIColor(rgb: 0x1565C0)
        bt.titleLabel?.font = UIFont.systemFont(ofSize: 14)
        bt.addTarget(self, action: #selector(btSubmitForCameraClicked), for: .touchUpInside)
        return bt
    }()

    override func viewDidLoad() {
        super.viewDidLoad()

        // general
        title = "XPLPC"
        view.backgroundColor = .white
        navigationController?.navigationBar.isTranslucent = false
        navigationController?.navigationBar.isOpaque = true
        navigationController?.navigationBar.setBarColor(textColor: .white, backgroundColor: UIColor(rgb: 0x1565C0))
        navigationController?.navigationBar.addShadow()

        // keyboard
        hideKeyboardWhenTappedAround()
        edUsername.delegate = self
        edPassword.delegate = self

        // components
        view.addSubview(scrollContainer)
        scrollContainer.addSubview(container)
        container.addSubview(lbTitleBatteryLevel)
        container.addSubview(lbBatteryLevelMessage)
        container.addSubview(btSubmitForBatteryLevel)
        container.addSubview(divider1)
        container.addSubview(lbTitleLogin)
        container.addSubview(edUsername)
        container.addSubview(edPassword)
        container.addSubview(swRemember)
        container.addSubview(lbRememberHint)
        container.addSubview(lbFormMessage)
        container.addSubview(btSubmitForLogin)
        container.addSubview(divider2)
        container.addSubview(lbTitleCamera)
        container.addSubview(btSubmitForCamera)

        // layout
        scrollContainer.align(with: view)
        scrollContainer.constrainWidth(to: view)
        scrollContainer.constrainHeight(to: view)

        container.align(with: scrollContainer)
        container.constrainWidth(to: view)
        container.alignBottom(with: btSubmitForCamera, constant: 20) // always last element

        lbTitleBatteryLevel.alignTop(with: container, constant: 20)
        lbTitleBatteryLevel.alignLeadingTrailing(with: container)

        lbBatteryLevelMessage.alignTop(to: lbTitleBatteryLevel.bottomAnchor, constant: 20)
        lbBatteryLevelMessage.alignLeadingTrailing(with: container)

        btSubmitForBatteryLevel.alignTop(to: lbBatteryLevelMessage.bottomAnchor, constant: 30)
        btSubmitForBatteryLevel.centerHorizontal(with: container)
        btSubmitForBatteryLevel.constrainWidth(120)
        btSubmitForBatteryLevel.constrainHeight(44)

        divider1.alignTop(to: btSubmitForBatteryLevel.bottomAnchor, constant: 26)
        divider1.alignLeadingTrailing(with: container, constant: 16)
        divider1.constrainHeight(1)

        lbTitleLogin.alignTop(to: divider1.bottomAnchor, constant: 26)
        lbTitleLogin.alignLeadingTrailing(with: container)

        edUsername.alignTop(to: lbTitleLogin.bottomAnchor, constant: 20)
        edUsername.alignLeadingTrailing(with: container, constant: 60)
        edUsername.constrainHeight(44)

        edPassword.alignTop(to: edUsername.bottomAnchor, constant: 24)
        edPassword.alignLeadingTrailing(with: container, constant: 60)
        edPassword.constrainHeight(44)

        swRemember.alignTop(to: edPassword.bottomAnchor, constant: 24)
        swRemember.alignLeading(with: edPassword)
        swRemember.constrainHeight(30)

        lbRememberHint.centerVertical(with: swRemember)
        lbRememberHint.alignLeading(to: swRemember.trailingAnchor, constant: 20)
        lbRememberHint.alignTrailing(with: edPassword)

        lbFormMessage.alignTop(to: swRemember.bottomAnchor, constant: 20)
        lbFormMessage.alignLeadingTrailing(with: container)

        btSubmitForLogin.alignTop(to: lbFormMessage.bottomAnchor, constant: 30)
        btSubmitForLogin.centerHorizontal(with: container)
        btSubmitForLogin.constrainWidth(120)
        btSubmitForLogin.constrainHeight(44)

        divider2.alignTop(to: btSubmitForLogin.bottomAnchor, constant: 26)
        divider2.alignLeadingTrailing(with: container, constant: 16)
        divider2.constrainHeight(1)

        lbTitleCamera.alignTop(with: divider2, constant: 20)
        lbTitleCamera.alignLeadingTrailing(with: container)

        btSubmitForCamera.alignTop(to: lbTitleCamera.bottomAnchor, constant: 30)
        btSubmitForCamera.centerHorizontal(with: container)
        btSubmitForCamera.constrainWidth(120)
        btSubmitForCamera.constrainHeight(44)
    }

    @objc func btSubmitForBatteryLevelClicked() {
        let request = Request("platform.battery.level", Param("suffix", "%"))

        LocalClient.call(request) { (response: String?) in
            DispatchQueue.main.async {
                self.lbBatteryLevelMessage.text = "Response: " + (response ?? "ERROR")
            }
        }
    }

    @objc func btSubmitForLoginClicked() {
        let request = Request(
            "sample.login",
            Param("username", edUsername.text),
            Param("password", edPassword.text),
            Param("remember", swRemember.isOn)
        )

        RemoteClient.call(request) { (response: String?) in
            DispatchQueue.main.async {
                self.lbFormMessage.text = "Response: " + (response ?? "ERROR")
            }
        }
    }

    @objc func btSubmitForCameraClicked() {
        let c = CameraViewController()
        navigationController?.pushViewController(c, animated: true)
    }

    override func viewDidLayoutSubviews() {
        lbTitleBatteryLevel.preferredMaxLayoutWidth = lbTitleBatteryLevel.superview?.size.width ?? 0
    }

    func textFieldShouldReturn(_ textField: UITextField) -> Bool {
        textField.resignFirstResponder()
        return true
    }
}
