import logging

logger = logging.getLogger("XPLPC")


class Log:
    @staticmethod
    def d(message: str):
        logger.debug(message)

    @staticmethod
    def e(message: str):
        logger.error(message)
