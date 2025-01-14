// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:math' as math;
import 'dart:sky' as sky;

import 'package:sky/animation.dart';
import 'package:sky/rendering.dart';
import 'package:sky/src/widgets/basic.dart';
import 'package:sky/src/widgets/framework.dart';

const int _kSplashInitialOpacity = 0x30;
const double _kSplashCancelledVelocity = 0.7;
const double _kSplashConfirmedVelocity = 0.7;
const double _kSplashInitialSize = 0.0;
const double _kSplashUnconfirmedVelocity = 0.2;

double _getSplashTargetSize(Size bounds, Point position) {
  double d1 = (position - bounds.topLeft(Point.origin)).distance;
  double d2 = (position - bounds.topRight(Point.origin)).distance;
  double d3 = (position - bounds.bottomLeft(Point.origin)).distance;
  double d4 = (position - bounds.bottomRight(Point.origin)).distance;
  return math.max(math.max(d1, d2), math.max(d3, d4)).ceil().toDouble();
}

class InkSplash {
  InkSplash(this.pointer, this.position, this.well) {
    _targetRadius = _getSplashTargetSize(well.size, position);
    _radius = new AnimatedValue<double>(
        _kSplashInitialSize, end: _targetRadius, curve: easeOut);

    _performance = new AnimationPerformance()
      ..variable = _radius
      ..duration = new Duration(milliseconds: (_targetRadius / _kSplashUnconfirmedVelocity).floor())
      ..addListener(_handleRadiusChange)
      ..play();
  }

  final int pointer;
  final Point position;
  final RenderInkWell well;

  double _targetRadius;
  double _pinnedRadius;
  AnimatedValue<double> _radius;
  AnimationPerformance _performance;

  void _updateVelocity(double velocity) {
    int duration = (_targetRadius / velocity).floor();
    _performance
      ..duration = new Duration(milliseconds: duration)
      ..play();
  }

  void confirm() {
    _updateVelocity(_kSplashConfirmedVelocity);
  }

  void cancel() {
    _updateVelocity(_kSplashCancelledVelocity);
    _pinnedRadius = _radius.value;
  }

  void _handleRadiusChange() {
    if (_radius.value == _targetRadius)
      well._splashes.remove(this);
    well.markNeedsPaint();
  }

  void paint(PaintingCanvas canvas) {
    int opacity = (_kSplashInitialOpacity * (1.1 - (_radius.value / _targetRadius))).floor();
    sky.Paint paint = new sky.Paint()..color = new sky.Color(opacity << 24);
    double radius = _pinnedRadius == null ? _radius.value : _pinnedRadius;
    canvas.drawCircle(position, radius, paint);
  }
}

class RenderInkWell extends RenderProxyBox {
  RenderInkWell({ RenderBox child }) : super(child);

  final List<InkSplash> _splashes = new List<InkSplash>();

  EventDisposition handleEvent(sky.Event event, BoxHitTestEntry entry) {
    if (event is sky.GestureEvent) {
      switch (event.type) {
        case 'gesturetapdown':
          _startSplash(event.primaryPointer, entry.localPosition);
          return EventDisposition.processed;
        case 'gesturetap':
          _confirmSplash(event.primaryPointer);
          return EventDisposition.processed;
      }
    }
    return EventDisposition.ignored;
  }

  void _startSplash(int pointer, Point position) {
    _splashes.add(new InkSplash(pointer, position, this));
    markNeedsPaint();
  }

  void _forEachSplash(int pointer, Function callback) {
    _splashes.where((splash) => splash.pointer == pointer)
             .forEach(callback);
  }

  void _confirmSplash(int pointer) {
    _forEachSplash(pointer, (splash) { splash.confirm(); });
    markNeedsPaint();
  }

  void paint(PaintingContext context, Offset offset) {
    if (!_splashes.isEmpty) {
      final PaintingCanvas canvas = context.canvas;
      canvas.save();
      canvas.translate(offset.dx, offset.dy);
      canvas.clipRect(Point.origin & size);
      for (InkSplash splash in _splashes)
        splash.paint(canvas);
      canvas.restore();
    }
    super.paint(context, offset);
  }
}

class InkWell extends OneChildRenderObjectWrapper {
  InkWell({ Key key, Widget child })
    : super(key: key, child: child);

  RenderInkWell get renderObject => super.renderObject;
  RenderInkWell createNode() => new RenderInkWell();
}
