class Param {
  late String name;
  late dynamic value;

  Param(this.name, this.value);

  Map<String, dynamic> toJson() {
    return {
      'n': name,
      'v': value,
    };
  }

  Param.fromJson(Map<String, dynamic> json) {
    name = json['n'];
    value = json['v'];
  }
}
