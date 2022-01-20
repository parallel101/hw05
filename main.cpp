// 小彭老师作业05：假装是多线程 HTTP 服务器 - 富连网大厂面试官觉得很赞
#include <functional>
#include <iostream>
#include <mutex>
#include <sstream>
#include <cstdlib>
#include <string>
#include <thread>
#include <map>
#include <shared_mutex>
#include <chrono>
#include <atomic>

typedef std::shared_lock<std::shared_mutex> RLock;
typedef std::lock_guard<std::shared_mutex> WLock;

struct User {
    std::string password;
    std::string school;
    std::string phone;
};
std::shared_mutex s_mtx;

std::map<std::string, User> users;
std::map<std::string, std::chrono::time_point<std::chrono::system_clock> > has_login;  // 换成 std::chrono::seconds 之类的

// 作业要求1：把这些函数变成多线程安全的
// 提示：能正确利用 shared_mutex 加分，用 lock_guard 系列加分
std::string do_register(std::string username, std::string password, std::string school, std::string phone) {
    WLock lock_guard(s_mtx);
    User user = {password, school, phone};
    if (users.emplace(username, user).second)
        return "注册成功";
    else
        return "用户名已被注册";
}

std::string do_login(std::string username, std::string password) {
    RLock lock_guard(s_mtx);
    // 作业要求2：把这个登录计时器改成基于 chrono 的
    // long now = time(NULL);   // C 语言当前时间
    std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
    if (has_login.find(username) != has_login.end()) {
        int sec = std::chrono::duration_cast<std::chrono::seconds>(now - has_login.at(username)).count();
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
    RLock lock_guard(s_mtx);
    if(users.find(username) == users.end()) return "";
    auto &user = users.at(username);
    std::stringstream ss;
    ss << "用户名: " << username << std::endl;
    ss << "学校:" << user.school << std::endl;
    ss << "电话: " << user.phone << std::endl;
    return ss.str();
}


namespace thread_pool{

    class join_threads {
    private:
        std::vector<std::thread> &threads;
    public:
        explicit join_threads(std::vector<std::thread> &threads_) :
                threads(threads_) {}

        ~join_threads() {
            for (unsigned long i = 0; i < threads.size(); ++i) {
                if (threads[i].joinable()) threads[i].join();
            }
        }

    };

    // push使用head_mutex，pop使用tail_mutex，使用细粒度锁可以提高并行。
    template <typename T>
    class queue{
    private:
        struct node{
            std::shared_ptr<T> data;
            std::unique_ptr<node> next;
        };
        std::mutex head_mtx_;
        std::mutex tail_mtx_;
        std::unique_ptr<node> head_;
        node *tail_;
        std::condition_variable data_cond_;

        node* get_tail(){
            std::lock_guard<std::mutex> lockGuard(tail_mtx_);
            return tail_;
        }
        std::unique_ptr<node> pop_head(){
            std::unique_ptr<node> old_head = std::move(head_);
            head_ = std::move(old_head->next);
            return old_head;
        }

        std::unique_ptr<node> try_pop_head(){
            std::lock_guard<std::mutex> lockGuard(head_mtx_);
            if (head_.get() == get_tail()){
                return std::unique_ptr<node>();
            }
            return pop_head();
        }
        std::unique_ptr<node> try_pop_head(T &value){
            std::lock_guard<std::mutex> lockGuard(head_mtx_);
            if (head_.get() == get_tail()){
                return std::unique_ptr<node>();
            }
            value = std::move(*head_->data);
            return pop_head();
        }

    public:
        queue(): head_(new node), tail_(head_.get()){}
        queue(const queue &other) = delete;
        queue& operator=(const queue &other) = delete;

        void push(const T &&new_value){
            std::shared_ptr<T> new_data = std::make_shared<T>(new_value);
            std::unique_ptr<node> tmp(new node);
            node* new_tail = tmp.get();
            {
                std::lock_guard<std::mutex> lockGuard(tail_mtx_);
                tail_->data = new_data;
                tail_->next = std::move(tmp);
                tail_ = new_tail;
            }
            data_cond_.notify_all();
        }

        void push(T &&new_value){
            std::shared_ptr<T> new_data = std::make_shared<T>(std::move(new_value));
            std::unique_ptr<node> tmp(new node);
            node* new_tail = tmp.get();
            {
                std::lock_guard<std::mutex> lockGuard(tail_mtx_);
                tail_->data = new_data;
                tail_->next = std::move(tmp);
                tail_ = new_tail;
            }
            data_cond_.notify_all();
        }

        std::shared_ptr<T> try_pop(){
            std::unique_ptr<node> old_head = try_pop_head();
            return old_head ? old_head->data : std::shared_ptr<T>();
        }

        bool try_pop(T &value){
            std::unique_ptr<node> old_head = try_pop_head(value);
            return old_head != nullptr;
        }


        std::shared_ptr<T> wait_and_pop(){
            std::unique_lock<std::mutex> uniqueLock(head_mtx_);
            while (head_.get() == get_tail()){
                data_cond_.wait(uniqueLock);
            }
            std::unique_ptr<node> old_head = pop_head();
            return old_head->data;
        }
        void wait_and_pop(T &value){
            std::unique_lock<std::mutex> uniqueLock(head_mtx_);
            while (head_.get() == get_tail()){
                data_cond_.wait(uniqueLock);
            }
            std::unique_ptr<node> old_head = pop_head();
            value = std::move(*old_head->data);
        }

        bool empty(){
            std::lock_guard<std::mutex> lockGuard(head_mtx_);
            return head_.get() == get_tail();
        }
    };
    
    class thread_pool {

    private:

        // TODO the declaration order is important.
        std::atomic_bool done;
        queue<std::function<void()> > work_queue;
        std::vector<std::thread> threads;
        join_threads joiner;

        void worker_thread(){
            while (!done){
                std::function<void()> task;
                if (work_queue.try_pop(task)){
                    // std::cout << "task\n";
                    task();
                }else{
//                    std::this_thread::sleep_for(std::chrono::seconds(1));
//                    std::cout << "yield\n";
                    std::this_thread::yield();

                }
            }
        }

    public:
        thread_pool(): done(false), joiner(threads){
            unsigned const thread_count = std::thread::hardware_concurrency();

            try {
                for (int i = 0; i < thread_count; ++i) {
                    threads.emplace_back(&thread_pool::worker_thread, this); // add work thread
                }
            } catch (...) {
                done = true;
                throw;
            }
        }

        ~thread_pool(){
            while(!work_queue.empty()){
                std::this_thread::yield();
            }
            done = true; // TODO, when done is set to true,
                         // the worker thread will exit, even there are still tasks in work_queue.
        }

        void create(std::function<void()> f){
            work_queue.push(std::move(f));
        }

    };

}



thread_pool::thread_pool tpool;


namespace test {  // 测试用例？出水用力！
std::string username[] = {"张心欣", "王鑫磊", "彭于斌", "胡原名"};
std::string password[] = {"hellojob", "anti-job42", "cihou233", "reCihou_!"};
std::string school[] = {"九百八十五大鞋", "浙江大鞋", "剑桥大鞋", "麻绳理工鞋院"};
std::string phone[] = {"110", "119", "120", "12315"};
}

int main() {
    std::cout << do_register(test::username[rand() % 4], test::password[rand() % 4], test::school[rand() % 4], test::phone[rand() % 4]) << std::endl;
    std::cout << do_login(test::username[rand() % 4], test::password[rand() % 4]) << std::endl;
    std::cout << do_queryuser(test::username[rand() % 4]) << std::endl;

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
    // std::this_thread::sleep_for(std::chrono::seconds(100));

    // 作业要求4：等待 tpool 中所有线程都结束后再退出
    return 0;
}
