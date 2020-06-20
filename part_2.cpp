//
// Created by dyj on 2020/6/20.
//

#include "thread.h"

//namespace dyj25 {
//    class join_threads
//    {
//        std::vector<std::thread> &threads;
//    public:
//        explicit join_threads(std::vector<std::thread> &threads_):threads(threads_){}
//        ~join_threads()
//        {
//            for(unsigned long i = 0 ; i < threads.size();++i)
//            {
//                if(threads[i].joinable())
//                    threads[i].join();
//            }
//        }
//    };
//
//    template <typename Iterator>
//    void parallel_sum(Iterator first, Iterator last) {
//        typedef typename Iterator::value_type value_type;
//
//        struct process_chunk
//        {
//            void operator()(Iterator begin, Iterator last,
//                            std::future<value_type>* previous_end_value,
//                            std::promise<value_type>* end_value)
//            {
//                try {
//                    Iterator end = last;
//                    ++end;
//                    std::partial_sum(begin, end, begin);
//                    if (previous_end_value)
//                    {
//                        value_type& addend = *(previous_end_value->get());
//                        *last += addend;
//                        if (end_value)
//                        {
//                            end_value->set_value(*last);
//                        }
//                        std::for_each(begin, last, [addend](value_type& item){item += addend;});
//                    } else if (end_value)
//                    {
//                        end_value->set_value(*last);
//                    }
//                }
//                catch (...) {
//                    if (end_value)
//                    {
//                        end_value->set_exception(std::current_exception());
//                    } else
//                    {
//                        throw;
//                    }
//                }
//            }
//        };
//
//        unsigned long const length = std::distance(first, last);
//
//        if (!length)
//            return;
//        unsigned long const min_per_thread = 25;
//        unsigned long const max_threads = (length + min_per_thread - 1) / min_per_thread;
//        unsigned long const hardware_threads = std::thread::hardware_concurrency();
//        unsigned long const num_threads = std::min(hardware_threads != 0 ? hardware_threads : 2, max_threads);
//        unsigned long const block_size = length / num_threads;
//        std::vector<std::thread> threads(num_threads - 1);
//        std::vector<std::promise<value_type > > end_values(num_threads - 1);
//        std::vector<std::future<value_type > > previous_end_values;
//
//        previous_end_values.reserve(num_threads - 1);
//        join_threads joiner(threads);
//
//        Iterator block_start = first;
//        for (unsigned long i = 0; i < (num_threads - 1); ++i)
//        {
//            Iterator block_last = block_start;
//            ++block_start;
//            previous_end_values.push_back(end_values[i].get_future());
//        }
//        Iterator final_element = block_start;
//        std::advance(final_element, std::distance(block_start, last) - 1);
//        process_chunk()(block_start, final_element,
//                        (num_threads > 1 ? &previous_end_values.back() : 0, 0));
//    }
//} // namespace dyj25

namespace dyj26
{
    class join_threads
    {
        std::vector<std::thread> &threads;
    public:
        explicit join_threads(std::vector<std::thread> &threads_):threads(threads_){}
        ~join_threads()
        {
            for(unsigned long i = 0 ; i < threads.size();++i)
            {
                if(threads[i].joinable())
                    threads[i].join();
            }
        }
    };

    template < typename T > class threadsafe_queue {
    private:
        mutable std::mutex		mut;
        std::queue< T >			data;
        std::condition_variable data_cond;

    public:
        threadsafe_queue() {}

        void push( T new_value ) {
            std::lock_guard< std::mutex > lock( mut );
            data.push( new_value );
            data_cond.notify_one();
        }

        void wait_and_pop( T& value ) {
            std::unique_lock< std::mutex > lock( mut );
            data_cond.wait( lock, [this] { return !data.empty(); } );
            value = std::move( data.front() );
            data.pop();
        }

        std::shared_ptr< T > wait_and_pop_share( T& value ) {
            std::unique_lock< std::mutex > lock( mut );
            data_cond.wait( lock, [this] { return !data.empty(); } );
            std::shared_ptr< T > res( make_shared( std::move( data.front() ) ) );
            data.pop();
            return res;
        }

        bool try_pop( T& value ) {
            std::unique_lock< std::mutex > lock( mut );
            if ( data.empty() )
                return false;
            value = std::move( data.front() );
            data.pop();
            return true;
        }

        std::shared_ptr< T > try_pop() {
            std::unique_lock< std::mutex > lock( mut );
            if ( data.empty() )
                return false;
            std::shared_ptr< T > res( std::make_shared< T >( std::move( data.front() ) ) );
            return res;
        }

        bool empty() {
            std::unique_lock< std::mutex > lock( mut );
            return data.empty();
        }
    };

    class thread_pool
    {
        std::atomic_bool done;
        threadsafe_queue<std::function<void()>> work_queue;
        std::vector<std::thread> threads;
        join_threads joiner;



    public:
        thread_pool() : done(false), joiner(threads)
        {
            unsigned const thread_count = std::thread::hardware_concurrency();
            try {
                for (unsigned long i = 0; i < thread_count; ++i) {
                    threads.push_back(std::thread(&thread_pool::start, this));
                }
            }
            catch (...) {
                done = true;
                throw;
            }
        }

        ~thread_pool() { done = true; }

        void start()
        {
            while (!done)
            {
                std::function<void()> task;
                if (work_queue.try_pop(task))
                {
                    task();
                } else
                {
                    std::this_thread::yield();
                }
            }
        }

        template<typename FunctionType>
        void submit(FunctionType f)
        {
            work_queue.push(std::function<void()>(f));
        }
    };

    thread_pool pool;

    void fun(){
        cout << this_thread::get_id() << endl;
    }

    void fun_2(int x)
    {
        for (int i = 0; i < x; ++i) {
            pool.submit(fun);
        }
    }

    void test_1()
    {
//        thread_pool pool;
//        for (int i = 0; i < 500000; ++i) {
////            thread t(fun);
//            pool.submit(fun);
//        }
        std::thread t(fun_2, 100);
        t.join();
        pool.start();
    }
}

/*
namespace dyj27
{
    class function_wrapper
    {
        struct impl_base {
            virtual void call() = 0;
            virtual ~impl_base(){}
        };

        std::unique_ptr<impl_base> impl;

        template<typename F>
        struct impl_type : impl_base
        {
            F f;
            impl_type(F&& f_):f(std::move(f_)){}
            void call() { f(); }
        };

    public:
        function_wrapper() = default;

        function_wrapper(const function_wrapper&&) = delete;

        template<typename F>
        function_wrapper(F&& f) : impl((new impl_type<F>(std::move(f)))) {}

        void operator()() { impl->call(); }

        function_wrapper(function_wrapper&& other) : impl(std::move(other.impl)) {}

        function_wrapper(function_wrapper&) = delete ;


        function_wrapper& operator=(function_wrapper&& other)
        {
            impl = std::move(other.impl);
            return *this;
        }

        function_wrapper& operator=(const function_wrapper&) = delete;
    };

    class thread_pool
    {
        dyj26::threadsafe_queue<function_wrapper> work_queue;
        std::atomic_bool done;
        dyj26::join_threads joiner;
        vector<thread> threads;
    public:
        thread_pool() : done(false), joiner(threads)
        {
            unsigned const thread_count = std::thread::hardware_concurrency();
            try {
                for (unsigned long i = 0; i < thread_count; ++i) {
                    threads.push_back(std::thread(&thread_pool::worker_thread, this));
                }
            }
            catch (...) {
                done = true;
                throw;
            }
        }

        ~thread_pool() { done = true; }
        void worker_thread()
        {
            while (!done)
            {
                function_wrapper task;
                if (work_queue.try_pop(task))
                    task();
                else
                    std::this_thread::yield();
            }
        }

        template<typename FunctionType>
        auto submit(FunctionType f)
        {
            typedef typename std::result_of<FunctionType()>::type result_type;

            std::packaged_task<result_type()> task(std::move(f));

            std::future<result_type > res(task.get_future());

            work_queue.push(std::move(task));
            return res;
        }
    };

    void fun()
    {
        cout << this_thread::get_id() << endl;
    }

    void test_1()
    {
        thread_pool threadPool;
        auto x = threadPool.submit(fun);
//        for (int i = 0; i < 100; ++i) {
//            threadPool.submit(fun);
//        }
        x.get();
        threadPool.worker_thread();
    }
}
*/

int main() {
    dyj26::test_1();
    // dyj25::parallel_sum(vec.begin(), vec.end());
}