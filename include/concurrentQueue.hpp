
#include <mutex>
#include <queue>

template<typename T>
class concurrentQueue
{

private:
    std::queue<T> queue;
    mutable std::mutex mutex;
    const size_t maxSize;

public:
    concurrentQueue(size_t size) : maxSize(size) {}
    
    bool try_push(T data)
    {
        bool done = false;
        std::unique_lock<std::mutex>  lock(mutex);
        if(queue.size() < maxSize ){
            queue.push(std::move(data));
            done = true;
        }
        return done;
    }

    bool empty() const
    {
        std::unique_lock<std::mutex>  lock(mutex);
        return queue.empty();
    }

    bool try_pop(T& data)
    {
        std::unique_lock<std::mutex>  lock(mutex);
        if(queue.empty())
        {
            return false;
        }
        data = std::move(queue.front());
        queue.pop();
        return true;
    }
};
