// 小彭老师作业05：假装是多线程 HTTP 服务器 - 富连网大厂面试官觉得很赞
#include <functional>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <string>
#include <thread>
#include <map>
#include <mutex>
#include <shared_mutex>
#include <chrono>
#include <atomic>
#include "MTQueue.h"


struct User {
    std::string password;
    std::string school;
    std::string phone;
};

std::map<std::string, User> users;
std::shared_mutex user_mtx;
std::map<std::string, std::chrono::steady_clock::time_point> has_login;  
std::shared_mutex login_mtx;

// 作业要求1：把这些函数变成多线程安全的
// 提示：能正确利用 shared_mutex 加分，用 lock_guard 系列加分
std::string do_register(std::string username, std::string password, std::string school, std::string phone) {
    User user = {password, school, phone};
    std::unique_lock lck(user_mtx);
    if (users.emplace(username, user).second)
        return "注册成功";
    else
        return "用户名已被注册";
}

std::string do_login(std::string username, std::string password) {
    // 作业要求2：把这个登录计时器改成基于 chrono 的
    auto now = std::chrono::steady_clock::now();  
    {
        std::shared_lock lck(login_mtx);
        if (has_login.find(username) != has_login.end()) {
            int64_t sec = std::chrono::duration_cast<std::chrono::seconds>
                (now - has_login.at(username)).count();
            return std::to_string(sec) + "秒内登录过";
        }
    }

    {
        std::unique_lock lck(login_mtx);
        has_login[username] = now;
    }

    { 
        std::shared_lock lck(user_mtx);
        if (users.find(username) == users.end())
            return "用户名错误";
        if (users.at(username).password != password)
            return "密码错误";
    }

    return "登录成功";
}

std::string do_queryuser(std::string username) {
    
    std::shared_lock lck(user_mtx);
    if (users.find(username) == users.end())
        return "No such user";
    auto &user = users.at(username);
    lck.unlock();
    
    std::stringstream ss;
    ss << "用户名: " << username << std::endl;
    ss << "学校:" << user.school << std::endl;
    ss << "电话: " << user.phone << std::endl;
    return ss.str();
}


struct ThreadPool {
private:
    MTQueue<std::function<void()>> m_producers;
    std::vector<std::thread> m_consumers;
    std::atomic<bool> m_shutdown;

public:
    ThreadPool(int const n_threads = 4)
        : m_consumers(std::vector<std::thread>(n_threads)), m_shutdown(false) {
        init_threads();
    }

    void init_threads() {
        for (auto &thread: m_consumers) {
            thread = std::thread([&] {
                while (!m_shutdown) {
                    auto food = m_producers.pop();
                    if (food) {
                        food();
                    }
                }
            });

        }
    }

    void create(std::function<void()> start) {
        // 作业要求3：如何让这个线程保持在后台执行不要退出？
        // 提示：改成 async 和 future 且用法正确也可以加分
        m_producers.push(start);
    }

    void shutdown() {
        m_shutdown = true;
        for (size_t i = 0; i < m_consumers.size(); i++) {
            m_producers.push(nullptr);
        }
    }

    ~ThreadPool() {
        if (!m_shutdown) {
            shutdown();
        }
        
        for (auto &thread: m_consumers) {
            thread.join();
        }
        
    }

};

ThreadPool tpool;


namespace test {  // 测试用例？出水用力！
    std::string username[] = {"张心欣", "王鑫磊", "彭于斌", "胡原名"};
    std::string password[] = {"hellojob", "anti-job42", "cihou233", "reCihou_!"};
    std::string school[] = {"九百八十五大鞋", "浙江大鞋", "剑桥大鞋", "麻绳理工鞋院"};
    std::string phone[] = {"110", "119", "120", "12315"};
}

int main() {
    for (int i = 0; i < 262144; i++) {
        tpool.create([&] {
            std::cout << do_register(test::username[rand() % 4], test::password[rand() % 4],
                    test::school[rand() % 4], test::phone[rand() % 4]) << std::endl;
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
