# QGroundControl开发指南

[![版本发布](https://img.shields.io/github/release/mavlink/QGroundControl.svg)](https://github.com/mavlink/QGroundControl/releases) [![Discuss](https://img.shields.io/badge/discuss-dev-ff69b4.svg)](http://discuss.px4.io/c/qgroundcontrol/qgroundcontrol-developers)

如果要构建，修改或扩展QGroundControl（QGC），此开发人员指南是获取信息的最佳来源。 它展示了如何获取和构建源代码，解释了QGC的工作原理，并提供了为项目贡献代码的指南。

> **Tip** 本指南适用于开发人员！ 要了解如何使用QGroundControl，请参阅“用户指南”。

<span></span>

> **Note** 本指南的编写是一项正在进行的活动 - 信息应该是正确的，但可能不完整！ 如果您发现它缺少有用的信息（或错误），请提出问题。

## 设计理念

QGC 的设计是为了提供一个能够跨越多个操作系统平台以及多种设备的单个代码库。

QGC 用户界面使用[Qt QML](http://doc.qt.io/qt-5/qtqml-index.html)实现。 QML提供硬件加速，这是平板电脑或手机等低功率设备的关键功能。 QML还提供了一些功能，使我们能够更轻松地创建单个用户界面，以适应不同的屏幕尺寸和分辨率。

与基于桌面鼠标的UI相比，QGC UI更倾向于平板电脑+触摸式UI。 这使得单个UI更容易创建，因为平板电脑样式UI也可以在台式机/笔记本电脑上正常工作。

## 支持 {#support}

Development questions can be raised in the [QGroundControl Developer](http://discuss.px4.io/c/qgroundcontrol/qgroundcontrol-developers) discuss category.

## 贡献

有关贡献的信息，包括编码样式，测试和许可证，可以在代码提交中找到。

> **Tip** We expect all contributors to adhere to the [QGroundControl code of conduct](https://github.com/mavlink/qgroundcontrol/blob/master/CODE_OF_CONDUCT.md). This code aims to foster an open and welcoming environment.

### Coordination Call

The developer team meets bi-weekly to discuss the highest priority issues, project coordination, and discuss any Issues, PRs, or Questions from the Community. ([Developer Call Details](contribute/dev_call.md))

### Translations

We use [Crowdin](https://crowdin.com) to make it easier to manage translation for both *QGroundControl* and the documentation.

The translation projects (and join links) are listed below:

- [QGroundControl](https://crowdin.com/project/qgroundcontrol)（[加入](https://crwd.in/qgroundcontrol)）
- [QGroundControl使用指南](https://crowdin.com/project/qgroundcontrol-user-guide)（[加入](https://crwd.in/qgroundcontrol-user-guide)）
- [QGroundControl开发者指南](https://crowdin.com/project/qgroundcontrol-developer-guide)（[加入](https://crwd.in/qgroundcontrol-developer-guide)）

The PX4 Developer Guide contains additional information about the (common) docs/translation toolchain:

- [文档](https://dev.px4.io/en/contribute/docs.html)
- [翻译](https://dev.px4.io/en/contribute/docs.html)

## 许可证

*QGroundControl* source code is [dual-licensed under Apache 2.0 and GPLv3](https://github.com/mavlink/qgroundcontrol/blob/master/COPYING.md). For more information see: [Licenses](contribute/licences.md).

## 管理

The QGroundControl mission planner is hosted under the governance of the [Dronecode Project](https://www.dronecode.org/).

<a href="https://www.dronecode.org/" style="padding:20px" ><img src="https://mavlink.io/assets/site/logo_dronecode.png" alt="Dronecode Logo" width="110px"/></a>
<a href="https://www.linuxfoundation.org/projects" style="padding:20px;"><img src="https://mavlink.io/assets/site/logo_linux_foundation.png" alt="Linux Foundation Logo" width="80px" /></a>

<div style="padding:10px">&nbsp</div>
