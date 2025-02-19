// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'package:sky/animation.dart';
import 'package:sky/src/widgets/animated_component.dart';
import 'package:sky/src/widgets/basic.dart';
import 'package:sky/src/widgets/framework.dart';
import 'package:sky/src/widgets/mimic.dart';

class MimicOverlay extends AnimatedComponent {
  MimicOverlay({
    Key key,
    this.children,
    this.overlay,
    this.duration: const Duration(milliseconds: 200),
    this.curve: linear,
    this.targetRect
  }) : super(key: key);

  List<Widget> children;
  GlobalKey overlay;
  Duration duration;
  Curve curve;
  Rect targetRect;

  void syncConstructorArguments(MimicOverlay source) {
    children = source.children;

    duration = source.duration;
    _expandPerformance.duration = duration;

    targetRect = source.targetRect;
    _mimicBounds.end = targetRect;
    if (_expandPerformance.isCompleted) {
      _mimicBounds.value = _mimicBounds.end;
    }

    curve = source.curve;
    _mimicBounds.curve = curve;

    if (overlay != source.overlay) {
      overlay = source.overlay;
      if (_expandPerformance.isDismissed) {
        _activeOverlay = overlay;
      } else {
        _expandPerformance.reverse();
      }
    }
  }

  void initState() {
    _mimicBounds = new AnimatedRect(new Rect(), curve: curve);
    _mimicBounds.end = targetRect;
    _expandPerformance = new AnimationPerformance()
      ..duration = duration
      ..addVariable(_mimicBounds)
      ..addListener(_handleAnimationTick)
      ..addStatusListener(_handleAnimationStatusChanged);
    watch(_expandPerformance);
  }

  GlobalKey _activeOverlay;
  AnimatedRect _mimicBounds;
  AnimationPerformance _expandPerformance;

  void _handleAnimationStatusChanged(AnimationStatus status) {
    if (status == AnimationStatus.dismissed) {
      setState(() {
        _activeOverlay = overlay;
      });
    }
  }

  void _handleAnimationTick() {
    if (_activeOverlay == null)
      return;
    _updateMimicBounds();
  }

  void _updateMimicBounds() {
    Mimicable mimicable = GlobalKey.getWidget(_activeOverlay) as Mimicable;
    Rect globalBounds = mimicable.globalBounds;
    if (globalBounds == null)
      return;
    Rect localBounds = globalToLocal(globalBounds.topLeft) & globalBounds.size;
    if (localBounds == _mimicBounds.begin)
      return;
    setState(() {
      _mimicBounds.begin = localBounds;
      if (_expandPerformance.isDismissed)
        _mimicBounds.value = _mimicBounds.begin;
    });
  }

  void _handleMimicReady() {
    _updateMimicBounds();
    if (_expandPerformance.isDismissed)
      _expandPerformance.forward();
  }

  Widget build() {
    List<Widget> layers = new List<Widget>();

    if (children != null)
      layers.addAll(children);

    if (_activeOverlay != null) {
      layers.add(
        new Positioned(
          left: _mimicBounds.value.left,
          top: _mimicBounds.value.top,
          child: new SizedBox(
            width: _mimicBounds.value.width,
            height: _mimicBounds.value.height,
            child: new Mimic(
              onMimicReady: _handleMimicReady,
              original: _activeOverlay
            )
          )
        )
      );
    }

    return new Stack(layers);
  }
}
