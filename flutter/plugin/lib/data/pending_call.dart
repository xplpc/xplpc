// A call this isolate made, which carries the answer when the mapping answered before the call returned and the handle the router answers later otherwise.
class PendingCall {
  const PendingCall.answered(String this.answer) : token = 0, call = 0;

  const PendingCall.waiting(this.token, this.call) : answer = null;

  final int token;
  final int call;
  final String? answer;

  bool get answered => answer != null;
}
