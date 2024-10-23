//
// Created by jjangchan on 2024/10/17.
//

#ifndef VISUAL_CAMP_THREADPOOL_H
#define VISUAL_CAMP_THREADPOOL_H

#include <queue>
#include <thread>
#include <future>
#include <functional>
#include <chrono>
#include <atomic>

//=================================================== extern ===========================================================
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
//======================================================================================================================

class ThreadPool{
private:
    // [thread 보관 컨테이너]
    std::vector<std::thread> threads;
    // [job functions 보관 컨테이너]
    std::deque<std::function<void()>> functions;

    std::mutex work_mutex;
    std::condition_variable notify_cv;
    std::atomic<bool> status;

public:
    ThreadPool(const std::size_t n):status(true){
        assign_threads(n);
    }

    ~ThreadPool(){
        stop_thread();
        destroy_thead();
    }

public:
    void stop(){
        stop_thread();
    }

    template<class F, class... Args>
    std::future<typename std::result_of<F(Args...)>::type> add_job(
            F&& f, Args&&... args){
        if(!status) throw std::runtime_error("....stop thread....");
        // --> 쓰레드 functor return type 정의
        using return_type = typename std::result_of<F(Args...)>::type;
        // --> 비동기적으로 수행할 함수 보관 및 promise 에 리턴값 설정
        // --> job 은 지역변수 이므로, add_job 함수가 끝나면 소멸 되므로, shared_ptr로 instance 자원 공유
        auto job = std::make_shared<std::packaged_task<return_type()>>(
                std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        std::future<return_type> result = job->get_future();
        {
            std::lock_guard<std::mutex> lock(work_mutex);
            functions.push_back([job](){(*job)();}); // --> job 공유
        }
        notify_cv.notify_one();

        return result;
    }

private:

    // [ thread 할당 함수]
    void assign_threads(const std::size_t n){

        if(status && !threads.empty()) assert(!".....working thread......");

        // --> 자원 존재 하면 제거
        destroy_thead();

        threads.reserve(n);
        status = true;
        for(std::size_t i = 0; i < n; i++){
            threads.emplace_back([this](){this->work_thread();});
        }
    }

    // [ 컨테이너 자원 제거 함수]
    void destroy_thead(){
        status = false;
        // --> 모든 쓰레드 꺠우기
        notify_cv.notify_all();
        {
            if(!functions.empty()){
                // --> 할당 되어 있는 job 모두 제거
                std::lock_guard<std::mutex> lock(work_mutex);
                functions.clear();
            }
        }
        {
            if(!threads.empty()){
                // -->  할당 되어 있는 thread 모두 제거
                std::lock_guard<std::mutex> lock(work_mutex);
                threads.clear();
            }
        }
    }

    // [ 쓰레드 자원 반환 함수]
    void stop_thread(){
        status = false;
        notify_cv.notify_all();
        for(auto& t : threads){
            if(t.joinable()) t.join();
        }
    }

    // [ job 큐에 작업 처리 함수]
    void work_thread(){
        while(status){
            // lock (critical section)
            std::unique_lock<std::mutex> lock(work_mutex);
            notify_cv.wait(lock, [this](){
                // 큐에 작업 노드가 들어오면 작업 시작 or 쓰레드풀이 중단되면
                return !this->functions.empty() || !status;
            });
            if(!status && functions.empty()) return;

            std::function<void()> func = std::move(functions.front());
            functions.pop_front();
            lock.unlock();
            // 작업 시작
            func();
        }
    }


};

#endif //VISUAL_CAMP_THREADPOOL_H
