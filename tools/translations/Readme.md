### Extract translation text and use AI to perform the translation.
- modify `files_to_process` list in qgc_translate.py
- run qgc_translate.py, it will generate qgc_xxx.ts_missing in translations folders,
- all the text in it are the text that need translate, copy them to let AI translate.
- copy result to dict_xxx.py's get_dict function, and then run qgc_translate.py again.
- Bingo!!!, you will found the xxx.ts already translated.

### Notice:
- Do not copy too much text to the AI at once, as it may exceed the limit. 500 is ok.
- The AI translation might also have issues, so it's best to have it double-checked manually.
- Can not support long multi-line text.
- ***This method can be used for all QT projects translate.***

### Translation prompt(For zh_CN in https://chat.deepseek.com/)
```
以下文本都是 "英文":"TODO", 的形式, 这些英文是无人机相关的英文, 将他们翻译成中文, 并替换成 "英文": """中文""", 的格式.
注意:
1.不要更改原始输入英文的格式,在输出的中文中也保留对应的格式字符,只要 txt 的纯文本格式, 不要转换成其他格式.
2.不需要流式给我推送结果, 整体处理完以后一起给我翻译后的结果.

输入示例:
"Forward": "TODO",
"Frame Class": "TODO",
"Currently set to frame class &apos;%1&apos;": "TODO",
"All Files (*)": "TODO",
"Receiving signal. Perform range test &amp; confirm.": "TODO",

输出示例:
"Forward": """前进""",
"Frame Class": """机架类别""",
"Currently set to frame class &apos;%1&apos;": """当前设置为机架类别 &apos;%1&apos;""",
"All Files (*)": """所有文件 (*)""",
"Receiving signal. Perform range test &amp; confirm.": """正在接收信号。执行距离测试 &amp; 确认""",

以下是对应要翻译部分的列表:

```
