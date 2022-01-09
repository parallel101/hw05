// 小彭老师作业05：假装是多线程 HTTP 服务器 - 富连网大厂面试官觉得很赞
#include <functional>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <string>
#include <thread>
#include <map>
#include <shared_mutex>
#include <chrono>
#include <vector>
#include <future>


struct User {
    std::string password;
    std::string school;
    std::string phone;
};

auto now = std::chrono::steady_clock::now();
std::map<std::string, User> users;
std::map<std::string, decltype(now)> has_login;  // 换成 std::chrono::seconds 之类的

// user资源的读写锁
std::shared_mutex mtx;


// 作业要求1：把这些函数变成多线程安全的
// 提示：能正确利用 shared_mutex 加分，用 lock_guard 系列加分
std::string do_register(std::string username, std::string password, std::string school, std::string phone) {
    User user = {password, school, phone};
    std::shared_lock grd{ mtx };
    if (users.emplace(username, user).second)
        return "注册成功";
    else
        return "用户名已被注册";
}

std::string do_login(std::string username, std::string password) {
    // 作业要求2：把这个登录计时器改成基于 chrono 的
    auto now = std::chrono::steady_clock::now();
    if (has_login.find(username) != has_login.end()) {
        auto dt = now - has_login.at(username);
        // 登录过期时间判断;
        if ( dt <= std::chrono::seconds(10) ) {
            int64_t sec = std::chrono::duration_cast<std::chrono::seconds>(dt).count();
            return std::to_string(sec) + "秒内登录过";
        }
    }
    has_login[username] = now;

    std::unique_lock grd {mtx};
    if (users.find(username) == users.end())
        return "用户名错误";
    if (users.at(username).password != password)
        return "密码错误";
    return "登录成功";
}

std::string do_queryuser(std::string username) {
    std::unique_lock grd {mtx};
    if ( users.find(username) == users.end() ) {
        return "没有该用户";
    }
    auto &user = users.at(username);
    std::stringstream ss;
    ss << "用户名: " << username << std::endl;
    ss << "学校:" << user.school << std::endl;
    ss << "电话: " << user.phone << std::endl;
    return ss.str();
}


// 简易线程池
struct ThreadPool {
    std::vector<std::thread> workers;
    std::vector<std::function<void()>> tasks;
    std::mutex mtx;
    std::condition_variable condition;
    bool stop;

    ThreadPool(int num_workers=4): stop(false) {
        for ( int i = 0; i < num_workers; ++i ) {
            workers.emplace_back([this] {
               while (true) {
                   std::function<void()> t;
                   {
                       std::unique_lock lck(mtx);
                       condition.wait(lck, [this] { return stop || !tasks.empty(); });
                       if (stop && tasks.empty()) return;
                       
                        t = std::move(tasks.back());
                        tasks.pop_back();
                   }
                   t();
               } 
            });
            std::cout << "Thread " << i << " Start" << std::endl;
        }
    }

    ~ThreadPool() {
        {
            // 给各个worker通知stop=true
            std::unique_lock lck{mtx};
            stop = true;
            condition.notify_all();
        }
        for ( auto& worker: workers ) {
            // 等待每个worker正确退出；
            // worker正确退出条件: stop=true && 任务队列为空;
            worker.join();
        }
    }
    
    void create(std::function<void()> start) {
        // 作业要求3：如何让这个线程保持在后台执行不要退出？
        // 提示：改成 async 和 future 且用法正确也可以加分
        // std::thread thr(start);
        std::unique_lock lck(mtx);
        tasks.push_back(start);
        condition.notify_one();
    }
};

ThreadPool tpool(4);


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
    // 线程池等待任务结束后退出

    return 0;
}
