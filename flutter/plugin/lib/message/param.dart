class Param {
  final String name;
  final dynamic value;

  Param(this.name, this.value);

  Map<String, dynamic> toJson() {
    return {'n': name, 'v': value};
  }
}
