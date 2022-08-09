// 小彭老师作业05：假装是多线程 HTTP 服务器 - 富连网大厂面试官觉得很赞
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <map>
#include <chrono>
#include <shared_mutex>
#include <condition_variable>
#include <queue>

#define MAX_THREAD_N 8

//改用类实现，原来的几个变量、数据结构和函数都是强耦合关系，应该封装在一起
class Users {
    struct User {
        std::string password;
        std::string school;
        std::string phone;
    };

    std::map<std::string, User> users;
    std::map<std::string, std::chrono::_V2::steady_clock::time_point> has_login; // 换成 std::chrono::seconds 之类的
    std::shared_mutex users_mtx;
    std::shared_mutex has_login_mtx;

public:
    // 作业要求1：把这些函数变成多线程安全的
    // 提示：能正确利用 shared_mutex 加分，用 lock_guard 系列加分
    std::string do_register(std::string username, std::string password, std::string school, std::string phone) {
        std::cout << "in do_register" << std::endl;
        User user = {password, school, phone};
        std::unique_lock lck(users_mtx);
        if (users.emplace(username, user).second)
            return "注册成功";
        else
            return "用户名已被注册";
    }

    std::string do_login(std::string username, std::string password) {
        // 作业要求2：把这个登录计时器改成基于 chrono 的
        std::cout<<"in do_login"<<std::endl;
        auto now = std::chrono::steady_clock::now();
        std::shared_lock s_lck1(has_login_mtx);
        if (has_login.find(username) != has_login.end()) {
            auto dt = now - has_login[username];
            auto sec = std::chrono::duration_cast<std::chrono::seconds>(dt).count();  
            return std::to_string(sec) + "秒内登录过";
        }
        s_lck1.unlock();

        std::unique_lock u_lck1(has_login_mtx);
        has_login[username] = now;
        u_lck1.unlock();

        std::shared_lock s_lck2(users_mtx);
        if (users.find(username) == users.end())
            return "用户名错误";
        if (users.at(username).password != password)
            return "密码错误";
        return "登录成功";
    }

    std::string do_queryuser(std::string username) {
        std::cout<<"in do_queryuser"<<std::endl;
        std::stringstream ss;
        std::shared_lock lck(users_mtx);
        if(users.find(username)==users.end()){
            return ss.str();
        }
        auto &user = users.at(username);
        ss << "用户名: " << username << std::endl;
        ss << "学校:" << user.school << std::endl;
        ss << "电话: " << user.phone << std::endl;
        return ss.str();
    }

} users;

//改用类实现
class ThreadPool {
    int thread_num;
    std::vector<std::thread> threads;
    bool activate;//用来关闭工作线程

    std::queue<std::function<void()>> missions_queue;
    std::mutex mtx;//用来独占missions_queue
    std::condition_variable cv;//用来控制工作线程等待任务队列

public:
    ThreadPool(int num = MAX_THREAD_N):thread_num(num), threads(num), activate(true){
        for(int i = 0; i<thread_num; ++i){
            //使用cv实现，每个工作线程常驻，避免重复构造和析构std::thread，没有任务就wait，
            threads[i]=std::thread(&ThreadPool::working,this);
        }
    }
    ~ThreadPool(){
        while (missions_queue.size()){
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        activate=false;
        cv.notify_all();
        for(auto &t:threads){
            t.join();
        }
    }
    void create(std::function<void()> start)
    {
        // 作业要求3：如何让这个线程保持在后台执行不要退出？
        // 提示：改成 async 和 future 且用法正确也可以加分
        //用cv改成工作线程常驻，等待任务队列的形式。
        std::unique_lock lck(mtx);
        missions_queue.push(start);
        lck.unlock();
        cv.notify_one();
    }

    void working(){
        std::unique_lock lck(mtx);
        while(activate){
            if(missions_queue.size()==0){
                cv.wait(lck);
                continue;
            }
            std::function<void()> f{[](){}};
            f = missions_queue.front();
            missions_queue.pop();
            lck.unlock();
            f();
            lck.lock();
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
    for (int i = 0; i < 262144; i++) {
        tpool.create([&] {
            std::cout << users.do_register(test::username[rand() % 4], test::password[rand() % 4], test::school[rand() % 4], test::phone[rand() % 4]) << std::endl;
        });
        tpool.create([&] {
            std::cout << users.do_login(test::username[rand() % 4], test::password[rand() % 4]) << std::endl;
        });
        tpool.create([&] {
            std::cout << users.do_queryuser(test::username[rand() % 4]) << std::endl;
        });
    }

    // 作业要求4：等待 tpool 中所有线程都结束后再退出
    // 结束自动调用析构函数，析构函数会逐个join
    return 0;
}
