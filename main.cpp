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
#include <mutex>
#include <shared_mutex>
#include <future>

struct User {
    std::string password;
    std::string school;
    std::string phone;
};

std::map<std::string, User> users;
// std::map<std::string, long> has_login;  // 换成 std::chrono::seconds 之类的
std::map<std::string, std::chrono::steady_clock::time_point> has_login;

std::mutex users_mtx;
std::mutex login_mtx;
std::shared_mutex users_smtx;
std::shared_mutex login_smtx;

// 作业要求1：把这些函数变成多线程安全的
// 提示：能正确利用 shared_mutex 加分，用 lock_guard 系列加分
std::string do_register(std::string username, std::string password, std::string school, std::string phone) {
    User user = {password, school, phone};
    auto grd=std::unique_lock<std::shared_mutex>{users_smtx}; //使用写锁
    if (users.emplace(username, user).second)
        return "注册成功";
    else
        return "用户名已被注册";
}

std::string do_login(std::string username, std::string password) {
    // 作业要求2：把这个登录计时器改成基于 chrono 的
    // long now = time(NULL);   // C 语言当前时间
    auto now=std::chrono::steady_clock::now(); //c++ 语言时间差
    auto rgrd=std::shared_lock<std::shared_mutex>{login_smtx}; //使用读锁
    if (has_login.find(username) != has_login.end()) {
        auto sec = now - has_login.at(username);  // C++ 语言算时间差
        int64_t seconds = std::chrono::duration_cast<std::chrono::seconds>(sec).count();  
        return std::to_string(seconds) + "秒内登录过";
    }
    rgrd.unlock();

    auto wgrd=std::unique_lock<std::shared_mutex>{login_smtx}; //使用写锁
    has_login[username] = now;

    if (users.find(username) == users.end())
        return "用户名错误";
    if (users.at(username).password != password)
        return "密码错误";
    return "登录成功";
}

std::string do_queryuser(std::string username) {
    auto grd=std::shared_lock<std::shared_mutex>{users_smtx}; //使用读锁
    if (users.find(username) == users.end()){ //如果查询不到，直接返回
        return std::string("查询失败\n");
    }
    auto &user = users.at(username);
    std::stringstream ss;
    ss << "用户名: " << username << std::endl;
    ss << "学校:" << user.school << std::endl;
    ss << "电话: " << user.phone << std::endl;
    return ss.str();
}


struct ThreadPool {
    std::vector<std::thread> pools;
    std::vector<std::future<void>> futures;

    // void create(std::function<void()> start) { // 
    //     // 作业要求3：如何让这个线程保持在后台执行不要退出？
    //     // 提示：改成 async 和 future 且用法正确也可以加分
    //     std::thread thr(start);
    //     pools.push_back(std::move(thr));
    // }

    void create(std::function<void()> start) {
        // async 和 future方式
        auto f=std::async(start);
        futures.push_back(std::move(f));
    }

    void join(){
        for (auto & t:pools){
            t.join();
        }
    }

    void wait(){
        for(auto & f:futures){
            f.get();
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
    for (int i = 0; i < 300; i++) { //限制线程数量，否则会资源耗尽推出
    // for (int i = 0; i < 262144; i++) { //线程池未使用固定线程数量的的方式，导致无法处理如此多的线程
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
    // tpool.join(); //std::thread方式等待子线程结束
    tpool.wait(); // std::async方式等待子线程结束

    return 0;
}
