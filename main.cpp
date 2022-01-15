// 小彭老师作业05：假装是多线程 HTTP 服务器 - 富连网大厂面试官觉得很赞
#include <chrono>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <map>
#include <shared_mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

struct User {
        std::string password;
        std::string school;
        std::string phone;
};

std::map<std::string, User> users;
std::shared_mutex user_lock;

std::map<std::string, std::chrono::time_point<std::chrono::steady_clock>>
    has_login; // 换成 std::chrono::seconds 之类的
std::shared_mutex login_lock;

// 作业要求1：把这些函数变成多线程安全的
// 提示：能正确利用 shared_mutex 加分，用 lock_guard 系列加分
std::string do_register(std::string username, std::string password,
                        std::string school, std::string phone) {
        User user = {password, school, phone};

        std::lock_guard<std::shared_mutex> a(user_lock);
        if (users.emplace(username, user).second)
                return "注册成功";
        else
                return "用户名已被注册";
}

std::string do_login(std::string username, std::string password) {
        // 作业要求2：把这个登录计时器改成基于 chrono 的
        {
                std::shared_lock<std::shared_mutex> lck(user_lock);
                if (users.find(username) == users.end())
                        return "用户名错误";
                if (users.at(username).password != password)
                        return "密码错误";
        }

        auto now = std::chrono::steady_clock::now(); // C 语言当前时间
        {
                std::shared_lock<std::shared_mutex> lck(login_lock);
                if (has_login.find(username) != has_login.end()) {
                        auto sec =
                            std::chrono::duration_cast<std::chrono::seconds>(
                                now - has_login.at(username)); // C 语言算时间差
                        std::stringstream ss;
                        ss << sec.count() << "秒内登陆过";
                        return ss.str();
                }
        }

        {
                std::lock_guard<std::shared_mutex> lck(login_lock);
                if (has_login.find(username) != has_login.end()) {
                        has_login[username] = now;
                }
        }

        return "登录成功";
}

std::string do_queryuser(std::string username) {

        try {
                std::shared_lock<std::shared_mutex> lck(user_lock);
                auto &user = users.at(username);
                std::stringstream ss;
                ss << "用户名: " << username << std::endl;
                ss << "学校:" << user.school << std::endl;
                ss << "电话: " << user.phone << std::endl;
                return ss.str();
        } catch (std::exception &error) {
                std::cerr << "User not found" << std::endl;
                return "";
        }
}

struct ThreadPool {
        void create(std::function<void()> start) {
                // 作业要求3：如何让这个线程保持在后台执行不要退出？
                // 提示：改成 async 和 future 且用法正确也可以加分
                thread_pool.emplace_back(std::thread(start));
        }

        void join_all() {
                for (auto &thr : thread_pool) {
                        thr.join();
                }
        }

      private:
        std::vector<std::thread> thread_pool;
};

ThreadPool tpool;

namespace test { // 测试用例？出水用力！
std::string username[] = {"张心欣", "王鑫磊", "彭于斌", "胡原名"};
std::string password[] = {"hellojob", "anti-job42", "cihou233", "reCihou_!"};
std::string school[] = {"九百八十五大鞋", "浙江大鞋", "剑桥大鞋",
                        "麻绳理工鞋院"};
std::string phone[] = {"110", "119", "120", "12315"};
} // namespace test

int main() {
        for (int i = 0; i < 262144; i++) {
                tpool.create([&] {
                        std::cout << do_register(test::username[rand() % 4],
                                                 test::password[rand() % 4],
                                                 test::school[rand() % 4],
                                                 test::phone[rand() % 4])
                                  << std::endl;
                });
                tpool.create([&] {
                        std::cout << do_login(test::username[rand() % 4],
                                              test::password[rand() % 4])
                                  << std::endl;
                });
                tpool.create([&] {
                        std::cout << do_queryuser(test::username[rand() % 4])
                                  << std::endl;
                });
        }

        // 作业要求4：等待 tpool 中所有线程都结束后再退出
        tpool.join_all();
        return 0;
}
