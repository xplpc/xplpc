import XCTest
import xplpc

struct Todo: Codable {
    var id: Int64
    var title: String
    var body: String
    var data: [String: String]
    var done: Bool
}

final class TodoTest: XPLPCTestCase {
    func testSingleItem() {
        let todo = Todo(id: 1, title: "Title 1", body: "Body 1", data: [:], done: true)

        let request = Request("sample.todo.single", Param("item", todo))

        verifyCall(request) { (r: Todo?) in
            XCTAssertEqual("Title 1", r?.title)
        }
    }

    func testMultipleItems() {
        let todo1 = Todo(id: 1, title: "Title 1", body: "Body 1", data: [:], done: true)
        let todo2 = Todo(id: 2, title: "Title 2", body: "Body 2", data: [:], done: true)
        let list = [todo1, todo2]

        let request = Request("sample.todo.list", Param("items", list))

        verifyCall(request) { (r: [Todo]?) in
            XCTAssertEqual(2, r?.count)
            XCTAssertEqual("Title 1", r?[0].title)
            XCTAssertEqual("Title 2", r?[1].title)
        }
    }

    func testSingleItemAsync() {
        let todo = Todo(id: 1, title: "Title 1", body: "Body 1", data: [:], done: true)
        let request = Request("sample.todo.single", Param("item", todo))

        verifyCallFromBackground(request) { (r: Todo?) in
            XCTAssertEqual("Title 1", r?.title)
        }
    }

    func testMultipleItemsAsync() {
        let todo1 = Todo(id: 1, title: "Title 1", body: "Body 1", data: [:], done: true)
        let todo2 = Todo(id: 2, title: "Title 2", body: "Body 2", data: [:], done: true)
        let list = [todo1, todo2]

        let request = Request("sample.todo.list", Param("items", list))

        verifyCallFromBackground(request) { (r: [Todo]?) in
            XCTAssertEqual(2, r?.count)
            XCTAssertEqual("Title 1", r?[0].title)
            XCTAssertEqual("Title 2", r?[1].title)
        }
    }
}
