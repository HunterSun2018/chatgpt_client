#pragma once

#include <coroutine>
#include <future>
//
//  Task class for C++ 20 coroutine
//
template <typename T>
struct Task
{
    struct promise_type;
    using co_handle = std::coroutine_handle<promise_type>;

    co_handle _handle;
    std::future<T> _future;

    T get() { return _future.get(); }

    struct promise_type : std::promise<T>
    {
        std::coroutine_handle<> _prev = nullptr; // previous promise

        Task get_return_object() { return Task{co_handle::from_promise(*this), this->get_future()}; }

        std::suspend_never initial_suspend() { return {}; }

        std::suspend_never final_suspend() noexcept
        {
            if (_prev)
                _prev.resume();

            return {};
        }

        void unhandled_exception()
        {
            this->set_exception(std::current_exception());
        }

        void return_value(const T &v) noexcept(std::is_nothrow_copy_constructible_v<T>)
        {
            this->set_value(v);
        }

        void return_value(T &&value) noexcept(std::is_nothrow_move_constructible_v<T>)
        {
            this->set_value(std::move(value));
        }

        // auto await_transform(promise_type && p) noexcept
        // {
        //     struct awaiter : std::suspend_never
        //     {
        //         promise_type&& _p;
        //         auto await_resume() const noexcept
        //         {
        //             return _p.result();
        //         }
        //     };

        //     return awaiter{ p };
        // }
        // template <typename U>
        // U &&await_transform(U &&awaitable) noexcept
        // {
        //     return static_cast<U &&>(awaitable);
        // }
    };

    auto operator co_await()
    {
        struct Awaitor
        {
            Task &task;

            bool await_ready() const noexcept
            {
                using namespace std::chrono_literals;
                return task._future.wait_for(0s) == std::future_status::ready;
            }

            T await_resume() const
            {
                return task.get();
            }

            // template <typename PROMISE>
            void await_suspend(std::coroutine_handle<> h)
            {
                task._handle.promise()._prev = h;
            }
        };

        return Awaitor{*this};
    }
};

template <>
struct Task<void>
{
    struct promise_type;
    using co_handle = std::coroutine_handle<promise_type>;

    co_handle _handle;
    std::future<void> _future;

    struct promise_type : std::promise<void>
    {
        std::coroutine_handle<> _prev = nullptr; // previous promise

        Task<void> get_return_object()
        {
            return Task{co_handle::from_promise(*this), get_future()}; // initialize co_handle, future
        }

        std::suspend_never initial_suspend() { return {}; }

        auto final_suspend() noexcept
        {
            if (_prev)
                _prev.resume();

            return std::suspend_never();
        }

        void return_void()
        {
            this->set_value();
        }

        void unhandled_exception()
        {
            this->set_exception(std::current_exception());
        }
    };

    //
    // Chain calling
    //
    auto operator co_await()
    {
        struct Awaitor
        {
            Task &task;

            bool await_ready() const noexcept
            {
                using namespace std::chrono_literals;
                return task._future.wait_for(0s) == std::future_status::ready;
            }

            void await_resume() const
            {
                task._future.get();
            }

            void await_suspend(std::coroutine_handle<> h)
            {
                task._handle.promise()._prev = h;
            }
        };

        return Awaitor{*this};
    }

    void get()
    {
        _future.get();
    }
};