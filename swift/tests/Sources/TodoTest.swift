import XCTest

@testable import xplpc

struct Todo: Codable {
    var id: Int64
    var title: String
    var body: String
    var data: [String: String]
    var done: Bool
}

final class TodoTest: XCTestCase {
    override func setUpWithError() throws {
        // Put setup code here. This method is called before the invocation of each test method in the class.
        XPLPC.shared.initialize(
            config: Config(serializer: JsonSerializer())
        )
    }

    override func tearDownWithError() throws {
        // Put teardown code here. This method is called after the invocation of each test method in the class.
    }

    func testSingleItem() throws {
        let todo = Todo(id: 1, title: "Title 1", body: "Body 1", data: [:], done: true)

        let request = Request("sample.todo.single", Param("item", todo))
        let r: Todo? = RemoteClient.call(request)

        XCTAssertEqual("Title 1", r?.title)
    }

    func testMultipleItems() throws {
        let todo1 = Todo(id: 1, title: "Title 1", body: "Body 1", data: [:], done: true)
        let todo2 = Todo(id: 2, title: "Title 2", body: "Body 2", data: [:], done: true)
        let list = [todo1, todo2]

        let request = Request("sample.todo.list", Param("items", list))
        let r: [Todo]? = RemoteClient.call(request)

        XCTAssertEqual(2, r?.count)
        XCTAssertEqual("Title 1", r?[0].title)
        XCTAssertEqual("Title 2", r?[1].title)
    }

    func testSingleItemCallback() throws {
        let todo = Todo(id: 1, title: "Title 1", body: "Body 1", data: [:], done: true)

        let request = Request("sample.todo.single", Param("item", todo))

        RemoteClient.callAsync(request) { (r: Todo?) in
            XCTAssertEqual("Title 1", r?.title)
        }
    }

    func testMultipleItemsCallback() throws {
        let todo1 = Todo(id: 1, title: "Title 1", body: "Body 1", data: [:], done: true)
        let todo2 = Todo(id: 2, title: "Title 2", body: "Body 2", data: [:], done: true)
        let list = [todo1, todo2]

        let request = Request("sample.todo.list", Param("items", list))

        RemoteClient.callAsync(request) { (r: [Todo]?) in
            XCTAssertEqual(2, r?.count)
            XCTAssertEqual("Title 1", r?[0].title)
            XCTAssertEqual("Title 2", r?[1].title)
        }
    }

    #if compiler(>=5.7) && canImport(_Concurrency)
        @available(macOS 12, iOS 13, tvOS 13, watchOS 6, *)
        func testSingleItemAsync() async throws {
            let todo = Todo(id: 1, title: "Title 1", body: "Body 1", data: [:], done: true)

            let request = Request("sample.todo.single", Param("item", todo))
            let r: Todo? = await RemoteClient.callAsync(request)

            XCTAssertEqual("Title 1", r?.title)
        }

        @available(macOS 12, iOS 13, tvOS 13, watchOS 6, *)
        func testMultipleItemsAsync() async throws {
            let todo1 = Todo(id: 1, title: "Title 1", body: "Body 1", data: [:], done: true)
            let todo2 = Todo(id: 2, title: "Title 2", body: "Body 2", data: [:], done: true)
            let list = [todo1, todo2]

            let request = Request("sample.todo.list", Param("items", list))
            let r: [Todo]? = await RemoteClient.callAsync(request)

            XCTAssertEqual(2, r?.count)
            XCTAssertEqual("Title 1", r?[0].title)
            XCTAssertEqual("Title 2", r?[1].title)
        }
    #endif
}
