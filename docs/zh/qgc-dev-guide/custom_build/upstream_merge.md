# 使您的自定义构建保持最新

## 存储库设置

* 从主QGC仓库创建一个新的存储库。 不要克隆，创建新的仓库，从主QGC仓库初始化它。
* 您现在可以克隆上面的仓库来完成您的工作并从中创建pull请求。
* 在你的克隆中创建一个叫做“mavlink”的远程，指向主QGC 仓库. 
  *     git remote add mavlink https://github.com/mavlink/qgroundcontrol.git

## 上游合并

我们将更新您的自定义构建的过程称为最新的QGC位和“上游合并”。 以下是如何执行此操作的示例：

* 首先确保您的本地主人与您自己的回购主人是最新的。
* 创建一个分支以进行所有更改： 
  *     git checkout -b UpstreamMerge

* 从QGC中提取最新的位： 
  *     git pull mavlink master
  
  * 您将获得一个编辑器来更新合并更新。 他们很好，只是 ```:q``` 为了退出
* 现在，您需要更新自定义构建中的资源： 
  *     cd custom
  
  *     python updateqrc.py

* 全部构建以确保没有问题。
* 你现在完成了。 您可以将其作为针对您的仓库的拉动提交，或者您希望将更改提交到主仓库中。

注意：这假设您的自定义构建基于QGC master。 如果它基于Stable分支，则使用稳定分支名称替换master。