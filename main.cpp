#include <iostream>
#include <functional>
#include <sstream>
#include <cstdlib>
#include <string>
#include <thread>
#include <map>


struct User {
    std::string password;
    std::string school;
    std::string phone;
};

// 作业要求1：把这个 users 的访问变成多线程安全的
std::map<std::string, User> users;

std::string do_login(std::string username, std::string password) {
    if (users.find(username) == users.end())
        return "用户名错误";
    if (users.at(username).password != password)
        return "密码错误";
    return "登录成功";
}

std::string do_queryuser(std::string username) {
    auto &user = users.at(username);
    std::stringstream ss;
    ss << "用户名: " << username << std::endl;
    ss << "学校:" << user.school << std::endl;
    ss << "电话: " << user.phone << std::endl;
    return ss.str();
}

bool do_register(std::string username, std::string password, std::string school, std::string phone) {
    User user = {password, school, phone};
    if (users.emplace(username, user).second)
        return "注册成功";
    else
        return "用户名已被注册";
}


struct ThreadPool {
    void create(std::function<void()> start) {
        // 作业要求2：如何让这个线程保持后台执行不要退出？
        std::thread thr(start);
    }
};

ThreadPool tpool;


namespace test {  // 出水用例
std::string username[] = {"张心欣", "王鑫磊", "彭于斌", "胡原名"};
std::string password[] = {"hellojob", "anti-job42", "cihou233", "reCihou_!"};
std::string school[] = {"九百八十五大鞋", "浙江大鞋", "剑桥大鞋", "麻绳理工鞋院"};
std::string phone[] = {"110", "119", "120", "12315"};
}

int main() {
    for (int i = 0; i < 1024; i++) {
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

    // 作业要求3：等待所有线程都结束后再退出
    return 0;
}
