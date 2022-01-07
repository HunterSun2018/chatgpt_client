#include <coroutine>

namespace co_helper
{
    template <typename T>
    struct Task
    {
        struct promise_type;
        using co_handle = std::coroutine_handle<promise_type>;
        co_handle _handle;

        struct promise_type
        {
            T value;

            Task get_return_object() { return Task{co_handle::from_promise(*this)}; }

            std::suspend_never initial_suspend() { return {}; }

            std::suspend_never final_suspend() { return {}; }

            void unhandled_exception() { std::rethrow_exception(std::current_exception()); }

            std::suspend_never return_value(T v)
            {
                value = v;
                return {};
            }
        };

        T get() { return _handle.promise().value; }

        ~Task()
        {
            if (_handle)
                _handle.destroy();
        }
    };

    template <>
    struct Task<void>
    {
        struct promise_type
        {
            Task<void> get_return_object() { return {}; }
            std::suspend_never initial_suspend() { return {}; }
            std::suspend_never final_suspend() noexcept { return {}; }
            void return_void() {}
            void unhandled_exception() { std::rethrow_exception(std::current_exception()); }
        };
    };

    template <std::movable T>
    class Generator
    {
    public:
        struct promise_type
        {
            Generator<T> get_return_object()
            {
                return Generator{Handle::from_promise(*this)};
            }
            static std::suspend_always initial_suspend() noexcept
            {
                return {};
            }
            static std::suspend_always final_suspend() noexcept
            {
                return {};
            }
            std::suspend_always yield_value(T value) noexcept
            {
                current_value = std::move(value);
                return {};
            }
            // Disallow co_await in generator coroutines.
            void await_transform() = delete;
            [[noreturn]] static void unhandled_exception()
            {
                throw;
            }

            std::optional<T> current_value;
        };

        using Handle = std::coroutine_handle<promise_type>;

        explicit Generator(const Handle coroutine) : m_coroutine{coroutine}
        {
        }

        Generator() = default;
        ~Generator()
        {
            if (m_coroutine)
            {
                m_coroutine.destroy();
            }
        }

        Generator(const Generator &) = delete;
        Generator &operator=(const Generator &) = delete;

        Generator(Generator &&other) noexcept : m_coroutine{other.m_coroutine}
        {
            other.m_coroutine = {};
        }
        Generator &operator=(Generator &&other) noexcept
        {
            if (this != &other)
            {
                if (m_coroutine)
                {
                    m_coroutine.destroy();
                }
                m_coroutine = other.m_coroutine;
                other.m_coroutine = {};
            }
            return *this;
        }

        // Range-based for loop support.
        class Iter
        {
        public:
            void operator++()
            {
                m_coroutine.resume();
            }
            const T &operator*() const
            {
                return *m_coroutine.promise().current_value;
            }
            bool operator==(std::default_sentinel_t) const
            {
                return !m_coroutine || m_coroutine.done();
            }

            explicit Iter(const Handle coroutine) : m_coroutine{coroutine}
            {
            }

        private:
            Handle m_coroutine;
        };

        Iter begin()
        {
            if (m_coroutine)
            {
                m_coroutine.resume();
            }
            return Iter{m_coroutine};
        }
        std::default_sentinel_t end()
        {
            return {};
        }

    private:
        Handle m_coroutine;
    };    
}