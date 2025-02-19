// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'package:sky/src/rendering/box.dart';
import 'package:sky/src/rendering/debug.dart';
import 'package:sky/src/rendering/object.dart';

const double _kMaxWidth = 100000.0;
const double _kMaxHeight = 100000.0;

class RenderErrorBox extends RenderBox {

  double getMinIntrinsicWidth(BoxConstraints constraints) {
    return constraints.constrainWidth(0.0);
  }

  double getMaxIntrinsicWidth(BoxConstraints constraints) {
    return constraints.constrainWidth(_kMaxWidth);
  }

  double getMinIntrinsicHeight(BoxConstraints constraints) {
    return constraints.constrainHeight(0.0);
  }

  double getMaxIntrinsicHeight(BoxConstraints constraints) {
    return constraints.constrainHeight(_kMaxHeight);
  }

  bool get sizedByParent => true;

  void performResize() {
    size = constraints.constrain(const Size(_kMaxWidth, _kMaxHeight));
  }

  void paint(PaintingContext context, Offset offset) {
    context.canvas.drawRect(offset & size, new Paint() .. color = debugErrorBoxColor);
  }

}
