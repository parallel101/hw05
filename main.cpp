﻿// 小彭老师作业05：假装是多线程 HTTP 服务器 - 富连网大厂面试官觉得很赞
#include <functional>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <string>
#include <thread>
#include <map>
#include <mutex>
#include <shared_mutex>
#include "ThreadPool.h"
struct User
{
    std::string password;
    std::string school;
    std::string phone;
};

std::map<std::string, User> users;
std::map<std::string, std::chrono::steady_clock::time_point> has_login; // 换成 std::chrono::seconds 之类的
std::shared_mutex users_mtx;
std::shared_mutex login_mtx;
// 作业要求1：把这些函数变成多线程安全的
// 提示：能正确利用 shared_mutex 加分，用 lock_guard 系列加分
std::string do_register(const std::string& username, const std::string &password,const std::string& school, const std::string& phone)
{
    std::unique_lock grd(users_mtx);
    User user = {password, school, phone};
    if (users.emplace(username, user).second)
        return "注册成功";
    else
        return "用户名已被注册";
}

std::string do_login(const std::string &username, const std::string &password)
{
    // 作业要求2：把这个登录计时器改成基于 chrono 的

    std::unique_lock unique_grd(login_mtx);
    auto now = std::chrono::steady_clock::now();
    if (has_login.find(username) != has_login.end())
    {
        auto sec = now - has_login.at(username); // C 语言算时间差
        return std::to_string(std::chrono::duration_cast<std::chrono::seconds>(sec).count()) + "秒内登录过";
    }
    has_login[username] = now;
    unique_grd.unlock();

    std::shared_lock shard_grd(users_mtx);
    if (users.find(username) == users.end())
        return "用户名错误";
    if (users.at(username).password != password)
        return "密码错误";
    return "登录成功";
}

std::string do_queryuser(const std::string &username)
{
    std::shared_lock grd(users_mtx);
    if (users.find(username) == users.end()){
        return "用户不存在";
    }
        
    auto &user = users.at(username);
    std::stringstream ss;
    ss << "用户名: " << username << std::endl;
    ss << "学校:" << user.school << std::endl;
    ss << "电话: " << user.phone << std::endl;
    return ss.str();
}


namespace test
{ // 测试用例？出水用力！
    std::string username[] = {"张心欣", "王鑫磊", "彭于斌", "胡原名"};
    std::string password[] = {"hellojob", "anti-job42", "cihou233", "reCihou_!"};
    std::string school[] = {"九百八十五大鞋", "浙江大鞋", "剑桥大鞋", "麻绳理工鞋院"};
    std::string phone[] = {"110", "119", "120", "12315"};
}

ThreadPool tpool(std::thread::hardware_concurrency());

int main()
{
    constexpr int n = 262144;

    for (int i = 0; i <n ; i++)
    {
        tpool.create([&]
                     { std::cout << do_register(test::username[rand() % 4], test::password[rand() % 4], test::school[rand() % 4], test::phone[rand() % 4]) << std::endl; });
        tpool.create([&]
                     { std::cout << do_login(test::username[rand() % 4], test::password[rand() % 4]) << std::endl; });
        tpool.create([&]
                     { std::cout << do_queryuser(test::username[rand() % 4]) << std::endl; });
    }
  
   
    // 作业要求4：等待 tpool 中所有线程都结束后再退出
    return 0;
}
