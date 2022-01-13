/**
 * @file main.cpp
 * @author The one who cannot be named.
 * @brief Thie is the 5th homework of HPC
 * @version 0.1
 * @date 2022-01-10
 * 
 * @copyright Copyright (c) 2022
 * 
 */

// 小彭老师作业05：假装是多线程 HTTP 服务器 - 富连网大厂面试官觉得很赞
#include <functional>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <string>
#include <thread>
#include <map>

#include<vector>
#include<mutex>
#include<algorithm>
#include<shared_mutex>
struct User {
    std::string password;
    std::string school;
    std::string phone;
};

std::map<std::string, User> users;
std::map<std::string, std::chrono::time_point<std::chrono::steady_clock>> has_login;
std::shared_mutex mtx;
std::mutex mtx_login;


// 作业要求1：把这些函数变成多线程安全的
// 提示：能正确利用 shared_mutex 加分，用 lock_guard 系列加分
std::string do_register(std::string username, std::string password, std::string school, std::string phone) {
    std::unique_lock guard(mtx);
    User user = {password, school, phone};
    if (users.emplace(username, user).second)
        return "注册成功";
    else
        return "用户名已被注册";
}

std::string do_login(std::string username, std::string password) {
    // 作业要求2：把这个登录计时器改成基于 chrono 的
    std::shared_lock guard(mtx);
    auto now = std::chrono::steady_clock::now();   // Current time acquired from std::chrono
    if (has_login.find(username) != has_login.end()) {
        auto time = std::chrono::steady_clock::now();
        auto sec = std::chrono::duration_cast<std::chrono::seconds>(time - has_login.at(username)).count();
        return std::to_string(sec) + "秒内登录过";
    }
    std::unique_lock guard_(mtx_login);
    has_login[username] = now;
    guard_.unlock();
    if (users.find(username) == users.end())
        return "用户名错误";
    if (users.at(username).password != password)
        return "密码错误";
    return "登录成功";
}

std::string do_queryuser(std::string username) {
    std::shared_lock guard(mtx);
    if(users.find(username)!=users.end()){
        auto &user = users.at(username);
        std::stringstream ss;
        ss << "用户名: " << username << std::endl;
        ss << "学校:" << user.school << std::endl;
        ss << "电话: " << user.phone << std::endl;
        return ss.str();
    }else
        return "Cannot find user!" + username;
}

/**
 * @brief This pool is not completely thread safe.
 * 
 */
struct ThreadPool {
    /**
     * @brief I assume every public member function is unlocked. 
     * 
     */
    public:
        ThreadPool(int length=100):size(length) {
            temp.resize(length);
            work.resize(length);
            idx = 0;
            }
        /**
         * @brief implement concurrent processing method
         * @todo implement asynchornize processing methods.
         * @param start: request queries. 
         */
        void create(std::function<void()> start) {
            // 作业要求3：如何让这个线程保持在后台执行不要退出？
            // 提示：改成 async 和 future 且用法正确也可以加分
            std::thread thr(start);
            temp[idx] = std::move(thr);
            idx++;
            if(idx==size){
                std::swap(temp, work);
                task_processing(work);
                release.join();
                idx = 0;
            }
            if(release.joinable()) release.join();
        }
        ~ThreadPool(){
            for(int i = 0; i<idx;i++) temp[i].join();
        }
    private:
        /**
         * @brief It should be awared that we should assume every mutex is locked when implementing private member function
         * 
         */
        unsigned long size;
        std::atomic<unsigned long >idx;
        std::vector<std::thread> temp, work;
        std::thread release;
        void task_processing(std::vector<std::thread> &work){
            release = std::move(std::thread([&]{
                std::for_each(work.begin(), work.end(), std::mem_fn(&std::thread::join));
                work.clear();
                work.resize(size);
            }));
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
    auto tick = std::chrono::steady_clock::now();
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
    // have already implemented in the deconstruction function of ThreadPool.
    auto tock = std::chrono::steady_clock::now();
    std::string ret = "Duration: "+std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(tock - tick).count())+"ms";
    std::cout<<ret<<std::endl;
    return 0;
}
