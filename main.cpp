// 小彭老师作业05：假装是多线程 HTTP 服务器 - 富连网大厂面试官觉得很赞
#include <functional>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <string>
#include <thread>
#include <map>
#include <chrono>
#include <mutex>
#include <shared_mutex>
#include <future>
#include <condition_variable>
#include <atomic>
#include <queue>


struct User {
    std::string password;
    std::string school;
    std::string phone;
};
std::shared_mutex sm1;//users mutex
std::shared_mutex sm2;//has login mutex
std::mutex out;//output

std::map<std::string, User> users;
std::map<std::string, std::chrono::steady_clock::time_point> has_login;  // 换成 std::chrono::seconds 之类的

// 作业要求1：把这些函数变成多线程安全的
// 提示：能正确利用 shared_mutex 加分，用 lock_guard 系列加分
std::string do_register(std::string username, std::string password, std::string school, std::string phone) {

    User user = {password, school, phone};
    std::unique_lock grd(sm1);
    if (users.emplace(username, user).second)
    {
        return "注册成功";
    }
    else
    {
        return "用户名已被注册";
    }
}

std::string do_login(std::string username, std::string password) {
    // 作业要求2：把这个登录计时器改成基于 chrono 的

    std::unique_lock grd_has_login(sm2);
    auto now = std::chrono::steady_clock::now();   // C++ 语言当前时间
    if (has_login.find(username) != has_login.end()) {
        auto sec = now - has_login.at(username);  // C++ 语言算时间差
        using double_s = std::chrono::duration<double>;
        return std::to_string(std::chrono::duration_cast<double_s>(sec).count()) + "秒内登录过";
    }
    has_login.emplace(username,now);
    grd_has_login.unlock();

    std::shared_lock grd(sm1);
    if (users.find(username) == users.end())
    {
        return "用户名错误";
    }
    if (users.at(username).password != password)
        return "密码错误";
    return "登录成功";
}

std::string do_queryuser(std::string username) {
    std::shared_lock grd(sm1);
    if(users.find(username) == users.end())
        return "";
    auto &user = users.at(username);
    grd.unlock();
    std::stringstream ss;
    ss << "用户名: " << username << std::endl;
    ss << "学校:" << user.school << std::endl;
    ss << "电话: " << user.phone << std::endl;
    return ss.str();
}

struct FunctionPool {

private:
    std::atomic<bool> accept_functions ;
    std::queue<std::function<void()>> function_queue;
    std::mutex m;
    std::condition_variable data_condition;
public:
    FunctionPool():function_queue(),m(), data_condition(),accept_functions(true){
    }

    void push(std::function<void()>&& task) {
        std::unique_lock grd(m);
        function_queue.push(std::move(task));
        grd.unlock();
        data_condition.notify_one();
    }

    void infinite_loop()
    {
        std::function<void()> func;
        while(true)
        {
            {
                std::unique_lock grd(m);
                data_condition.wait(grd, [this](){
                    return !function_queue.empty()||!accept_functions;
                });
                if(!accept_functions && function_queue.empty())
                {
                    return;
                }
                func = function_queue.front();
                function_queue.pop();
            }
            func();
        }
    }
    void done()
    {
        std::unique_lock grd(m);
        accept_functions = false;
        grd.unlock();
        data_condition.notify_all();
    }


    ~FunctionPool() {
    }


};

struct ThreadPool {
    int num_threads = std::thread::hardware_concurrency();
    std::vector<std::thread> thread_pool;
    FunctionPool func_pool;

    ThreadPool()
    {
        for(int i=0;i<num_threads;i++)
        {
            thread_pool.push_back(
                std::thread(
                    &FunctionPool::infinite_loop,
                    &func_pool
                )
            );
        }
    }

    void create(std::function<void()> start) {
        // 作业要求3：如何让这个线程保持在后台执行不要退出？
        // 提示：改成 async 和 future 且用法正确也可以加分
        // v.emplace_back(std::move(thr));
        func_pool.push(std::move(start));
    }
    ~ThreadPool()
    {
        func_pool.done();
        for(auto&& thr:thread_pool)
        {
            thr.join();
        }
    }
};

ThreadPool tpool;

// ThreadPool tpool;


namespace test {  // 测试用例？出水用力！
    std::string username[] = {"张心欣", "王鑫磊", "彭于斌", "胡原名"};
    std::string password[] = {"hellojob", "anti-job42", "cihou233", "reCihou_!"};
    std::string school[] = {"九百八十五大鞋", "浙江大鞋", "剑桥大鞋", "麻绳理工鞋院"};
    std::string phone[] = {"110", "119", "120", "12315"};
}

int main() {
    for (int i = 0; i < 262144; i++) {//262144
        tpool.create([&] {
            std::cout << do_register(test::username[rand() % 4],
            test::password[rand() % 4], test::school[rand() % 4],
            test::phone[rand() % 4]) << std::endl;
        });
        tpool.create([&] {
            std::cout << do_login(test::username[rand() % 4],
            test::password[rand() % 4]) << std::endl;
        });
        tpool.create([&] {
            std::cout << do_queryuser(test::username[rand() % 4]) << std::endl;
        });
    }

    // 作业要求4：等待 tpool 中所有线程都结束后再退出
    return 0;
}
