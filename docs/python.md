# Python

## Requirements

* Python version 3.10 or later

## How to build the shared library

Execute on terminal:

```
python3 xplpc.py c-build-shared
```

or

```
cmake -S . -B build/c-shared -DXPLPC_TARGET=c-shared -DXPLPC_ADD_CUSTOM_DATA=ON
cmake --build build/c-shared
```

## How to install the Python package

Before install and build the Python package, you need build the C shared library.

To utilize the default behavior, which installs the development package, execute the following command in the terminal:

```
python3 xplpc.py python-install
```

To build a wheel package and install that instead, which is what a consumer receives:

```
python3 xplpc.py python-build
python3 xplpc.py python-install --wheel
```

The wheel is written to `build/python/dist`.

## How to run the Python sample

Before run the Python sample, you need install the Python package.

Install the sample dependencies:

```
python3 -m pip install -r python/sample/src/requirements.txt
```

Run the sample executing on terminal:

```
python3 xplpc.py python-run-sample
```

## How to run the Python sample with PyInstaller

Because of some problems on PyInstaller to identify the `xplpc` Python libraries, you need be inside another environment to work.

Build the Python package for Poetry find it locally on `build/python` folder:

```
python3 xplpc.py python-build
```

Before run the Python sample with PyInstaller, you need install Poetry and PyInstaller packages with command:

```
python3 -m pip install -r python/sample/pyinstaller/requirements.txt
```

Execute PyInstaller:

```
python3 xplpc.py python-pyinstaller
```

You can execute the binary inside folder `build/pyinstaller`.

## How to run the Python tests

Before run the Python tests, you need install the Python package.

Execute on terminal:

```
python3 xplpc.py python-test
```

## How to format the Python code

Execute on terminal:

```
python3 xplpc.py python-format
```

## Bringing the library up

Nothing runs on its own here, so the library is initialized with the serializer it carries before the first call:

```python
from xplpc.core.config import Config
from xplpc.core.xplpc import XPLPC
from xplpc.serializer.json_serializer import JsonSerializer

XPLPC().initialize(Config(JsonSerializer()))
```

A mapping is registered by name, which is what makes it reachable from any other language:

```python
from xplpc.data.mapping_list import MappingList
from xplpc.map.mapping_item import MappingItem

MappingList().add("platform.battery.level", MappingItem(battery_level))
```

Registering reaches the native side, so it fails rather than registering into a bridge that is not there when the library has not been brought up yet.

## Making a call

There are three ways to make a call and they differ in how the answer reaches you.

`call` hands the answer to a callback:

```python
request = Request(
    "sample.login",
    [
        Param("username", "paulo"),
        Param("password", "123456"),
        Param("remember", True),
    ],
)

def on_response(response):
    print(response)

Client.call(request, on_response, str)
```

`call_async` hands it to what the language waits on:

```python
response = await Client.call_async(request, str)
print(response)
```

Both give control back before the answer arrives, so neither one holds up whoever called you. Each has a variant taking the encoded request instead of a `Request`, named `call_from_string` and `call_async_from_string`.

The callback runs on the thread that produced the answer, which is the mapping's thread when the mapping answered later. Anything it touches has to be safe there.

A mapping that answers right away runs the callback on the calling thread, where code that is not safe on another thread happens to work. Treating the callback as if it always ran elsewhere is what keeps it working when that mapping later starts answering from a thread of its own.

`call_sync` answers with the value itself, and `call_sync_from_string` takes the encoded request:

```python
response = Client.call_sync(request, str)
print(response)
```

It reads an answer that is already there rather than waiting for one, so it works only when the mapping answers before it returns. A mapping that takes time is reported and answered with the empty value, and the registration it would have resolved is dropped rather than left behind, so nothing is queued and nothing is leaked. It never blocks a thread, which is why there is no timeout to pass and no deadlock to reach.

Whether a mapping answers before it returns belongs to the mapping rather than to the call site, and that mapping may be written in another language by somebody else. A call that works today answers empty the day that mapping starts doing real work, so `callSync` is for reading something you know is a plain read.

The empty value is also what a mapping with nothing to answer produces, so the two are told apart by the log and not by the value.

## Writing a mapping

A mapping is handed a message and something to answer through, and the one rule is to return quickly. Where the work runs is your decision.

Answering right away is fine when there is nothing slow to do:

```python
def battery_level(m, r):
    r(f"100{m.get('suffix') or ''}")
```

Anything that takes time goes to a thread of your own, and the answer is given from there:

```python
def ip(m, r):
    def work():
        r(urllib.request.urlopen("https://httpbin.io/ip").read().decode())

    threading.Thread(target=work, daemon=True).start()
```

The mapping runs on whichever thread the call arrived on. Doing slow work there instead of handing it to a thread holds that thread for as long as the work takes, which is what the rule above is about.

## Sending a large buffer

`DataView` carries an address and a size across the bridge, so a large buffer crosses without being copied or encoded. `create_from_byte_buffer` exports the buffer into the view, which is what keeps the source from moving or being released while the address is in use:

```python
data = bytearray(read_something())
view = DataView.create_from_byte_buffer(data, len(data))

request = Request("sample.image.grayscale.dataview", [Param("dataView", view)])

Client.call(request, lambda response: print(response))
```

Reading a value never names a type on this bridge, so a data view parameter reaches a mapping as the dictionary the wire carried and `DataView.from_json` turns it into a view. A value that is not a data view is reported and answered as an empty view rather than raising:

```python
view = DataView.from_json(message.get("image"))
data = ByteArrayHelper.create_from_data_view(view)
```

`create_from_data_view` answers an empty result for a view that carries no address rather than reading from it. A view is released only by the side that allocated it, and a mapping that answers with a view has to keep its buffer alive after it returns.

## Logging

The library reports every failing path. An error names what failed and where, and is always reported. A debug line carries the reason behind it, including the message the parser produced, and is off until you ask for it.

The library writes to a logger named `XPLPC`, and your application decides the level and where it goes:

```python
import logging

logging.basicConfig()
logging.getLogger("XPLPC").setLevel(logging.DEBUG)
```

## Sample project

You can see the sample project in directory `python/sample`.

<img width="280" src="https://github.com/xplpc/xplpc/blob/main/extras/images/screenshot-python-macos.png?raw=true">

<img width="280" src="https://github.com/xplpc/xplpc/blob/main/extras/images/screenshot-python-windows.png?raw=true">

<img width="280" src="https://github.com/xplpc/xplpc/blob/main/extras/images/screenshot-python-linux.png?raw=true">
