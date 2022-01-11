// 小彭老师作业05：假装是多线程 HTTP 服务器 - 富连网大厂面试官觉得很赞
#include <functional>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <string>
#include <thread>
#include <unordered_map>
#include <mutex>
#include <shared_mutex>
#include <chrono>
#include <vector>
#include <future>

struct User {
    std::string password;
    std::string school;
    std::string phone;
};

std::shared_mutex get_mtx;
std::shared_mutex user_login_mtx;
std::unordered_map<std::string, User> users;
std::unordered_map<std::string, decltype(std::chrono::steady_clock::now())> has_login;  // 换成 std::chrono::seconds 之类的

// 作业要求1：把这些函数变成多线程安全的
// 提示：能正确利用 shared_mutex 加分，用 lock_guard 系列加分
std::string do_register(std::string username, std::string password, std::string school, std::string phone) {
    User user = {password, school, phone};
    std::unique_lock grd{get_mtx};
    if (users.emplace(username, user).second)
        return "注册成功";
    else
        return "用户名已被注册";
}

std::string do_login(std::string username, std::string password) {
    // 作业要求2：把这个登录计时器改成基于 chrono 的
    // long now = time(NULL);   // C 语言当前时间
    auto now = std::chrono::steady_clock::now();
    {
        std::shared_lock grd{user_login_mtx};
        if (has_login.find(username) != has_login.end())
        {
            // int sec = now - has_login.at(username);  // C 语言算时间差
            // 时间段类型：chrono::milliseconds
            auto  dt = now - has_login.at(username);
            auto sec = std::chrono::duration_cast<std::chrono::seconds>(dt).count();
            return std::to_string(sec) + "秒内登录过";
        }
    }
    {
        std::unique_lock grd{user_login_mtx};
        has_login[username] = now;
    }

    std::shared_lock grd{get_mtx};
    if (users.find(username) == users.end())
        return "用户名错误";
    if (users.at(username).password != password)
        return "密码错误";
    return "登录成功";
}

std::string do_queryuser(std::string username) {
    std::shared_lock grd{get_mtx};
    if (users.find(username) == users.end())
        return "没有该用户";
    auto &user = users.at(username);
    std::stringstream ss;
    ss << "用户名: " << username << std::endl;
    ss << "学校:" << user.school << std::endl;
    ss << "电话: " << user.phone << std::endl;
    return ss.str();
}


struct ThreadPool {
    std::vector<std::future<void>> m_pool;
    std::mutex m_mtx;

    void create(std::function<void()> start) {
        // 作业要求3：如何让这个线程保持在后台执行不要退出？
        // 提示：改成 async 和 future 且用法正确也可以加分
        std::future<void> fret = std::async(start);
        std::lock_guard grd{m_mtx};
        m_pool.push_back(std::move(fret));
    }

    ~ThreadPool()
    {
        for (auto &t : m_pool)
        {
            t.get();
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
            std::cout << do_register(test::username[rand() % 4], test::password[rand() % 4], test::school[rand() % 4], test::phone[rand() % 4]) << std::endl;
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