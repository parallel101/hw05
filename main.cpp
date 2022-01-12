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

struct User
{
    std::string password;
    std::string school;
    std::string phone;
};

std::map<std::string, User> users;
std::map<std::string, long> has_login;  // 换成 std::chrono::seconds 之类的

std::shared_mutex mtx_user;
std::shared_mutex mtx_login;

using ReadLock = std::shared_lock<std::shared_mutex>;
using WriteLock = std::lock_guard<std::shared_mutex>;

// 作业要求1：把这些函数变成多线程安全的
// 提示：能正确利用 shared_mutex 加分，用 lock_guard 系列加分
std::string do_register(std::string username, std::string password, std::string school, std::string phone) {
    WriteLock lock(mtx_user);

    User user = {password, school, phone};
    if (users.emplace(username, user).second)
        return "注册成功\n";
    else
        return "用户名已被注册\n";
}

std::string do_login(std::string username, std::string password) {
    ReadLock lock_usr(mtx_user);
    WriteLock lock_log(mtx_login);

    // 作业要求2：把这个登录计时器改成基于 chrono 的
    long now = time(NULL);   // C 语言当前时间
    if (has_login.find(username) != has_login.end())
    {
        int sec = now - has_login.at(username);  // C 语言算时间差
        return username + "在" + std::to_string(sec) + "秒内登录过\n";
    }
    has_login[username] = now;

    if (users.find(username) == users.end())
        return "用户名错误\n";
    if (users.at(username).password != password)
        return "密码错误\n";
    return "登录成功\n";
}

std::string do_queryuser(std::string username) {
    ReadLock lock(mtx_user);

    if(users.find(username) == users.end()) return "用户未找到\n";
    auto &user = users.at(username);
    std::stringstream ss;
    ss << "用户名: " << username << std::endl;
    ss << "学校: " << user.school << std::endl;
    ss << "电话: " << user.phone << std::endl;
    return ss.str();
}


struct ThreadPool {
    void create(std::function<void()> start) {
        // 作业要求3：如何让这个线程保持在后台执行不要退出？
        // 提示：改成 async 和 future 且用法正确也可以加分
        std::thread thr(start);
        
        m_pool.push_back(std::move(thr));
    }

    void join() {
        for(auto&& t : m_pool) t.join();
    }

    std::vector<std::thread> m_pool;
};

ThreadPool tpool;


namespace test
{
    // 测试用例？出水用力！
    std::string username[] = {"张心欣", "王鑫磊", "彭于斌", "胡原名"};
    std::string password[] = {"hellojob", "anti-job42", "cihou233", "reCihou_!"};
    std::string school[] = {"九百八十五大鞋", "浙江大鞋", "剑桥大鞋", "麻绳理工鞋院"};
    std::string phone[] = {"110", "119", "120", "12315"};
}

int main() {
    for (int i = 0; i < 100; i++)
    {
        size_t randomNums[7];
        for(int i = 0; i < 7; ++i) randomNums[i] = rand() % 4;
        
        tpool.create([&] {
            std::cout << do_register(test::username[randomNums[0]], test::password[randomNums[1]], test::school[randomNums[2]], test::phone[randomNums[3]]) << std::endl;
        });
        tpool.create([&] {
            std::cout << do_login(test::username[randomNums[4]], test::password[randomNums[5]]) << std::endl;
        });
        tpool.create([&] {
            std::cout << do_queryuser(test::username[randomNums[6]]) << std::endl;
        });
    }

    // 作业要求4：等待 tpool 中所有线程都结束后再退出
    tpool.join();

    return 0;
}
