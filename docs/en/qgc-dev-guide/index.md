# QGroundControl Dev Guide

[![Releases](https://img.shields.io/github/release/mavlink/QGroundControl.svg)](https://github.com/mavlink/QGroundControl/releases) [![Discuss](https://img.shields.io/badge/discuss-dev-ff69b4.svg)](http://discuss.px4.io/c/qgroundcontrol/qgroundcontrol-developers)

This developer guide is the best source for information if you want to build, modify or extend [QGroundControl](http://qgroundcontrol.com) (QGC).
It shows how to obtain and build the source code, explains how QGC works, and provides guidelines for contributing code to the project.

:::tip
This part of the guide is for **developers**!
:::

:::warning
This is an active work in progress - information should be correct, but may not be complete!
If you find that it is missing helpful information (or errors) please raise an issue.
:::

## Design Philosophy

QGC is designed to provide a single codebase that can run across multiple OS platforms as well as multiple device sizes and styles.

The QGC user interface is implemented using [Qt QML](http://doc.qt.io/qt-5/qtqml-index.html). QML provides for hardware acceleration which is a key feature on lower powered devices such as tablets or phones. QML also provides features which allows us to more easily create a single user interface which can adapt itself to differing screen sizes and resolution.

The QGC UI targets itself more towards a tablet+touch style of UI than a desktop mouse-based UI. This make a single UI easier to create since tablet style UI also tends to work fine on desktop/laptops.

## Support {#support}

Development questions can be raised in the [QGroundControl Developer](http://discuss.px4.io/c/qgroundcontrol/qgroundcontrol-developers) discuss category.

## Contribution

Information about contributing, including coding styles, testing and licenses can be found in [Code Submissions](contribute/index.md).

:::tip
We expect all contributors to adhere to the [QGroundControl code of conduct](https://github.com/mavlink/qgroundcontrol/blob/master/CODE_OF_CONDUCT.md).
This code aims to foster an open and welcoming environment.
:::

### Coordination Call

The developer team meets bi-weekly to discuss the highest priority issues, project coordination, and discuss any Issues, PRs, or Questions from the Community. ([Developer Call Details](contribute/dev_call.md))

### Translations

We use [Crowdin](https://crowdin.com) to make it easier to manage translation for both _QGroundControl_ and the documentation.

The translation projects (and join links) are listed below:

- [QGroundControl](https://crowdin.com/project/qgroundcontrol) ([join](https://crwd.in/qgroundcontrol))
- [QGroundControl User Guide](https://crowdin.com/project/qgroundcontrol-user-guide) ([join](https://crwd.in/qgroundcontrol-user-guide))
- [QGroundControl Developer Guide](https://crowdin.com/project/qgroundcontrol-developer-guide) ([join](https://crwd.in/qgroundcontrol-developer-guide))

The PX4 Developer Guide contains additional information about the (common) docs/translation toolchain:

- [Documentation](https://dev.px4.io/en/contribute/docs.html)
- [Translation](https://dev.px4.io/en/contribute/docs.html)

## License

_QGroundControl_ source code is [dual-licensed under Apache 2.0 and GPLv3](https://github.com/mavlink/qgroundcontrol/blob/master/COPYING.md).
For more information see: [Licenses](contribute/licences.md).

## Governance

The QGroundControl mission planner is hosted under the governance of the [Dronecode Project](https://www.dronecode.org/).

<a href="https://www.dronecode.org/" style="padding:20px" ><img src="https://mavlink.io/assets/site/logo_dronecode.png" alt="Dronecode Logo" width="110px"/></a>
<a href="https://www.linuxfoundation.org/projects" style="padding:20px;"><img src="https://mavlink.io/assets/site/logo_linux_foundation.png" alt="Linux Foundation Logo" width="80px" /></a>

<div style="padding:10px">&nbsp</div>
