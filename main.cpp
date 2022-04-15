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


struct User {
    std::string password;
    std::string school;
    std::string phone;
};

std::map<std::string, User> users;
// std::map<std::string, long> has_login;  // 换成 std::chrono::seconds 之类的
std::map<std::string, std::chrono::steady_clock::time_point> has_login;

std::mutex users_write_mutex;
std::shared_mutex users_read_mutex;
std::shared_mutex has_login_read_mutex;

// 作业要求1：把这些函数变成多线程安全的
// 提示：能正确利用 shared_mutex 加分，用 lock_guard 系列加分
std::string do_register(std::string username, std::string password, std::string school, std::string phone) {
    User user = {password, school, phone};
    std::shared_lock grd_r(users_read_mutex);
    if (users.find(username) == users.end()) {
        std::unique_lock grd_w(users_write_mutex);
        users.emplace(username, user);
        return "注册成功";
    } 
    else {
        return "用户名已被注册";
    }     
}
std::string do_login(std::string username, std::string password) {
    // 作业要求2：把这个登录计时器改成基于 chrono 的
    std::shared_lock grd(has_login_read_mutex);
    auto t0 = std::chrono::steady_clock::now();
    if (has_login.find(username) != has_login.end()) {
        // int sec = now - has_login.at(username);  // C 语言算时间差
        auto dt = t0 - has_login.at(username);
        int sec = std::chrono::duration_cast<std::chrono::seconds>(dt).count();
        return std::to_string(sec) + "秒内登录过";
    }
    has_login[username] = t0;

    if (users.find(username) == users.end())
        return "用户名错误";
    if (users.at(username).password != password)
        return "密码错误";
    return "登录成功";
}

std::string do_queryuser(std::string username) {
    if(users.find(username) == users.end()) return "没有这样的用户";
    auto &user = users.at(username);
    std::stringstream ss;
    ss << "用户名: " << username << std::endl;
    ss << "学校:" << user.school << std::endl;
    ss << "电话: " << user.phone << std::endl;
    return ss.str();
}

class ThreadPool {
public:
    void create(std::function<void()> start) {
        std::thread thr(start);
        pool.push_back(std::move(thr));
    }
    ~ThreadPool() {
        for(auto& t : pool) {
            t.join();
        }
    }
private:
    std::vector<std::thread> pool;
};

ThreadPool tpool;


namespace test {  // 测试用例？出水用力！
std::string username[] = {"张心欣", "王鑫磊", "彭于斌", "胡原名"};
std::string password[] = {"hellojob", "anti-job42", "cihou233", "reCihou_!"};
std::string school[] = {"九百八十五大鞋", "浙江大鞋", "剑桥大鞋", "麻绳理工鞋院"};
std::string phone[] = {"110", "119", "120", "12315"};
}

int main() {
    for (int i = 0; i < 1e4; i++) {  // 262144
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
    // 在ThreadPool的析构函数中等待线程结束
    return 0;
}
