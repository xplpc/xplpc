import json
from xplpc.serializer.base_serializer import BaseSerializer


class JsonSerializer(BaseSerializer):
    def decodeFunctionName(self, data):
        try:
            return json.loads(data)["f"]
        except Exception as e:
            print(f"[JsonSerializer : decodeFunctionName] Error when parse json: {e}")
        return ""

    def decodeFunctionReturnValue(self, data, class_type=None):
        try:
            if class_type:
                instance = class_type()
                instance.__dict__.update(json.loads(data)["r"])
                return instance
            else:
                return json.loads(data)["r"]
        except Exception as e:
            print(
                f"[JsonSerializer : decodeFunctionReturnValue] Error when parse json: {e}"
            )
        return None

    def decodeMessage(self, data):
        try:
            decodedData = json.loads(data)
            message = Message()

            for param in decodedData["p"]:
                message.set(param["n"], param["v"])

            return message
        except Exception as e:
            print(f"[JsonSerializer : decodeMessage] Error when decode message: {e}")
        return None

    def encodeFunctionReturnValue(self, data):
        try:
            return json.dumps({"r": data})
        except Exception as e:
            print(
                f"[JsonSerializer : encodeFunctionReturnValue] Error when encode data: {e}"
            )
        return ""

    def encodeRequest(self, functionName, params=None):
        try:
            return json.dumps(
                {
                    "f": functionName,
                    "p": params if params else [],
                }
            )
        except Exception as e:
            print(f"[JsonSerializer : encodeRequest] Error when encode data: {e}")
        return ""
