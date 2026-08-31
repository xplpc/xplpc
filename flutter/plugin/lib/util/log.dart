import 'package:logging/logging.dart';

class Log {
  static final log = Logger('XPLPC');

  static void d(String message) {
    log.log(Level.FINE, message);
  }

  static void e(String message) {
    log.log(Level.SEVERE, message);
  }
}
