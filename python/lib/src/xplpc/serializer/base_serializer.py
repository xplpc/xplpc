class BaseSerializer:
    def decodeFunctionName(self, data):
        pass

    def decodeFunctionReturnValue(self, data, class_type=None):
        pass

    def decodeMessage(self, data):
        pass

    def encodeFunctionReturnValue(self, data):
        pass

    def encodeRequest(self, functionName, params=None):
        pass
