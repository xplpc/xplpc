import inspect
import json

from xplpc.message.decoded_request import DecodedRequest
from xplpc.message.message import Message
from xplpc.serializer.base_serializer import BaseSerializer
from xplpc.util.log import Log

# The reference format is what the c++ core writes, and python is the only json library here whose defaults differ from it, by spacing the output and by escaping every character outside ascii.
# The wire has no token for a number that is not finite, so writing one would put a document on it that no other bridge can read.
SEPARATORS = (",", ":")


class JsonSerializer(BaseSerializer):
    def decode_function_return_value(self, data, class_type=None):
        # An empty response is the empty value every failing path answers with, not a document that failed to parse.
        if not data:
            return None

        try:
            # A document that carries no value answers the empty one, the same as every other bridge, rather than raising.
            value = json.loads(data).get("r")

            if not class_type:
                return value

            if isinstance(value, dict):
                if inspect.signature(class_type.__init__) == inspect.signature(
                    object.__init__
                ):
                    instance = class_type.__new__(class_type)
                else:
                    instance = class_type()

                instance.__dict__.update(value)
                return instance

            # A value that is not an object cannot fill one, so it answers only when it already is the requested type.
            if isinstance(value, class_type):
                return value

            Log.e(
                "[JsonSerializer : decode_function_return_value] Value has another type"
            )
            Log.d(
                f"[JsonSerializer : decode_function_return_value] Value has another type: {type(value).__name__}"
            )

            return None
        except Exception as e:
            Log.e(
                "[JsonSerializer : decode_function_return_value] Error when parse json"
            )
            Log.d(
                f"[JsonSerializer : decode_function_return_value] Error when parse json: {e}"
            )

        return None

    def decode_request(self, data):
        try:
            decoded_data = json.loads(data)
            message = Message()

            # A parameter that arrived without a value is the same as one carrying null, which is what every other bridge reads it as.
            for param in decoded_data.get("p") or []:
                message.set(param["n"], param.get("v"))

            # The decoded shape has to say what actually arrived, since a name that is not text reaches the proxy and raises there instead.
            function_name = decoded_data.get("f")

            if not isinstance(function_name, str):
                function_name = ""

            return DecodedRequest(function_name, message)
        except Exception as e:
            Log.e("[JsonSerializer : decode_request] Error when decode request")
            Log.d(f"[JsonSerializer : decode_request] Error when decode request: {e}")

        return None

    def encode_function_return_value(self, data):
        try:
            return json.dumps(
                {"r": data},
                separators=SEPARATORS,
                ensure_ascii=False,
                allow_nan=False,
            )
        except Exception as e:
            Log.e(
                "[JsonSerializer : encode_function_return_value] Error when encode data"
            )
            Log.d(
                f"[JsonSerializer : encode_function_return_value] Error when encode data: {e}"
            )

        return ""

    def encode_request(self, function_name, params=None):
        try:
            if not params:
                params = []
            else:
                params = [
                    param.to_json() if hasattr(param, "to_json") else param
                    for param in params
                ]

            return json.dumps(
                {
                    "f": function_name,
                    "p": params,
                },
                separators=SEPARATORS,
                ensure_ascii=False,
                allow_nan=False,
            )
        except Exception as e:
            Log.e("[JsonSerializer : encode_request] Error when encode data")
            Log.d(f"[JsonSerializer : encode_request] Error when encode data: {e}")

        return ""
