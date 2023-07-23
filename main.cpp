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
std::shared_mutex mtx1;

std::map<std::string, long> has_login;  // 换成 std::chrono::seconds 之类的
std::shared_mutex mtx2;

// 作业要求1：把这些函数变成多线程安全的
// 提示：能正确利用 shared_mutex 加分，用 lock_guard 系列加分
std::string do_register(std::string username, std::string password, std::string school, std::string phone) {
    User user = {password, school, phone};
    std::shared_lock lck1(mtx1);
    if (users.emplace(username, user).second){
        std::cout << username << std::endl;
        return "注册成功";
    }
    else
        return "用户名已被注册";
    lck1.unlock();
}

std::string do_login(std::string username, std::string password) {
    // 作业要求2：把这个登录计时器改成基于 chrono 的
    std::unique_lock lck2(mtx2);

    auto now_chrono = std::chrono::steady_clock::now(); 
    auto utc_now = std::chrono::time_point_cast<std::chrono::seconds>(now_chrono);
    auto timestamp = utc_now.time_since_epoch().count();
    long now = static_cast<long>(timestamp);

    if (has_login.find(username) != has_login.end()) {
        long sec = now - has_login.at(username);  // C 语言算时间差

        return std::to_string(sec) + "秒内登录过";
    }
    has_login[username] = now;
    lck2.unlock();

    std::shared_lock lck3(mtx1);
    if (users.find(username) == users.end())
        return "用户名错误";
    if (users.at(username).password != password)
        return "密码错误";
    return "登录成功";
    mtx1.unlock_shared();
}

std::string do_queryuser(std::string username) {
    std::shared_lock lck4(mtx1);
    auto &user = users.at(username);
    std::stringstream ss;
    ss << "用户名: " << username << std::endl;
    ss << "学校:" << user.school << std::endl;
    ss << "电话: " << user.phone << std::endl;
    return ss.str();
    mtx1.unlock_shared();
}

struct ThreadPool {
    std::vector<std::thread> m_pool;
public:
    void create(std::function<void()> start) {
        // 作业要求3：如何让这个线程保持在后台执行不要退出？
        // 提示：改成 async 和 future 且用法正确也可以加分
        std::thread thr(start);
        m_pool.push_back(std::move(thr));
    }
    ~ThreadPool(){
        for (auto &t :m_pool) {
            t.join();
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
    for (int i = 0; i < 2; i++) {
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
