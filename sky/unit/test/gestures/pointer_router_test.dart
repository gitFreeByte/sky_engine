import 'dart:sky' as sky;

import 'package:sky/gestures/pointer_router.dart';
import 'package:test/test.dart';

import '../engine/mock_events.dart';

void main() {
  test('Should route pointers', () {
    bool callbackRan = false;
    void callback(sky.PointerEvent event) {
      callbackRan = true;
    }

    TestPointer pointer2 = new TestPointer(2);
    TestPointer pointer3 = new TestPointer(3);

    PointerRouter router = new PointerRouter();
    router.addRoute(3, callback);
    router.route(pointer2.down());
    expect(callbackRan, isFalse);
    router.route(pointer3.down());
    expect(callbackRan, isTrue);
    callbackRan = false;
    router.removeRoute(3, callback);
    router.route(pointer3.up());
    expect(callbackRan, isFalse);
  });
}
