// 小彭老师作业05：假装是多线程 HTTP 服务器 - 富连网大厂面试官觉得很赞
#include <functional>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <string>
#include <thread>
#include <map>
#include <vector>
#include <chrono>
#include <shared_mutex>
#include <mutex>


struct User {
    std::string password;
    std::string school;
    std::string phone;
};

std::shared_mutex users_mutex;
std::shared_mutex login_mutex;

std::map<std::string, User> users;
std::map<std::string, std::chrono::steady_clock::time_point> has_login;  // 换成 std::chrono::seconds 之类的

// 作业要求1：把这些函数变成多线程安全的
// 提示：能正确利用 shared_mutex 加分，用 lock_guard 系列加分
std::string do_register(std::string username, std::string password, std::string school, std::string phone) {
    User user = {password, school, phone};
    std::unique_lock grd(users_mutex);
    if (users.emplace(username, user).second)
        return "用户名：[" + username + "]注册成功!\n";
    else
        return "用户名:[" + username + "]已被注册!\n";
}

std::string do_login(std::string username, std::string password) {
    // 作业要求2：把这个登录计时器改成基于 chrono 的
    std::unique_lock login_grd(login_mutex);
    auto now = std::chrono::steady_clock::now();  // 获取当前时间点
    if (has_login.find(username) != has_login.end()) {
        int sec = std::chrono::duration_cast<std::chrono::milliseconds>(now - has_login.at(username)).count();
        return username + "在" + std::to_string(sec) + "ms内登录过\n";
    }
    has_login[username] = now;
    login_grd.unlock();
    std::shared_lock grd(users_mutex);
    if (users.find(username) == users.end())
        return "用户名错误\n";
    if (users.at(username).password != password)
        return "密码错误\n";
    return "登录成功\n";
}

std::string do_queryuser(std::string username) {
    std::shared_lock grd(users_mutex);
    if (users.find(username) == users.end()) {
        return "用户不存在\n";
    }
    auto &user = users.at(username);
    std::stringstream ss;
    ss << "正在查询用户:" + username << std::endl;
    ss << "用户名: " << username << std::endl;
    ss << "学校:" << user.school << std::endl;
    ss << "电话: " << user.phone << std::endl;
    return ss.str();
}


struct ThreadPool {
    std::vector<std::thread> threads;
    void create(std::function<void()> start) {
        // 作业要求3：如何让这个线程保持在后台执行不要退出？
        // 提示：改成 async 和 future 且用法正确也可以加分
        threads.push_back(std::thread(start));
    }
    ~ThreadPool() {
        for (auto &thr : threads) {
            thr.join();
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
    for (int i = 0; i < 100; i++) {
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
