// 小彭老师作业05：假装是多线程 HTTP 服务器 - 富连网大厂面试官觉得很赞
#include <functional>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <cstdlib>
#include <string>
#include <thread>
#include <map>
#include <shared_mutex>
#include "MTQueue.h"


struct User {
    std::string password;
    std::string school;
    std::string phone;
};

std::unordered_map <std::string, User> users;
std::shared_mutex g_user_mu{};

std::map<std::string, std::chrono::system_clock::time_point> has_login;  // 换成 std::chrono::seconds 之类的
std::shared_mutex g_login_mu{};

// 作业要求1：把这些函数变成多线程安全的
// 提示：能正确利用 shared_mutex 加分，用 lock_guard 系列加分
bool do_register(std::string username, std::string password, std::string school, std::string phone) {
    User user = {std::move(password), std::move(school), std::move(phone)};
    std::unique_lock u_lock(g_user_mu);
    if (users.emplace(std::move(username), user).second)
        return "注册成功";
    else
        return "用户名已被注册";
}

std::string do_login(std::string username, std::string password) {
    // 作业要求2：把这个登录计时器改成基于 chrono 的
    auto now = std::chrono::system_clock::now();
    std::unique_lock login_lock(g_login_mu);
    if (has_login.find(username) != has_login.end()) {
        auto sec = now - has_login.at(username);  // C 语言算时间差
        return std::to_string(std::chrono::duration_cast<std::chrono::seconds>(sec).count()) + "秒内登录过";
    }
    has_login[username] = now;
    login_lock.unlock();

    std::shared_lock user_lock(g_user_mu);
    if (users.find(username) == users.end())
        return "用户名错误";
    if (users.at(username).password != password)
        return "密码错误";
    return "登录成功";
}

std::string do_queryuser(std::string username) {
    std::shared_lock user_lock(g_user_mu);
    if (users.find(username) == users.end()) {
        return {};
    }

    auto user = users.at(username);
    user_lock.unlock();

    std::stringstream ss;
    ss << "用户名: " << username << std::endl;
    ss << "学校:" << user.school << std::endl;
    ss << "电话: " << user.phone << std::endl;
    return ss.str();

}


struct ThreadPool {
    using task_t = std::function<void()>;

    explicit ThreadPool(std::size_t thread_nbr = std::thread::hardware_concurrency()) {
        create_threads(thread_nbr);
    }

    void create(std::function<void()> task) {
        // 作业要求3：如何让这个线程保持在后台执行不要退出？
        // 提示：改成 async 和 future 且用法正确也可以加分

        this->task_queue_.push(std::move(task));
    }

    void create_threads(std::size_t thread_pool_size) {
        for (std::size_t i = 0; i != thread_pool_size; ++i) {
            std::thread t([&] {
                              while (!this->user_stopped_) {
                                  auto task = this->task_queue_.pop();
                                  if (task) {
                                      task();
                                  }
                              }
                          }
            );
            this->thread_container_.push_back(std::move(t));
        }
    }

    void stop() {
        this->user_stopped_ = true;
        for (std::size_t i = 0; i != this->thread_container_.size(); ++i) {
            this->task_queue_.push(nullptr); //the end condition for thread stop.
        }
    }


    ~ThreadPool() {

        if (!this->user_stopped_) {
            stop();
        }

        for (auto &th: thread_container_) {
            th.join();
        }
    }


private:
    std::atomic_bool user_stopped_{false};
    MTQueue<task_t> task_queue_;
    std::vector<std::thread> thread_container_;
};

ThreadPool tpool(std::thread::hardware_concurrency());


namespace test {  // 测试用例？出水用力！
    std::string username[] = {"张心欣", "王鑫磊", "彭于斌", "胡原名"};
    std::string password[] = {"hellojob", "anti-job42", "cihou233", "reCihou_!"};
    std::string school[] = {"九百八十五大鞋", "浙江大鞋", "剑桥大鞋", "麻绳理工鞋院"};
    std::string phone[] = {"110", "119", "120", "12315"};
}


int main() {
    for (int i = 0; i < 262144; i++) {
        tpool.create([&] {
            std::cout << do_register(test::username[rand() % 4], test::password[rand() % 4], test::school[rand() % 4],
                                     test::phone[rand() % 4]) << std::endl;
        });
        tpool.create([&] {
            std::cout << do_login(test::username[rand() % 4], test::password[rand() % 4]) << std::endl;
        });
        tpool.create([&] {
            std::cout << do_queryuser(test::username[rand() % 4]) << std::endl;
        });
    }

    // 作业要求4：等待 tpool 中所有线程都结束后再退出
    return 0;
}
