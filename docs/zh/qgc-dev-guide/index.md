# QGroundControl开发指南

[![Discuss](https://img.shields.io/badge/discuss-dev-ff69b4.svg)](http://discuss.px4.io/c/qgroundcontrol/qgroundcontrol-developers)
[![Discord](https://discordapp.com/api/guilds/1022170275984457759/widget.png?style=shield)](https://discord.com/channels/1022170275984457759/1022185820683255908)

如果要构建，修改或扩展 [QGroundControl](http://qgroundcontrol.com)（QGC），此开发人员指南是获取信息的最佳来源。 它展示了如何获取和构建源代码，解释了QGC的工作原理，并提供了为项目贡献代码的指南。
它展示了如何获取和构建源代码，解释了QGC 是如何运作的，并提供了为项目提供代码的指导方针。

:::tip
指南的这一部分是为 **开发者** 提供的！
:::

:::warning
这是一项正在积极推进的工作——信息应该是正确的，但可能并不完整！
如果您发现缺少有用的信息(或错误)，请提出问题或更好地向文档提交包含更新信息的拉取请求。
:::

## 设计理念

QGC 的设计是为了提供一个能够跨越多个操作系统平台以及多种设备的单个代码库。

QGC 用户界面是使用 Qt Qml 实现的。 Qml 提供硬件加速，它是平板电脑或手机等低功率设备上的一个关键功能。 Qml 还提供了一些功能，使我们能够更轻松地创建一个单一的用户界面，使其能够适应不同的屏幕尺寸和分辨率。

与基于桌面鼠标的UI相比，QGC UI更倾向于平板电脑+触摸式UI。 这使得单个UI更容易创建，因为平板电脑样式UI也可以在台式机/笔记本电脑上正常工作。 这使得单个用户界面更容易创建，因为平板电脑风格界面也会在桌面电脑/笔记本电脑上正常工作。

## 支持 {#support}

开发问题可以在 [QGroundControl Developer] (http://discuss.px4.io/c/qgroundcontrol/qgroundcontrol-developers) 讨论类别中提出。

## 贡献

有关贡献的信息，包括编码样式，测试和许可证，可以在[代码提交](contribute/index.md)中找到。

:::tip
我们期望所有贡献者都遵守[QGroundControl Code of Conduct](https://github.com/mavlink/qgroundcontrol/blob/master/.github/CODE_OF_CONDUCT.md)。
该守则旨在营造一个开放和热情的环境。
:::

### 翻译

我们使用 [Crowdin](https://crowdin.com) 来使管理翻译 _QGroundControl_ 和文档更容易。

以下是翻译项目（和加入链接）：

- [QGroundControl](https://crowdin.com/project/qgroundcontrol)（[加入](https://crwd.in/qgroundcontrol)）
- [QGroundControl使用指南](https://crowdin.com/project/qgroundcontrol-user-guide)（[加入](https://crwd.in/qgroundcontrol-user-guide)）
- [QGroundControl开发者指南](https://crowdin.com/project/qgroundcontrol-developer-guide)（[加入](https://crwd.in/qgroundcontrol-developer-guide)）

PX4 开发者指南包含更多关于（通用）文件翻译工具链的信息：

- [文档](https://dev.px4.io/en/contribute/docs.html)
- [翻译](https://dev.px4.io/en/contribute/docs.html)

## 许可证

_QGroundControl_ 源代码是 [Apache 2.0 和 GPLv3下的双向授权](https://github.com/mavlink/qgroundcontrol/blob/master/.github/COPYING.md)。
详情见： [Licenses](contribute/licences.md)。

## 管理

QGroundControl 地面站由[Dronecode Project](https://www.dronecode.org/)管理。

<div style="padding:10px"> </div>
