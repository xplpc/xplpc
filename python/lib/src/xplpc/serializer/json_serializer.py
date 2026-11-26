import dataclasses
import inspect
import json
import math
import sys

from xplpc.message.decoded_request import DecodedRequest
from xplpc.message.message import Message
from xplpc.serializer.serializer import Serializer
from xplpc.util.log import Log

# The reference format is what the c++ core writes, and python is the only json library here whose defaults differ from it, by spacing the output and by escaping every character outside ascii.
# The wire has no token for a number that is not finite, so writing one would put a document on it that no other bridge can read.
SEPARATORS = (",", ":")


class JsonSerializer(Serializer):
    def decode_function_return_value(self, data, class_type=None):
        # An empty response is the empty value every failing path answers with, not a document that failed to parse.
        if not data:
            return None

        try:
            document = JsonSerializer._read(data)
        except Exception as e:
            Log.e(
                "[JsonSerializer : decode_function_return_value] Error when parse json"
            )
            Log.d(
                f"[JsonSerializer : decode_function_return_value] Error when parse json: {e}"
            )

            return None

        # A document that carries no value is the empty value, which is not a failure to report.
        value = document.get("r") if isinstance(document, dict) else None

        if not class_type or value is None:
            return value

        if self._is_of_type(value, class_type):
            return value

        # An integer reads as a float only when the float holds it exactly, which is the rule every bridge that names a type follows.
        if class_type is float and self._is_integer(value) and self._float_holds(value):
            return float(value)

        # A type that reads itself off the wire decides what it accepts, so a data view reads anything that is not a handle as naming no buffer.
        if hasattr(class_type, "from_json") or (
            isinstance(value, dict) and class_type.__module__ != "builtins"
        ):
            return self._fill(class_type, value)

        # A value that is not an object cannot fill one, so it answers only when it already is the requested type.
        Log.e("[JsonSerializer : decode_function_return_value] Value has another type")
        Log.d(
            f"[JsonSerializer : decode_function_return_value] Value has another type: {type(value).__name__}"
        )

        return None

    def decode_request(self, data):
        try:
            decoded_data = JsonSerializer._read(data)

            if not isinstance(decoded_data, dict):
                Log.e("[JsonSerializer : decode_request] Error when decode request")
                return None

            # A request that names no function as text routes nowhere, which is what the reference refuses before it ever reaches a decoder.
            function_name = decoded_data.get("f")

            if not isinstance(function_name, str) or not function_name:
                Log.e("[JsonSerializer : decode_request] The request names no function")
                return None

            params = decoded_data.get("p")

            if params is not None and not isinstance(params, list):
                Log.e(
                    f'[JsonSerializer : decode_request] Parameters of function "{function_name}" are not a list'
                )
                return None

            message = Message()
            names = set()

            for param in params or []:
                name = param.get("n") if isinstance(param, dict) else None

                if not isinstance(name, str):
                    Log.e(
                        f'[JsonSerializer : decode_request] Function "{function_name}" carries a parameter whose name is not text'
                    )
                    return None

                # A map keeps only one value of a repeated name, so every bridge refuses the request rather than read it as something the sender did not write.
                if name in names:
                    Log.e(
                        f'[JsonSerializer : decode_request] Parameter "{name}" of function "{function_name}" appears more than once'
                    )
                    return None

                names.add(name)

                # A parameter that arrived without a value is the same as one carrying null, which is what every other bridge reads it as.
                message.set(name, param.get("v"))

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
                default=JsonSerializer._encode_object,
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
            return json.dumps(
                {
                    "f": function_name,
                    "p": params or [],
                },
                separators=SEPARATORS,
                ensure_ascii=False,
                allow_nan=False,
                default=JsonSerializer._encode_object,
            )
        except Exception as e:
            Log.e("[JsonSerializer : encode_request] Error when encode data")
            Log.d(f"[JsonSerializer : encode_request] Error when encode data: {e}")

        return ""

    # An object that describes itself is written as that description wherever it sits in a value, which is how a view inside a list or an answer reaches the wire.
    @staticmethod
    def _encode_object(value):
        if hasattr(value, "to_json"):
            return value.to_json()

        raise TypeError(f"Object of type {type(value).__name__} is not serializable")

    # The decoder reads tokens and magnitudes no other bridge accepts, so a document carrying one is refused as one it could not read.
    @staticmethod
    def _read(data):
        return json.loads(
            data,
            parse_constant=JsonSerializer._refuse_constant,
            parse_float=JsonSerializer._read_float,
        )

    @staticmethod
    def _refuse_constant(token):
        raise ValueError(f"The number {token} is not finite")

    @staticmethod
    def _read_float(text):
        value = float(text)

        if not math.isfinite(value):
            raise ValueError("The number is past what a double holds")

        return value

    @staticmethod
    def _float_holds(value):
        # An integer past what a double holds cannot become one, which the conversion reports by raising.
        return abs(value) <= sys.float_info.max and float(value) == value

    @staticmethod
    def _is_integer(value):
        return isinstance(value, int) and not isinstance(value, bool)

    @staticmethod
    def _is_of_type(value, class_type):
        # A bool is an int here, and a boolean is never a number on the wire.
        if isinstance(value, bool) and class_type in (int, float):
            return False

        return isinstance(value, class_type)

    @staticmethod
    def _fill(class_type, value):
        try:
            return JsonSerializer._instantiate(class_type, value)
        except Exception as e:
            Log.e(
                "[JsonSerializer : decode_function_return_value] Value has another type"
            )
            Log.d(
                f"[JsonSerializer : decode_function_return_value] Value has another type: {e}"
            )

        return None

    @staticmethod
    def _instantiate(class_type, value):
        if hasattr(class_type, "from_json"):
            return class_type.from_json(value)

        # A field the type does not declare is ignored, the way every other bridge decodes an object.
        if dataclasses.is_dataclass(class_type):
            fields = {
                field.name for field in dataclasses.fields(class_type) if field.init
            }

            return class_type(**{n: v for n, v in value.items() if n in fields})

        parameters = inspect.signature(class_type).parameters.values()

        if any(parameter.kind is parameter.VAR_KEYWORD for parameter in parameters):
            return class_type(**value)

        accepted = {
            parameter.name
            for parameter in parameters
            if parameter.kind
            in (parameter.POSITIONAL_OR_KEYWORD, parameter.KEYWORD_ONLY)
        }

        instance = class_type(**{n: v for n, v in value.items() if n in accepted})
        slots = JsonSerializer._declared_slots(class_type)

        for name, item in value.items():
            if name in accepted:
                continue

            if hasattr(instance, "__dict__") or name in slots:
                setattr(instance, name, item)

        return instance

    @staticmethod
    def _declared_slots(class_type):
        slots = set()

        for klass in class_type.__mro__:
            declared = getattr(klass, "__slots__", ())
            slots.update((declared,) if isinstance(declared, str) else declared)

        return slots
