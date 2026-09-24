# QGroundControl 字符串翻译

QGC 使用标准的 Qt Linguist 机制翻译字符串，翻译内容来自众包
[Crowdin 项目](https://crowdin.com/project/qgroundcontrol)。

## Crowdin 集成

Crowdin 每天同步一次 `qgc.ts`，自动获取新增内容，并针对生成的翻译提交拉取请求（PR）。

### 添加新语言

在 Crowdin 设置中添加该语言。

### 在发布周期中更新基础翻译文件

`lupdate.yml` 工作流每周日重新生成 `.ts` 源文件，并提交拉取请求（PR）以纳入变更。
也可以在 Actions 页面手动触发，或者在仓库根目录运行：

```bash
python3 tools/translations/qgc_lupdate.py
```

Crowdin 会获取这些内容，并在翻译完成后将译文提交回来。

## C++ 和 QML 代码中的字符串

使用标准的 Qt `tr()`（C++）和 `qsTr()`（QML）机制。

## 翻译 JSON 文件中的字符串

QGC 将一些同样需要翻译的元数据存储在 JSON 文件中。
[JSON 解析器](https://github.com/mavlink/qgroundcontrol/blob/master/tools/translations/qgc_lupdate_json.py)
会查找源码树中的每个 JSON 文件，提取可翻译的字符串，并生成 Qt `.ts` 翻译文件。
要标记需要翻译的字符串，请在 JSON 根对象中添加相应的键。

> **重要：** JSON 文件不能重名，因为文件名用作翻译查找时的上下文；如有重名，解析器会报错。
> 根文件名还必须与 Qt 资源别名一致，因为解析器看到的是文件系统中的名称，而 QGC 的 C++ 代码
> 通过 Qt 资源系统读取文件（看到的是别名路径）。

### 已知文件类型

解析器支持两种已知类型。将根对象的 `fileType` 键设置为 `MAVCmdInfo` 或 `FactMetaData`，
即可应用对应的默认配置：

```json
// MAVCmdInfo
"translateKeys": "label,enumStrings,friendlyName,description",
"arrayIDKeys":   "rawName,comment"

// FactMetaData
"translateKeys": "shortDescription,longDescription,enumStrings",
"arrayIDKeys":   "name"
```

### 手动配置解析器

不设置 `fileType`，而是按需在根对象中设置以下键：

- `translateKeys`：用逗号分隔的待翻译键名列表。
- `arrayIDKeys`：对于数组，指定能够唯一标识每个元素的键名，以便为译者提供有意义的上下文，
  而不只是数组索引。

### 消歧

如果同一文件中的两个相同字符串需要不同的译文，请在值前添加消歧标记：

```json
"foo": "#loc.disambiguation#This is the foo version of baz#baz"
"bar": "#loc.disambiguation#This is the bar version of baz#baz"
```

这里的 `baz` 是两个条目共用的字符串；`#loc.disambiguation#...#` 为译者提供说明，
最后一个 `#` 后面才是实际需要翻译的字符串。
