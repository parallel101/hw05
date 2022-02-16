#include <thread>
#include <mutex>
#include <vector>
#include <future>
#include "MTQueue.h"
#include <queue>
class ThreadPool
{
public:
    explicit ThreadPool(size_t num)
        : _threads_num(num), _stopped(false)
    {
        for (size_t i = 0; i < _threads_num; ++i)
        {
            _pool.emplace_back([this]()
                               {
                                   for (;;)
                                   {
                                       if (_stopped && _task_queue.empty())
                                           return;
                                       auto task = _task_queue.pop();
                                       task();
                                   }
                               });
        }
    }
    void create(std::function<void()> start)
    {
        // 作业要求3：如何让这个线程保持在后台执行不要退出？
        // 提示：改成 async 和 future 且用法正确也可以加分
        _task_queue.push(start);
    }
    ~ThreadPool()
    {
        _stopped = true;
        for (auto &th : _pool)
        {
            th.join();
        }
        
    }

private:
    std::atomic_bool _stopped;
    size_t _threads_num;
    std::vector<std::thread> _pool;
    MTQueue<std::function<void()>> _task_queue;
};

