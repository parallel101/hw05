// 小彭老师作业05：假装是多线程 HTTP 服务器 - 富连网大厂面试官觉得很赞
#include <MTQueue.h>
#include <condition_variable>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <map>
#include <mutex>
#include <queue>
#include <shared_mutex>
#include <sstream>
#include <string>
#include <thread>

struct User {
  std::string password;
  std::string school;
  std::string phone;
};

std::shared_mutex lk_users;
std::shared_mutex lk_login;
std::map<std::string, User> users;
std::map<std::string, long> has_login;  // 换成 std::chrono::seconds 之类的

// 作业要求1：把这些函数变成多线程安全的
// 提示：能正确利用 shared_mutex 加分，用 lock_guard 系列加分
std::string do_register(std::string username, std::string password,
                        std::string school, std::string phone) {
  User user = {password, school, phone};
  std::unique_lock<std::shared_mutex> lock(lk_users);
  if (users.emplace(username, user).second)
    return "注册成功";
  else
    return "用户名已被注册";
}

std::string do_login(std::string username, std::string password) {
  // 作业要求2：把这个登录计时器改成基于 chrono 的
  long now = time(NULL);  // C 语言当前时间
  std::unique_lock<std::shared_mutex> lk(lk_login);
  if (has_login.find(username) != has_login.end()) {
    int sec = now - has_login.at(username);  // C 语言算时间差
    return std::to_string(sec) + "秒内登录过";
  }
  has_login[username] = now;
  lk.unlock();

  std::shared_lock<std::shared_mutex> lk_u(lk_users);
  if (users.find(username) == users.end()) return "用户名错误";
  if (users.at(username).password != password) return "密码错误";
  return "登录成功";
}

std::string do_queryuser(std::string username) {
  std::shared_lock<std::shared_mutex> lk(lk_users);
  if (users.find(username) == users.end()) {
    return "用户名错误";
  }
  auto& user = users.at(username);
  std::stringstream ss;
  ss << "用户名: " << username << std::endl;
  ss << "学校:" << user.school << std::endl;
  ss << "电话: " << user.phone << std::endl;
  return ss.str();
}

class ThreadPool {
 public:
  explicit ThreadPool(size_t num_workers) : stop_(false) {
    std::cout << "worker number: " << num_workers << std::endl;
    workers_.reserve(num_workers);
    for (size_t i = 0; i < num_workers; ++i) {
      workers_.push_back(std::thread([&]() -> void {
        while (true) {
          std::unique_lock<std::mutex> lk(mtx_tasks_);
          cv_tasks_.wait(lk, [&]() { return stop_ || !tasks_.empty(); });
          if (stop_) {
            break;
          }
          auto task = std::move(tasks_.front());
          tasks_.pop();
          task();
        }
      }));
    }
  }

  ThreadPool(const ThreadPool& rhs) = delete;
  ThreadPool& operator=(const ThreadPool& rhs) = delete;
  ThreadPool(ThreadPool&& rhs) = default;
  ThreadPool& operator=(ThreadPool&& rhs) = default;

  ~ThreadPool() {
    stop_ = true;
    cv_tasks_.notify_all();
    for (auto& worker : workers_) worker.join();
    std::cout << "ThreadPool finished!" << std::endl;
  }

  void create(std::function<void()> start) {
    // 作业要求3：如何让这个线程保持在后台执行不要退出？
    // 提示：改成 async 和 future 且用法正确也可以加分
    std::lock_guard<std::mutex> lk(mtx_tasks_);
    tasks_.push(std::move(start));
    cv_tasks_.notify_one();
  }

 private:
  using task = std::function<void()>;
  std::queue<task> tasks_;
  std::vector<std::thread> workers_;
  std::mutex mtx_tasks_;
  std::condition_variable cv_tasks_;
  bool stop_;
};

namespace test {  // 测试用例？出水用力！
std::string username[] = {"张心欣", "王鑫磊", "彭于斌", "胡原名"};
std::string password[] = {"hellojob", "anti-job42", "cihou233", "reCihou_!"};
std::string school[] = {"九百八十五大鞋", "浙江大鞋", "剑桥大鞋",
                        "麻绳理工鞋院"};
std::string phone[] = {"110", "119", "120", "12315"};
}  // namespace test

int main() {
  constexpr int M = 262144;
  // constexpr int M = 10000;

  ThreadPool tpool(std::thread::hardware_concurrency());

  for (int i = 0; i < M; i++) {
    tpool.create([&] {
      std::cout << do_register(
                       test::username[rand() % 4], test::password[rand() % 4],
                       test::school[rand() % 4], test::phone[rand() % 4])
                << std::endl;
    });
    tpool.create([&] {
      std::cout << do_login(test::username[rand() % 4],
                            test::password[rand() % 4])
                << std::endl;
    });
    tpool.create([&] {
      std::cout << do_queryuser(test::username[rand() % 4]) << std::endl;
    });
  }

  // 作业要求4：等待 tpool 中所有线程都结束后再退出
  return 0;
}
