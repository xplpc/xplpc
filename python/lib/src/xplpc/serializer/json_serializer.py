import inspect
import json
import logging as log

from xplpc.message.message import Message
from xplpc.serializer.base_serializer import BaseSerializer


class JsonSerializer(BaseSerializer):
    def decode_function_name(self, data):
        try:
            return json.loads(data)["f"]
        except Exception as e:
            log.error(
                f"[JsonSerializer : decode_function_name] Error when parse json: {e}"
            )

        return ""

    def decode_function_return_value(self, data, class_type=None):
        try:
            if class_type:
                if inspect.signature(class_type.__init__) == inspect.signature(
                    object.__init__
                ):
                    instance = class_type.__new__(class_type)
                else:
                    instance = class_type()

                instance.__dict__.update(json.loads(data)["r"])
                return instance
            else:
                return json.loads(data)["r"]
        except Exception as e:
            log.error(
                f"[JsonSerializer : decode_function_return_value] Error when parse json: {e}"
            )

        return None

    def decode_message(self, data):
        try:
            decodedData = json.loads(data)
            message = Message()

            for param in decodedData["p"]:
                message.set(param["n"], param["v"])

            return message
        except Exception as e:
            log.error(
                f"[JsonSerializer : decode_message] Error when decode message: {e}"
            )

        return None

    def encode_function_return_value(self, data):
        try:
            return json.dumps({"r": data})
        except Exception as e:
            log.error(
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
                }
            )
        except Exception as e:
            log.error(f"[JsonSerializer : encode_request] Error when encode data: {e}")

        return ""
