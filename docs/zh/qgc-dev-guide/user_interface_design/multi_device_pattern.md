# 多设备设计模式

QGroundControl设计用于从桌面到笔记本电脑，平板电脑和使用鼠标和触摸的小型手机屏幕等多种设备类型。 以下是QGC如何做到以及背后原理的描述。 Below is the description of how QGC does it and the reasoning behind it.

## 高效的一个人开发团队

QGC开发用于解决此问题的设计模式基于快速开发新功能并允许代码库由一个非常小的团队测试和维护（假设1个开发人员作为默认的开发团队规模）。 实现这一目标的模式非常严格，因为不遵循它将导致更慢的开发时间和更低的质量。 The pattern to achieve this is followed very strictly, because not following it will lead to slower dev times and lower quality.

Supporting this 1 person dev team concept leads to some tough decisions which not everyone may be happy about. But it does lead to QGC being released on many OS and form factors using a single codebase. This is something most other ground stations out there are not capable of achieving.

What about contributors you ask? QGC has a decent amount of contributors. 你可能会问，贡献者呢？ QGC拥有相当数量的贡献者。 Can't they help move things past this 1 person dev team concept? Yes QGC has quite a few contributors. But unfortunately they come and go over time. And when they go, the code they contributed still must be maintained. Hence you fall back to the 1 person dev team concept which is mostly what has been around as an average over the last three years of development.

## 目标设备

从触摸的角度和屏幕尺寸的角度来看，QGC UI设计的优先目标是平板电脑（比如三星Galaxy）。 由于此决定，其他设备类型和大小可能会看到一些视觉和/或可用性的错误。 基于优先级的决策时的当前顺序是平板电脑，笔记本电脑，台式机，电话（任何小屏幕）。 Other device types and sizes may see some sacrifices of visuals and/or usability due to this decision. The current order when making priority based decisions is Tablet, Laptop, Desktop, Phone (any small screen).

### 手机大小的屏幕支持

As specified above, at this point smaller phone sized screens are the lowest level priority for QGC. More focus is put onto making active flight level displays, such as the Fly view, more usable. Less focus is placed on Setup related views such as Setup and Plan. 较少关注设置相关视图，例如“设置”和“计划”。 Those specific views are tested to be functionally usable on small screens but they may be painful to use.

## 使用的开发工具

### Qt布局控件

QGC没有针对不同屏幕尺寸和/或形状因子的不同编码的UI。 通常，它使用QML Layout功能来重排一组QML UI代码以适应不同的外形。 在某些情况下，它提供了较小的屏幕尺寸细节，使事情适合。 但这是一个简单的可见性模式。 是的QGC有几个贡献者。 但是，他们随着时间的推移加入而来。 当它们离开时，它们贡献的代码仍然必须保持 因此，你回到了过去三年发展中的平均1人开发团队的概念 In some cases it provides less detail on small screen sizes to make things fit. But that is a simple visibility pattern.

### FactSystem（事实系统）

QGC内部是一个系统，用于管理系统中的所有单个数据。 这个数据模型是连接到控件的。 This data model is then connected to controls.

### 严重依赖可重用控件

QGC UI是从一组可重用的控件和UI元素开发而来的。 这样，现在可以在整个UI中使用添加到可重用控件的任何新功能。 这些可重用的控件还连接到FactSystem Facts，然后FactSystem Facts自动提供适当的UI。 This way any new feature added to a reusable control is now available throughout the UI. These reusable controls also connect to FactSystem Facts which then automatically provides appropriate UI.

## 这种设计模式的缺点

- QGC用户界面最终成为台式机/笔记本电脑/平板电脑/手机的混合风格。 因此，不一定看起来或感觉它被优化为任何这些。 Hence not necessarily looking or feeling like it is optimized to any of these.
- 给定目标设备优先级列表以及QGC倾向于重新布局相同UI元素以适应不同形状因子的事实, 当你离优先目标越来越远时，你会发现这种混合方法会变得更糟。 因此，小型手机大小的屏幕在可用性方面受到的打击最大。 Hence small phone sized screens taking the worst hit on usability.
- 在某些情况下，QGC可重用控件集可能无法提供绝对最佳的UI。 但它仍然用于防止创建额外的维护表面区域。 But it is still used to prevent the creation of additional maintenance surface area.
- 由于QGC UI对所有操作系统使用相同的UI代码，因此QGC不遵循操作系统本身指定的UI设计准则。 它有自己的视觉风格，有点混合了从每个操作系统中挑选出来的东西。 因此，UI在所有操作系统上的外观和工作方式大致相同。 再次, 这意味着, 例如, QGC 运行在 android 并不一定看起来像一个 android 应用程序。或者, 在 iphone 上运行的 QGC 不会像其他大多数 iphone 应用程序那样看起来或工作。 也就是说，QGC视觉/功能样式对于这些OS用户来说应该是可以理解的。 It has it's own visual style which is somewhat of a hybrid of things picked from each OS. Hence the UI looks and works mostly the same on all OS. Once again this means for example that QGC running on Android won't necessarily look like an android app. 支持这个1人开发团队概念导致一些艰难的决定，并不是每个人都可能感到高兴。 但它确实导致QGC使用单个代码库在许多操作系统和硬件平台上发布。 This is something most other ground stations out there are not capable of achieving. That said the QGC visual/functional style should be understandable to these OS users.

## 这种设计模式的优点

- It takes less time to design a new feature since the UI coding is done once using this hybrid model and control set. 由于使用此混合模型和控制集进行一次UI编码，因此设计新功能所需的时间更短。 布局重排在Qt QML中非常强大，一旦你习惯它就成为第二天性。
- A piece of UI can be functionally tested on one platform since the functional code is the same across all form factors. UI可以仅在一个平台上进行功能测试，因为代码是平台无关的。 只有布局流必须在多个设备上进行可视化检查, 但这很容易使用移动模拟器完成。 在大多数情况下，这是需要的： In most cases this is what is needed:
  - 使用桌面构建，调整窗口大小以测试重排。 通常也会覆盖平板电脑大小的屏幕。 This will generally cover a tablet sized screen as well.
  - 将手机（小屏幕）级别优先级提升为更接近平板电脑。 目前的想法是，这将不会发生在3.3发布时间框架（在当前工作之后发布）。 使用移动模拟器直观地验证手机大小的屏幕。 在OSX XCode iPhone模拟器上工作得非常好。
- All of the above are critical to keeping our hypothetical 1 person dev team efficient and to keep quality high.

## 未来发展方向

- Raise phone (small screen) level prioritization to be more equal to Tablet. Current thinking is that this won't happen until a 3.3 release time frame (release after current one being worked on).
