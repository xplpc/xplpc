import 'dart:typed_data';

import 'package:test/test.dart';
import 'package:xplpc/client/remote_client.dart';
import 'package:xplpc/core/config.dart';
import 'package:xplpc/core/xplpc.dart';
import 'package:xplpc/data/mapping_list.dart';
import 'package:xplpc/helper/byte_array_helper.dart';
import 'package:xplpc/map/mapping_item.dart';
import 'package:xplpc/message/message.dart';
import 'package:xplpc/message/param.dart';
import 'package:xplpc/message/request.dart';
import 'package:xplpc/message/response.dart';
import 'package:xplpc/serializer/json_serializer.dart';
import 'package:xplpc/type/dataview.dart';

import 'remote_client_test.reflectable.dart';

void reverse(Message m, Response r) {
  r("ok");
}

void main() {
  initializeReflectable();
  XPLPC.instance.initializeReflectable();

  group('Testing Remote Client', () {
    setUp(() {
      XPLPC.instance.initialize(
        Config(
          JsonSerializer(),
        ),
      );
    });

    test('Login', () {
      var request = Request("sample.login", [
        Param("username", "paulo"),
        Param("password", "123456"),
        Param("remember", true),
      ]);

      RemoteClient.call<String>(request, (response) {
        expect("LOGGED-WITH-REMEMBER", response);
      });
    });

    test('Login Async', () async {
      var request = Request("sample.login", [
        Param("username", "paulo"),
        Param("password", "123456"),
        Param("remember", true),
      ]);

      RemoteClient.call<String>(request, (response) {
        expect("LOGGED-WITH-REMEMBER", response);
      });
    });

    test('Login Invalid Cast', () {
      var request = Request("sample.login", [
        Param("username", "paulo"),
        Param("password", "123456"),
        Param("remember", true),
      ]);

      RemoteClient.call<bool>(request, (response) {
        expect(null, response);
      });
    });

    test('Reverse', () {
      MappingList.instance.add(
        "platform.reverse.response",
        MappingItem(reverse),
      );

      var request = Request("sample.reverse");

      RemoteClient.call<String>(request, (response) {
        expect("response-is-ok", response);
      });
    });

    test('Reverse Async', () async {
      MappingList.instance.add(
        "platform.reverse.response",
        MappingItem(reverse),
      );

      var request = Request("sample.reverse");

      RemoteClient.call<String>(request, (response) {
        expect("response-is-ok", response);
      });
    });

    test('Grayscale Image With DataView', () {
      Uint8List data = Uint8List.fromList([
        // red pixel
        255, 0, 0, 255,
        // green pixel
        0, 255, 0, 255,
        // blue pixel
        0, 0, 255, 255,
        // transparent pixel
        0, 0, 0, 0,
      ]);

      var dataView = DataView.createFromByteArray(data);

      var request = Request("sample.image.grayscale.dataview", [
        Param("dataView", dataView),
      ]);

      RemoteClient.call<String>(request, (response) {
        expect("OK", response);

        var data = ByteArrayHelper.createFromDataView(dataView);

        expect(16, dataView.size);
        expect(data[0].toInt(), 85);
        expect(data[4].toInt(), 85);
        expect(data[8].toInt(), 85);
        expect(data[12].toInt(), 0);
      });
    });

    test('Grayscale Image With DataView Async', () async {
      Uint8List data = Uint8List.fromList([
        // red pixel
        255, 0, 0, 255,
        // green pixel
        0, 255, 0, 255,
        // blue pixel
        0, 0, 255, 255,
        // transparent pixel
        0, 0, 0, 0,
      ]);

      var dataView = DataView.createFromByteArray(data);

      var request = Request("sample.image.grayscale.dataview", [
        Param("dataView", dataView),
      ]);

      RemoteClient.call<String>(request, (response) {
        expect("OK", response);

        var data = ByteArrayHelper.createFromDataView(dataView);

        expect(16, dataView.size);
        expect(data[0].toInt(), 85);
        expect(data[4].toInt(), 85);
        expect(data[8].toInt(), 85);
        expect(data[12].toInt(), 0);
      });
    });

    test('DataView', () {
      // get data view
      var request = Request("sample.dataview");

      RemoteClient.call<DataView>(request, (response) {
        // check response
        expect(false, response == null);

        // check current values
        var dataView = response;
        var originalData = ByteArrayHelper.createFromDataView(dataView);

        expect(16, dataView.size);
        expect(originalData[0].toInt(), 255);
        expect(originalData[3].toInt(), 255);
        expect(originalData[7].toInt(), 255);
        expect(originalData[12].toInt(), 0);

        // send original data and check modified data
        var dataView2 = DataView.createFromByteArray(originalData);
        var request2 = Request("sample.image.grayscale.dataview", [
          Param("dataView", dataView2),
        ]);

        RemoteClient.call<String>(request2, (response2) {
          expect("OK", response2);

          var processedData = ByteArrayHelper.createFromDataView(dataView2);

          // check copied values
          expect(16, dataView.size);
          expect(processedData[0].toInt(), 85);
          expect(processedData[4].toInt(), 85);
          expect(processedData[8].toInt(), 85);
          expect(processedData[12].toInt(), 0);

          // check original values again
          expect(16, originalData.length);
          expect(originalData[0].toInt(), 255);
          expect(originalData[3].toInt(), 255);
          expect(originalData[7].toInt(), 255);
          expect(originalData[12].toInt(), 0);
        });
      });
    });

    test('DataView Async', () async {
      // get data view
      var request = Request("sample.dataview");

      RemoteClient.call<DataView>(request, (response) {
        // check response
        expect(false, response == null);

        // check current values
        var dataView = response;
        var originalData = ByteArrayHelper.createFromDataView(dataView);

        expect(16, dataView.size);
        expect(originalData[0].toInt(), 255);
        expect(originalData[3].toInt(), 255);
        expect(originalData[7].toInt(), 255);
        expect(originalData[12].toInt(), 0);

        // send original data and check modified data
        var dataView2 = DataView.createFromByteArray(originalData);
        var request2 = Request("sample.image.grayscale.dataview", [
          Param("dataView", dataView2),
        ]);

        RemoteClient.call<String>(request2, (response2) {
          expect("OK", response2);

          var processedData = ByteArrayHelper.createFromDataView(dataView2);

          // check copied values
          expect(16, dataView.size);
          expect(processedData[0].toInt(), 85);
          expect(processedData[4].toInt(), 85);
          expect(processedData[8].toInt(), 85);
          expect(processedData[12].toInt(), 0);

          // check original values again
          expect(16, originalData.length);
          expect(originalData[0].toInt(), 255);
          expect(originalData[3].toInt(), 255);
          expect(originalData[7].toInt(), 255);
          expect(originalData[12].toInt(), 0);
        });
      });
    });
  });
}
