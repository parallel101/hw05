/**
 * @file main.cpp
 * @author Zhao Gong (gongzhao1995@outlook.com)
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
struct User {
    std::string password;
    std::string school;
    std::string phone;
};

std::map<std::string, User> users;
// std::map<std::string, long> has_login;  // 换成 std::chrono::seconds 之类的
std::map<std::string, std::chrono::time_point<std::chrono::steady_clock>> has_login;
std::mutex mtx_reg, mtx_log, mtx_que, mtx_release;


// 作业要求1：把这些函数变成多线程安全的
// 提示：能正确利用 shared_mutex 加分，用 lock_guard 系列加分
std::string do_register(std::string username, std::string password, std::string school, std::string phone) {
    // mtx.lock();
    std::lock_guard guard(mtx_reg);
    User user = {password, school, phone};
    if (users.emplace(username, user).second)
        return "注册成功";
    else
        return "用户名已被注册";
    // mtx.unlock();
}

// std::string do_login(std::string username, std::string password) {
//     // 作业要求2：把这个登录计时器改成基于 chrono 的
//     long now = time(NULL);   // C 语言当前时间
//     if (has_login.find(username) != has_login.end()) {
//         int sec = now - has_login.at(username);  // C 语言算时间差
//         return std::to_string(sec) + "秒内登录过";
//     }
//     std::lock_guard guard(mtx);
//     has_login[username] = now;

//     if (users.find(username) == users.end())
//         return "用户名错误";
//     if (users.at(username).password != password)
//         return "密码错误";
//     return "登录成功";
// }

std::string do_login(std::string username, std::string password) {
    // 作业要求2：把这个登录计时器改成基于 chrono 的
    std::lock_guard guard(mtx_log);
    auto now = std::chrono::steady_clock::now();   // Current time acquired from std::chrono
    if (has_login.find(username) != has_login.end()) {
        auto time = std::chrono::steady_clock::now();
        auto sec = std::chrono::duration_cast<std::chrono::seconds>(time - has_login.at(username)).count();
        return std::to_string(sec) + "秒内登录过";
    }
    
    has_login[username] = now;
    if (users.find(username) == users.end())
        return "用户名错误";
    if (users.at(username).password != password)
        return "密码错误";
    return "登录成功";
}

std::string do_queryuser(std::string username) {
    std::lock_guard guard(mtx_que);
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


class ThreadPool {
    /**
     * @brief This pool is thread safe.
     * 
     */
    public:
        ThreadPool(int length=1000):size(length) {
            temp.resize(length);
            work.resize(length);
            idx = 0;
            }
        /**
         * @brief implement concurrent processing method
         * @todo implement asynchornize processing methods.
         * @param start 
         */
        void create(std::function<void()> start) {
            // 作业要求3：如何让这个线程保持在后台执行不要退出？
            // 提示：改成 async 和 future 且用法正确也可以加分
            std::thread thr(start);
            temp[idx] = std::move(thr);
            idx++;
            if(idx==size){
                std::swap(temp, work);
                auto release = std::thread([&]{
                    std::lock_guard guard(mtx_release);
                    std::for_each(work.begin(), work.end(), std::mem_fn(&std::thread::join));
                    work.clear();
                    work.resize(size);
                    });
                release.join();
                idx = 0;
            }
            
        }
        ~ThreadPool(){
            std::for_each(temp.begin(), temp.begin()+idx, std::mem_fn(&std::thread::join));
        }
    private:
        unsigned long size;
        unsigned long idx;
        std::vector<std::thread> temp, work;
        void task_processing(std::vector<std::thread> &work){
            std::for_each(work.begin(), work.end(), std::mem_fn(&std::thread::join));
            work.clear();
            work.resize(size);
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
        std::cout<<"---------------"<<i<<"---------------"<<std::endl;
        tpool.create([&] {
            std::cout << do_register(test::username[rand() % 4], test::password[rand() % 4], test::school[rand() % 4], test::phone[rand() % 4]) << std::endl;
            // do_register(test::username[rand() % 4], test::password[rand() % 4], test::school[rand() % 4], test::phone[rand() % 4]);
        });
        tpool.create([&] {
            std::cout << do_login(test::username[rand() % 4], test::password[rand() % 4]) << std::endl;
            // do_login(test::username[rand() % 4], test::password[rand() % 4]);
        });
        tpool.create([&] {
            std::cout << do_queryuser(test::username[rand() % 4]) << std::endl;
            // do_queryuser(test::username[rand() % 4]);
        });
    }
    // 作业要求4：等待 tpool 中所有线程都结束后再退出
    // have already implemented in the definition of ThreadPool.
    auto tock = std::chrono::steady_clock::now();

    std::cout<<std::endl<<"Duration: "<<std::chrono::duration_cast<std::chrono::milliseconds>(tock - tick).count()<<"ms"<<std::endl;
    return 0;
}
