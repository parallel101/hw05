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
#include <queue>


struct User {
    std::string password;
    std::string school;
    std::string phone;
    friend std::ostream& operator<< (std::ostream& stream, const User& user) {
        stream << user.password + " " + user.school + " " + user.phone;
        return stream;
    }
};

std::map<std::string, User> users;
std::map<std::string, std::chrono::seconds> has_login;  // 换成 std::chrono::seconds 之类的

std::shared_mutex mutex_users;
std::shared_mutex mutex_login;

// 作业要求1：把这些函数变成多线程安全的
// 提示：能正确利用 shared_mutex 加分，用 lock_guard 系列加分
std::string do_register(std::string username, std::string password, std::string school, std::string phone) {
    User user = {password, school, phone};
    // write
    std::unique_lock lck(mutex_users);
    if (users.emplace(username, user).second)
        return "注册成功";
    else
        return "用户名已被注册";
}

std::string do_login(std::string username, std::string password) {
    // 作业要求2：把这个登录计时器改成基于 chrono 的
    auto now = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch());   
    {
        // read and write
        std::unique_lock lck(mutex_login);
        if (has_login.find(username) != has_login.end()) {
            int sec = (now - has_login.at(username)).count();  // C 语言算时间差
            return std::to_string(sec) + "秒内登录过";
        }
        has_login[username] = now;
    }
    // read
    std::shared_lock lck(mutex_users);
    if (users.find(username) == users.end())
        return "用户名错误";
    if (users.at(username).password != password)
        return "密码错误";
    return "登录成功";
}

std::string do_queryuser(std::string username) {
    // read
    std::shared_lock lck(mutex_users);
    if (users.find(username) == users.end()) return "查询失败, 无效的用户名";

    auto &user = users.at(username);
    std::stringstream ss;
    ss << "用户名: " << username << std::endl;
    ss << "学校:" << user.school << std::endl;
    ss << "电话: " << user.phone << std::endl;
    return ss.str();
}


struct ThreadPool {
    using Job = std::function<void()>;
    ThreadPool() 
        : m_stop_flag(false)
        , m_poll_size(std::thread::hardware_concurrency())
    {
        for (size_t i = 0; i < m_poll_size; ++i) {
            m_workers.emplace_back(
                [&]() {
                    while (true)
                    {
                        Job job;
                        // 持续等待新任务进来
                        {
                            std::unique_lock lck(m_mutex);
                            m_condition.wait(lck, [&](){
                                return !m_jobs.empty() || m_stop_flag;
                            });
                            // 停止并且无任务
                            if (m_stop_flag && m_jobs.empty()) return;

                            job = m_jobs.front();
                            m_jobs.pop();
                        }
                        job();
                    }
                }
            );
        }
    }
    void create(std::function<void()> start) {
        // 作业要求3：如何让这个线程保持在后台执行不要退出？
        // 提示：改成 async 和 future 且用法正确也可以加分
        {
            std::unique_lock lck(m_mutex);
            m_jobs.push(start);
        }
        m_condition.notify_one();
    }
    ~ThreadPool() {
        m_stop_flag = true;
        m_condition.notify_all();
        for (auto& th : m_workers) {
            th.join();
        }
    }
private:
    bool m_stop_flag;
    size_t m_poll_size;
    std::condition_variable m_condition;
    std::mutex m_mutex;
    std::queue<Job> m_jobs;
    std::vector<std::thread> m_workers;
};

ThreadPool tpool;


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
    return 0;
}
