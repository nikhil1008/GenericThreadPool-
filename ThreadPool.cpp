#include <stdio.h>
#include <stdio.h>
#include<iostream> 
#include<future>
#include<functional>
#include<thread>
#include<optional>
#include <queue>
#include <atomic> 

using namespace std;
template <class T> class ThreadSafeQueue;
template <typename T> class FunctionWrapper;
template <typename T> class Pool;

template<class T>
class ThreadSafeQueue {

private:
    queue<T> queue_;
    mutex mu;
public:

    ThreadSafeQueue() {}
    ThreadSafeQueue(const ThreadSafeQueue&& other) {
        lock_guard<mutex> lg(mu);
        queue_ = move(other->queue_);
    }

    optional<T> pop() {
        lock_guard<mutex> lg(mu);
        if (queue_.empty()) {
            return nullopt;
        }
        auto tmp = move(queue_.front());
        queue_.pop();
        return move(tmp);
    }

    void push(T&& val) {
        lock_guard<mutex> lg(mu);
        queue_.push(move(val));
    }

    T front() {
        lock_guard<mutex> lg(mu);
        return queue_.front();
    }

    unsigned int size() {
        lock_guard<mutex> lg(mu);
        return queue_.size();
    }
};


template <typename T>
class FunctionWrapper {
private:
    packaged_task<T()> mTask;
public:
    FunctionWrapper(packaged_task<T()> task) {
        mTask = move(task);
    }
    packaged_task<T()> getTask() {
        return move(mTask);
    }
};


template <typename T>
class Pool {
public:
    atomic_bool done;
    vector<thread> threads;
    ThreadSafeQueue<FunctionWrapper<T>> mQueue;
    void worker() {
        while (not done) {
            auto mWarpper = mQueue.pop();
            if (mWarpper) {
                //cout<<" Processing task using thread :- "<<hash<thread::id>{}(this_thread::get_id())<<"\n";
                auto curr_task = move((*mWarpper).getTask());
                curr_task();
            }
            this_thread::sleep_for (chrono::seconds(3));
        }
        cout << "Thread done = " << done << "\n";
    }

    Pool() {
        threads.push_back(move(thread(&Pool::worker, this)));
        threads.push_back(move(thread(&Pool::worker, this)));
        
    }

    ~Pool() {
        cout << "\nThreadPool Destructor called, joining threads ...\n";
        for (auto& th : threads) th.join();
    }

    future<T> submit(function<T()> func) {
        packaged_task<T()> task(func);
        future<T> fu = task.get_future();
        FunctionWrapper<T> mTask(move(task));
        this_thread::sleep_for(chrono::seconds(1));
        mQueue.push(move(mTask));
        return fu;
    }
};
