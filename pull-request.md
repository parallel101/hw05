## 完成部分

### 1. 基础部分
修改 main.cpp，修复并改进其中的 HTTP 服务器：

- 把 `login`, `register` 等函数变成多线程安全的 - 10 分：使用的是读写锁方式实现
- 把 `login` 的登录计时器改成基于 chrono 的 - 5 分：使用chrono标准库
- 能利用 `shared_mutex` 区分读和写 - 10 分：使用std::unique_lock和std::shared_lock区分
- 用 `lock_guard` 系列符合 RAII 思想 - 5 分：使用的是std::unique_lock和std::shared_lock
- 让 ThreadPool::create 创建的线程保持后台运行不要退出 - 15 分：将线程保存在ThreadPool.pools中
- 等待 tpool 中所有线程都结束后再退出 - 5 分：通过调用join()函数

### 2. 附加部分

- 使用async和future的方式实现多线程

## 存在的问题

目前**不能**处理线程数量大的情况，考虑后续使用condition_variable的方式。固定线程池中工作线程的数量，在每一个工作线程中执行用户线程，直到处理完所有用户的线程