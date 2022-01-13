# Submission

## Requests

* 修改 main.cpp，修复并改进其中的 HTTP 服务器：

- 把 `login`, `register` 等函数变成多线程安全的 - 10 分
- 把 `login` 的登录计时器改成基于 chrono 的 - 5 分
- 能利用 `shared_mutex` 区分读和写 - 10 分
- 用 `lock_guard` 系列符合 RAII 思想 - 5 分
- 让 ThreadPool::create 创建的线程保持后台运行不要退出 - 15 分
- 等待 tpool 中所有线程都结束后再退出 - 5 分

## Answer

### 修改 main.cpp，修复并改进其中的 HTTP 服务器 & 让 ThreadPool::create 创建的线程保持后台运行不要退出 & 等待 tpool 中所有线程都结束后再退出

* 新建std::vector\<std::thread\>来作为thread pool保存新建threads
* 定义deconstructor，使得thread pool中残留的threads能够正常执行并且退出
* 在实验中发现如果不对最大线程数进行限制的话，pool的扩容会变得越来越慢【注：Mac端仅仅是速度慢了，但是在WSL2上提示资源耗尽】 。
* 针对这种情况，放弃无限扩容的想法。限制最大线程数为100，并且采用query和worker两个线程分别cache和process。设计上来说应该是没问题，但是实现上并没有达到预期的目的。怀疑是实现的问题，有点眼高手低了。

## 把 `login`, `register` 等函数变成多线程安全的 - 10 分

加锁即可，具体方式见下

## 能利用 `shared_mutex` 区分读和写 & 用 `lock_guard` 系列符合 RAII 思想

创建了1个shared_mutex: mtx, 1个mutex: mtx_login

针对于写入的操作，采用了std::unique_lock来限制对于map的写入

针对于读取的操作，采用std::shared_lock来限制对map的读取

 ## 把 `login` 的登录计时器改成基于 chrono 的

见代码