// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'package:sky/src/widgets/theme.dart';

import 'package:sky/painting.dart';
import 'package:sky/src/rendering/flex.dart';
import 'package:sky/src/widgets/basic.dart';
import 'package:sky/src/widgets/default_text_style.dart';
import 'package:sky/src/widgets/framework.dart';
import 'package:sky/src/widgets/icon.dart';
import 'package:sky/theme/shadows.dart';
import 'package:sky/theme/typography.dart' as typography;
import 'package:sky/theme/view_configuration.dart';

class ToolBar extends Component {

  ToolBar({
    Key key,
    this.left,
    this.center,
    this.right,
    this.backgroundColor
  }) : super(key: key);

  final Widget left;
  final Widget center;
  final List<Widget> right;
  final Color backgroundColor;

  Widget build() {
    Color toolbarColor = backgroundColor;
    IconThemeData iconThemeData;
    TextStyle centerStyle = typography.white.title;
    TextStyle sideStyle = typography.white.body1;
    if (toolbarColor == null) {
      ThemeData themeData = Theme.of(this);
      toolbarColor = themeData.primaryColor;
      if (themeData.primaryColorBrightness == ThemeBrightness.light) {
        centerStyle = typography.black.title;
        sideStyle = typography.black.body2;
        iconThemeData = const IconThemeData(color: IconThemeColor.black);
      } else {
        iconThemeData = const IconThemeData(color: IconThemeColor.white);
      }
    }

    List<Widget> children = new List<Widget>();

    // left children
    if (left != null)
      children.add(left);

    // center children (left-aligned, but takes all remaining space)
    children.add(
      new Flexible(
        child: new Padding(
          child: center != null ? new DefaultTextStyle(child: center, style: centerStyle) : null,
          padding: new EdgeDims.only(left: 24.0)
        )
      )
    );

    // right children
    if (right != null)
      children.addAll(right);

    Widget content = new Container(
      child: new DefaultTextStyle(
        style: sideStyle,
        child: new Column([
            new Container(
              child: new Row(children),
              height: kToolBarHeight
            ),
          ],
          justifyContent: FlexJustifyContent.end
        )
      ),
      padding: new EdgeDims.symmetric(horizontal: 8.0),
      decoration: new BoxDecoration(
        backgroundColor: toolbarColor,
        boxShadow: shadows[2]
      )
    );

    if (iconThemeData != null)
      content = new IconTheme(data: iconThemeData, child: content);
    return content;
  }

}
