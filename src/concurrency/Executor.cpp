#include <afina/concurrency/Executor.h>

namespace Afina {
namespace Concurrency {

    Executor::Executor (int low_watermark, int hight_watermark, int max_queue_size, std::chrono::milliseconds idle_time):
        low_watermark(low_watermark),
        hight_watermark(hight_watermark),
        max_queue_size(max_queue_size),
        idle_time(idle_time) {
        std::lock_guard<std::mutex> lock(mutex);
        for (int i = 0; i < low_watermark; i++) {
            std::thread(perform, this).detach();
            working_threads++;

        }
        now_threads = working_threads;
        state = State::kRun;
    }

    Executor::~Executor() {
        Stop(true);

    }

    void Executor::Stop(bool await) {


       state = State::kStopping;
       empty_condition.notify_all();


       if (await) {
           std::unique_lock<std::mutex> _lock(mutex_);
           end_condition.wait(_lock, [this]{ return now_threads == 0; });

       }

       state = State::kStopped;

    }

    void perform(Executor *executor) {
        std::function<void()> task;

        while(true) {
            std::unique_lock<std::mutex> lock(executor->mutex);
            executor->working_threads--;

            bool result = executor->empty_condition.wait_for(lock, executor->idle_time,
                            [executor]{ return !(executor->tasks.empty()) || (executor->state != Executor::State::kRun);});
            if (result) {
                    if (executor->state == Executor::State::kRun || (executor->state == Executor::State::kStopping && !executor->tasks.empty())) {
                        task = executor->tasks.front();
                        executor->tasks.pop_front();
                        executor->now_threads++;


                    } else {
                        executor->working_threads--;
                        if (executor->working_threads == 0) {
                            executor->end_condition.notify_one();
                        }
                    }
                    break;
            } else {
                if (executor->now_threads > executor->low_watermark) {
                    executor->now_threads--;
                    break;


            }



        }
        task();


    }

}
}
} // namespace Afina
