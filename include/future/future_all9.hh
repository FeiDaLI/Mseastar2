#ifndef FUTURE_ALL_HH
#define FUTURE_ALL_HH
#include "../task/task.hh"
#include <stdexcept>
#include <atomic>
#include <memory>
#include <utility>
#include <tuple>
#include <type_traits>
#include "../util/shared_ptr.hh"
#include <assert.h>
#include <cstdlib>
#include <chrono>
#include <functional>
#include <type_traits>
#include <setjmp.h>
#include <optional>
#include "do_with.hh"
#include <chrono>
#include <boost/intrusive/list.hpp>
#include <setjmp.h>
#include <ucontext.h>
#include <list>
#include "../resource/resource.hh"
#include <chrono>
#include <limits>
#include <bitset>
#include <array>
#include <atomic>
#include <list>
#include <deque>
#include <unordered_map>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <optional>
#include <iostream>
#include <time.h>
#include <signal.h>
#include <thread>
#include <iomanip>
#include <mutex>
#include <stdexcept>
#include <exception>
#include <deque>
#include <unordered_set>

#include <queue>
#include <libaio.h>
#include <sys/mman.h>
#include "../util/align.hh"
#include "../util/backtrace.hh"

#ifdef __cpp_concepts
#define GCC6_CONCEPT(x...) x
#define GCC6_NO_CONCEPT(x...)
#else
#define GCC6_CONCEPT(x...)
#define GCC6_NO_CONCEPT(x...) x
#endif

__thread bool g_need_preempt;
inline bool need_preempt() {
    return true;
    // prevent compiler from eliminating loads in a loop
    std::atomic_signal_fence(std::memory_order_seq_cst);
    return g_need_preempt;
}



using shard_id = unsigned;
using namespace std::chrono_literals;
std::ostream& operator<<(std::ostream& os, const std::chrono::steady_clock::time_point& tp) {
    auto duration = tp.time_since_epoch();
    auto hours = std::chrono::duration_cast<std::chrono::hours>(duration);
    duration -= hours;
    auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration);
    duration -= minutes;
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
    os << hours.count() << "h " << minutes.count() << "m " << seconds.count() << "s";
    return os;
}

inline int block_notifier_signal() {
    return SIGRTMIN + 1;
}

/*----------------------------------------------------------*/
struct mmap_deleter {
    size_t _size;
    void operator()(void* ptr) const;
};
/*
    mmap_area起始地址是char[],长度在mmap_deleter中保存。
    思考：为什么unique_ptr第一个参数是char[]，而不是char *，或者void *，或者char。
*/
using mmap_area = std::unique_ptr<char[], mmap_deleter>;
/*--------------------------------------------------------------*/

mmap_area mmap_anonymous(void* addr, size_t length, int prot, int flags) {
    auto ret = ::mmap(addr, length, prot, flags | MAP_ANONYMOUS, -1, 0);

    if(ret == MAP_FAILED){
        throw std::runtime_error("mmap failed");
    }
    return mmap_area(reinterpret_cast<char*>(ret), mmap_deleter{length});
}

void mmap_deleter::operator()(void* ptr) const {
    ::munmap(ptr, _size);
}

class posix_thread {
public:
    class attr;
private:
    // must allocate, since this class is moveable
    std::unique_ptr<std::function<void ()>> _func;
    pthread_t _pthread;
    bool _valid = true;
    mmap_area _stack;
private:
    static void* start_routine(void* arg) noexcept;
public:
    posix_thread(std::function<void ()> func);
    posix_thread(attr a, std::function<void ()> func);
    posix_thread(posix_thread&& x);
    ~posix_thread();
    void join();
public:
    class attr {
    public:
        struct stack_size { size_t size = 0; };
        attr() = default;
        template <typename... A>
        attr(A... a) {
            set(std::forward<A>(a)...);
        }
        void set() {}
        template <typename A, typename... Rest>
        void set(A a, Rest... rest) {
            set(std::forward<A>(a));
            set(std::forward<Rest>(rest)...);
        }
        void set(stack_size ss) { _stack_size = ss; }
    private:
        stack_size _stack_size;
        friend class posix_thread;
    };
};

#include <atomic>
#include <boost/mpl/range_c.hpp>
#include <boost/mpl/for_each.hpp>
#include "../util/align.hh"
#include "../util/spinlock.hh"

static constexpr size_t  cacheline_size = 64;
template <size_t N, int RW, int LOC>
struct prefetcher;

template<int RW, int LOC>
struct prefetcher<0, RW, LOC> {
    prefetcher(uintptr_t ptr) {}
};

template <size_t N, int RW, int LOC>
struct prefetcher {
    prefetcher(uintptr_t ptr) {
        __builtin_prefetch(reinterpret_cast<void*>(ptr), RW, LOC);
        std::atomic_signal_fence(std::memory_order_seq_cst);
        prefetcher<N-64, RW, LOC>(ptr + 64);
    }
};
template<typename T, int LOC = 3>
void prefetch(T* ptr) {
    prefetcher<align_up(sizeof(T), cacheline_size), 0, LOC>(reinterpret_cast<uintptr_t>(ptr));
}

template<typename Iterator, int LOC = 3>
void prefetch(Iterator begin, Iterator end) {
    std::for_each(begin, end, [] (auto v) { prefetch<decltype(*v), LOC>(v); });
}

template<size_t C, typename T, int LOC = 3>
void prefetch_n(T** pptr) {
    boost::mpl::for_each< boost::mpl::range_c<size_t,0,C> >( [pptr] (size_t x) { prefetch<T, LOC>(*(pptr + x)); } );
}

template<size_t L, int LOC = 3>
void prefetch(void* ptr) {
    prefetcher<L*cacheline_size, 0, LOC>(reinterpret_cast<uintptr_t>(ptr));
}

template<size_t L, typename Iterator, int LOC = 3>
void prefetch_n(Iterator begin, Iterator end) {
    std::for_each(begin, end, [] (auto v) { prefetch<L, LOC>(v); });
}

template<size_t L, size_t C, typename T, int LOC = 3>
void prefetch_n(T** pptr) {
    boost::mpl::for_each< boost::mpl::range_c<size_t,0,C> >( [pptr] (size_t x) { prefetch<L, LOC>(*(pptr + x)); } );
}

template<typename T, int LOC = 3>
void prefetchw(T* ptr) {
    prefetcher<align_up(sizeof(T), cacheline_size), 1, LOC>(reinterpret_cast<uintptr_t>(ptr));
}
template<typename Iterator, int LOC = 3>
void prefetchw_n(Iterator begin, Iterator end) {
    std::for_each(begin, end, [] (auto v) { prefetchw<decltype(*v), LOC>(v); });
}
template<size_t C, typename T, int LOC = 3>
void prefetchw_n(T** pptr) {
    boost::mpl::for_each< boost::mpl::range_c<size_t,0,C> >( [pptr] (size_t x) { prefetchw<T, LOC>(*(pptr + x)); } );
}
template<size_t L, int LOC = 3>
void prefetchw(void* ptr) {
    prefetcher<L*cacheline_size, 1, LOC>(reinterpret_cast<uintptr_t>(ptr));
}
template<size_t L, typename Iterator, int LOC = 3>
void prefetchw_n(Iterator begin, Iterator end) {
   std::for_each(begin, end, [] (auto v) { prefetchw<L, LOC>(v); });
}

template<size_t L, size_t C, typename T, int LOC = 3>
void prefetchw_n(T** pptr) {
    boost::mpl::for_each< boost::mpl::range_c<size_t,0,C> >( [pptr] (size_t x) { prefetchw<L, LOC>(*(pptr + x)); } );
}

#include <bitset>
#include <limits>

namespace bitsets {
static constexpr int ulong_bits = std::numeric_limits<unsigned long>::digits;
template<typename T>
inline size_t count_leading_zeros(T value);
template<typename T>
static inline size_t count_trailing_zeros(T value);
template<>
inline size_t count_leading_zeros<unsigned long>(unsigned long value)
{
    return __builtin_clzl(value);
}
template<>
inline size_t count_leading_zeros<long>(long value)
{
    return __builtin_clzl((unsigned long)value) - 1;
}
template<>
inline size_t count_leading_zeros<long long>(long long value)
{
    return __builtin_clzll((unsigned long long)value) - 1;
}
template<>
inline
size_t count_trailing_zeros<unsigned long>(unsigned long value)
{
    return __builtin_ctzl(value);
}

template<>
inline
size_t count_trailing_zeros<long>(long value)
{
    return __builtin_ctzl((unsigned long)value);
}
template<size_t N>
static inline size_t get_first_set(const std::bitset<N>& bitset)
{
    static_assert(N <= ulong_bits, "bitset too large");
    return count_trailing_zeros(bitset.to_ulong());
}

template<size_t N>
static inline size_t get_last_set(const std::bitset<N>& bitset)
{
    static_assert(N <= ulong_bits, "bitset too large");
    return ulong_bits - 1 - count_leading_zeros(bitset.to_ulong());
}

template<size_t N>
class set_iterator : public std::iterator<std::input_iterator_tag, int>
{
private:
    void advance()
    {
        if (_bitset.none()) {
            _index = -1;
        } else {
            auto shift = get_first_set(_bitset) + 1;
            _index += shift;
            _bitset >>= shift;
        }
    }
public:
    set_iterator(std::bitset<N> bitset, int offset = 0)
        : _bitset(bitset)
        , _index(offset - 1)
    {
        static_assert(N <= ulong_bits, "This implementation is inefficient for large bitsets");
        _bitset >>= offset;
        advance();
    }

    void operator++()
    {
        advance();
    }

    int operator*() const
    {
        return _index;
    }

    bool operator==(const set_iterator& other) const
    {
        return _index == other._index;
    }

    bool operator!=(const set_iterator& other) const
    {
        return !(*this == other);
    }
private:
    std::bitset<N> _bitset;
    int _index;
};

template<size_t N>
class set_range
{
public:
    using iterator = set_iterator<N>;
    using value_type = int;

    set_range(std::bitset<N> bitset, int offset = 0)
        : _bitset(bitset)
        , _offset(offset)
    {
    }

    iterator begin() const { return iterator(_bitset, _offset); }
    iterator end() const { return iterator(0); }
private:
    std::bitset<N> _bitset;
    int _offset;
};

template<size_t N>
static inline set_range<N> for_each_set(std::bitset<N> bitset, int offset = 0)
{
    return set_range<N>(bitset, offset);
}

}




inline
sigset_t make_empty_sigset_mask() {
    sigset_t set;
    sigemptyset(&set);
    return set;
}

inline int alarm_signal() {
    // We don't want to use SIGALRM, because the boost unit test library
    // also plays with it.
    return SIGALRM;
    return SIGRTMIN;
}

inline
sigset_t make_sigset_mask(int signo) {
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, signo);
    return set;
}

template <typename T>
inline
void throw_pthread_error(T r) {
    if (r != 0) {
        throw std::system_error(r, std::system_category());
    }
}



struct signals {
        signals();
        ~signals();
        bool poll_signal();
        bool pure_poll_signal() const;
        void handle_signal(int signo, std::function<void ()>&& handler);
        void handle_signal_once(int signo, std::function<void ()>&& handler);
        static void action(int signo, siginfo_t* siginfo, void* ignore);
        struct signal_handler {
            signal_handler(int signo, std::function<void ()>&& handler);
            std::function<void ()> _handler;
        };
        std::atomic<uint64_t> _pending_signals;
        std::unordered_map<int, signal_handler> _signal_handlers;
        
};









class reactor;
reactor& engine();
template <typename Clock> class timer;
using steady_clock_type = std::chrono::steady_clock;


void schedule_normal(std::unique_ptr<task> t);
void schedule_urgent(std::unique_ptr<task> t);


class manual_clock {
    public:
        using rep = int64_t;
        using period = std::chrono::nanoseconds::period;
        using duration = std::chrono::duration<rep, period>;
        using time_point = std::chrono::time_point<manual_clock, duration>;
    private:
        static std::atomic<rep> _now;
    public:
        manual_clock();
        static time_point now() {
            return time_point(duration(_now.load(std::memory_order_relaxed)));
        }
        static void advance(duration d);
        static void expire_timers();
};
/*---------------------------------------------------posix相关-------------------------------------------------------*/
namespace posix {

template <typename Rep, typename Period>
struct timespec to_timespec(std::chrono::duration<Rep, Period> d) {
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(d).count();
    struct timespec ts {};
    ts.tv_sec = ns / 1000000000;
    ts.tv_nsec = ns % 1000000000;
    return ts;
}

template <typename Rep1, typename Period1, typename Rep2, typename Period2>
struct itimerspec
to_relative_itimerspec(std::chrono::duration<Rep1, Period1> base, std::chrono::duration<Rep2, Period2> interval) {
    struct itimerspec its {};
    its.it_interval = to_timespec(interval);
    its.it_value = to_timespec(base);
    return its;
}

template <typename Clock, class Duration, class Rep, class Period>
struct itimerspec
to_absolute_itimerspec(std::chrono::time_point<Clock, Duration> base, std::chrono::duration<Rep, Period> interval) {
    return to_relative_itimerspec(base.time_since_epoch(), interval);
}

}




// Utility function for error handling
inline void throw_system_error_on(bool condition) {
    if (condition) {
        throw std::system_error(errno, std::system_category());
    }
}


// timer 有默认参数,所以timer<>表示一个steady_clock_type
template <typename Clock = steady_clock_type>
class timer {
public:
    timer() = default;
    using time_point = typename Clock::time_point;
    using duration = typename Clock::duration;
    typedef Clock clock;
    using callback_t = std::function<void()>;
    using iterator = typename std::list<timer*>::iterator;
    iterator it; // 新增的迭代器成员
    iterator expired_it;  // 过期链表中的位置
    // boost::intrusive::list_member_hook<> _link;
    callback_t _callback;
    time_point _expiry; //到期时间点,每个timer都有一个到期时间点.
    std::optional<duration> _period;
    bool _armed = false;
    bool _queued = false;
    bool _expired = false;
    void readd_periodic();
    void arm_state(time_point until, std::optional<duration> period);
    timer(timer&& t) noexcept;
    explicit timer(callback_t&& callback);
    ~timer();
    // future<> expired();
    void set_callback(callback_t&& callback);
    void arm(time_point until, std::optional<duration> period = {});
    void rearm(time_point until, std::optional<duration> period = {});
    void rearm(duration delta) { rearm(Clock::now() + delta); }
    void arm(duration delta);
    void arm_periodic(duration delta);
    bool armed() const { return _armed; }
    bool cancel();
    time_point get_timeout();
};


class lowres_clock {
public:
    typedef int64_t rep;
    // The lowres_clock's resolution is 10ms. However, to make it is easier to
    // do calcuations with std::chrono::milliseconds, we make the clock's
    // period to 1ms instead of 10ms.
    typedef std::ratio<1, 1000> period;
    typedef std::chrono::duration<rep, period> duration;
    typedef std::chrono::time_point<lowres_clock, duration> time_point;
    lowres_clock();
    // ~lowres_clock();
    static time_point now() {
        auto nr = _now.load(std::memory_order_relaxed);
        return time_point(duration(nr));
    }
private:
    static void update();
    // _now is updated by cpu0 and read by other cpus. Make _now on its own
    // cache line to avoid false sharing.
    static std::atomic<rep> _now [[gnu::aligned(64)]];
    // High resolution timer to drive this low resolution clock
    struct timer_deleter {
        void operator()(void*) const;
    };
    timer<steady_clock_type> _timer; //?
    // High resolution timer expires every 10 milliseconds
    static constexpr std::chrono::milliseconds _granularity{10};
};

std::atomic<lowres_clock::rep> lowres_clock::_now;
std::atomic<manual_clock::rep> manual_clock::_now;
constexpr std::chrono::milliseconds lowres_clock::_granularity;


// Forward declarations for timer-related functions
void enable_timer(steady_clock_type::time_point when);
// Function declarations
bool queue_timer(timer<steady_clock_type>* tmr);
void add_timer(timer<steady_clock_type>* tmr);
void del_timer(timer<steady_clock_type>* tmr);
bool queue_timer(timer<lowres_clock>* tmr);
void add_timer(timer<lowres_clock>* tmr);
void del_timer(timer<lowres_clock>* tmr);
bool queue_timer(timer<manual_clock>* tmr);
void add_timer(timer<manual_clock>* tmr);
void del_timer(timer<manual_clock>* tmr);

template<typename Timer>
class timer_set {
public:
    using time_point = typename Timer::time_point;
    using timer_list_t = std::list<Timer*>;
    using duration = typename Timer::duration;
    using timestamp_t = typename duration::rep;
    static constexpr timestamp_t max_timestamp = std::numeric_limits<timestamp_t>::max();
    static constexpr int timestamp_bits = std::numeric_limits<timestamp_t>::digits;//63
    static constexpr int n_buckets = timestamp_bits + 1;//64
    std::array<timer_list_t, n_buckets> _buckets;
    timestamp_t _last;
    timestamp_t _next; 
    std::bitset<n_buckets> _non_empty_buckets;
    /// \brief 获取时间点对应的时间戳（计数）
    /// \param tp 时间点对象
    /// \return 时间点自纪元以来的计数值
    static timestamp_t get_timestamp(time_point tp) {
        return tp.time_since_epoch().count();
    }
    /// \brief 获取定时器的超时时间戳
    /// \param timer 定时器对象
    /// \return 定时器超时时间的时间戳
    static timestamp_t get_timestamp(Timer& timer) {
        return get_timestamp(timer.get_timeout());
    }
    /// \brief 根据时间戳计算对应的桶索引
    /// \param timestamp 要计算的定时器时间戳
    /// \return 对应的桶索引
    int get_index(timestamp_t timestamp) const {
        if (timestamp <= _last) {
            return n_buckets - 1;
        }
        auto index = bitsets::count_leading_zeros(timestamp ^ _last);
        assert(index < n_buckets - 1);
        return index;
    }
    /*
    这个需要手动推导一下
    */

    /// \brief 获取定时器对应的桶索引
    /// \param timer 定时器对象
    /// \return 对应的桶索引
    int get_index(Timer& timer) const {
        return get_index(get_timestamp(timer));
    }
    /*
    一个timer有唯一的一个过期时间。
    */

    /// \brief 获取最后一个非空桶的索引
    /// \return 最后一个非空桶的索引
    int get_last_non_empty_bucket() const {
        return bitsets::get_last_set(_non_empty_buckets);
    }

public:
    /// \brief 构造函数初始化成员变量
    timer_set() : _last(0), _next(max_timestamp), _non_empty_buckets(0) {}

    ~timer_set() {
        // 清理所有定时器资源
        for (auto& list : _buckets) {
            while (!list.empty()) {
                auto* timer = list.front();
                list.pop_front();
                timer->cancel();
            }
        }
    }

    /// \brief 将定时器插入到对应的桶中
    /// \param timer 要插入的定时器对象
    /// \return true 如果插入后_next被更新为更小的值，否则false
    bool insert(Timer& timer) {
        auto timestamp = get_timestamp(timer);
        auto index = get_index(timestamp);
        auto& list = _buckets[index];
        list.push_back(&timer);
        timer.it = --list.end();//timer.it是timer在list中的迭代器(使用尾插法,所以end前一个位置就是最后一个元素前开后闭)
        _non_empty_buckets[index] = true;
        if (timestamp < _next) {
            _next = timestamp;
            return true;
        }
        return false;
    }
    /*
        next是边插入边维护的一个变量.表示下一次过期的时间.
    */

    /// \brief 从集合中移除定时器
    /// \param timer 要移除的定时器对象
    void remove(Timer& timer) {
        auto index = get_index(timer);
        auto& list = _buckets[index];
        list.erase(timer.it);//erase一个节点会造成内存泄漏吗? 不会:见 STL源码, 解析
        if (list.empty()) {
            _non_empty_buckets[index] = false;
        }
    }
    /** 
     * 
     * 这个地方像是RTOS优先级位图
     * 
    */
    /// \brief 获取已到期的定时器列表
    /// \param now 当前时间点
    /// \return 包含所有已到期定时器的列表
    timer_list_t expire(time_point now) {
        timer_list_t exp;
        auto timestamp = get_timestamp(now);
        if (timestamp < _last) {
            abort();
        }
        //当前时间一定>=_last
        auto index = get_index(timestamp);
        // 处理所有在当前时间之前的非空桶
        for (int i : bitsets::for_each_set(_non_empty_buckets, index + 1)) {
            exp.splice(exp.end(), _buckets[i]);
            _non_empty_buckets[i] = false;
        }
        /*
            把所有过期的链表添加到exp后面(exp是一个临时的链表，操作时间间复杂度O(1)).
        */
        _last = timestamp;//所以_last就是最后一次处理过期定时器的时间.
        _next = max_timestamp;//_next设置为无穷.
        auto& list = _buckets[index];
        // 处理当前索引的桶中的定时器
        while (!list.empty()) {
            auto* timer = list.front();
            list.pop_front();
            if (timer->get_timeout() <= now) {
                exp.push_back(timer);
            } else {
                insert(*timer);
            }
        }
        _non_empty_buckets[index] = !list.empty();
        if (_next == max_timestamp && _non_empty_buckets.any()) {
            // 更新_next为最后一个非空桶中的最小时间戳
            for (auto* timer : _buckets[get_last_non_empty_bucket()]) {
                _next = std::min(_next, get_timestamp(*timer));
            }
        }
        return exp;//返回这个链表
    }

    time_point get_next_timeout() const {
        return time_point(duration(std::max(_last, _next)));
    }
    void clear() {
        for (auto& list : _buckets) {
            list.clear();
        }
        _non_empty_buckets.reset();
    }
    size_t size() const {
        size_t res = 0;
        for (const auto& list : _buckets) {
            res += list.size();
        }
        return res;
    }
    bool empty() const {
        return _non_empty_buckets.none();
    }
    time_point now() {
        return Timer::clock::now();
    }
};


template<typename T>
struct function_traits;

template<typename Ret, typename... Args>
struct function_traits<Ret(Args...)>
{
    using return_type = Ret;
    using args_as_tuple = std::tuple<Args...>;
    using signature = Ret (Args...);
 
    static constexpr std::size_t arity = sizeof...(Args);
 
    template <std::size_t N>
    struct arg
    {
        static_assert(N < arity, "no such parameter index.");
        using type = typename std::tuple_element<N, std::tuple<Args...>>::type;
    };
};

template<typename Ret, typename... Args>
struct function_traits<Ret(*)(Args...)> : public function_traits<Ret(Args...)>
{};

template <typename T, typename Ret, typename... Args>
struct function_traits<Ret(T::*)(Args...)> : public function_traits<Ret(Args...)>
{};

template <typename T, typename Ret, typename... Args>
struct function_traits<Ret(T::*)(Args...) const> : public function_traits<Ret(Args...)>
{};

template <typename T>
struct function_traits : public function_traits<decltype(&T::operator())>
{};

template<typename T>
struct function_traits<T&> : public function_traits<std::remove_reference_t<T>>
{};
template <typename... T> class future;
template <typename... T> class promise;
template <typename... T> struct future_state;


/// \cond internal
template <typename... T>
struct future_state {
    static constexpr bool copy_noexcept = std::is_nothrow_copy_constructible<std::tuple<T...>>::value;
    static_assert(std::is_nothrow_move_constructible<std::tuple<T...>>::value,
                  "Types must be no-throw move constructible");
    static_assert(std::is_nothrow_destructible<std::tuple<T...>>::value,
                  "Types must be no-throw destructible");
    static_assert(std::is_nothrow_copy_constructible<std::exception_ptr>::value,
                  "std::exception_ptr's copy constructor must not throw");
    static_assert(std::is_nothrow_move_constructible<std::exception_ptr>::value,
                  "std::exception_ptr's move constructor must not throw");
    enum class state {
         invalid,
         future,
         result,
         exception,
    } _state = state::future;
    union any {
        any() {}
        ~any() {}
        std::tuple<T...> value;
        std::exception_ptr ex;
    } _u;
    future_state() noexcept {}
    [[gnu::always_inline]]
    future_state(future_state&& x) noexcept
            : _state(x._state) {
        switch (_state) {
        case state::future:
            break;
        case state::result:
            new (&_u.value) std::tuple<T...>(std::move(x._u.value));
            x._u.value.~tuple();
            break;
        case state::exception:
            new (&_u.ex) std::exception_ptr(std::move(x._u.ex));
            x._u.ex.~exception_ptr();
            break;
        case state::invalid:
            break;
        default:
            abort();
        }
        x._state = state::invalid;
    }
    __attribute__((always_inline))
    ~future_state() noexcept {
        switch (_state) {
        case state::invalid:
            break;
        case state::future:
            break;
        case state::result:
            _u.value.~tuple();
            break;
        case state::exception:
            _u.ex.~exception_ptr();
            break;
        default:
            abort();
        }
    }
    future_state& operator=(future_state&& x) noexcept {
        if (this != &x) {
            this->~future_state();
            new (this) future_state(std::move(x));
        }
        return *this;
    }
    bool available() const noexcept { return _state == state::result || _state == state::exception; }
    bool failed() const noexcept { return _state == state::exception; }
    void wait();
    void set(const std::tuple<T...>& value) noexcept {
        assert(_state == state::future);
        new (&_u.value) std::tuple<T...>(value);
        _state = state::result;
    }
    void set(std::tuple<T...>&& value) noexcept {
        assert(_state == state::future);
        new (&_u.value) std::tuple<T...>(std::move(value));
        _state = state::result;
    }
    template <typename... A>
    void set(A&&... a) {
        assert(_state == state::future);
        new (&_u.value) std::tuple<T...>(std::forward<A>(a)...);
        _state = state::result;
    }
    void set_exception(std::exception_ptr ex) noexcept {
        assert(_state == state::future);
        new (&_u.ex) std::exception_ptr(ex);
        _state = state::exception;
    }
    std::exception_ptr get_exception() && noexcept {
        assert(_state == state::exception);
        // Move ex out so future::~future() knows we've handled it
        _state = state::invalid;
        auto ex = std::move(_u.ex);
        _u.ex.~exception_ptr();
        return ex;
    }
    std::exception_ptr get_exception() const& noexcept {
        assert(_state == state::exception);
        return _u.ex;
    }
    std::tuple<T...> get_value() && noexcept {
        assert(_state == state::result);
        return std::move(_u.value);
    }
    template<typename U = std::tuple<T...>>
    std::enable_if_t<std::is_copy_constructible<U>::value, U> get_value() const& noexcept(copy_noexcept) {
        assert(_state == state::result);
        return _u.value;
    }
    std::tuple<T...> get() && {
        assert(_state != state::future);
        if (_state == state::exception) {
            _state = state::invalid;
            auto ex = std::move(_u.ex);
            _u.ex.~exception_ptr();
            // Move ex out so future::~future() knows we've handled it
            std::rethrow_exception(std::move(ex));
        }
        return std::move(_u.value);
    }
    std::tuple<T...> get() const& {
        assert(_state != state::future);
        if (_state == state::exception) {
            std::rethrow_exception(_u.ex);
        }
        return _u.value;
    }
    void ignore() noexcept {
        assert(_state != state::future);
        this->~future_state();
        _state = state::invalid;
    }
    using get0_return_type = std::tuple_element_t<0, std::tuple<T...>>;
    static get0_return_type get0(std::tuple<T...>&& x) {
        return std::get<0>(std::move(x));
    }
    void forward_to(promise<T...>& pr) noexcept;
        // assert(_state != state::future);
        // if (_state == state::exception) {
        //     pr.set_urgent_exception(std::move(_u.ex));
        //     _u.ex.~exception_ptr();
        // } else {
        //     pr.set_urgent_value(std::move(_u.value));
        //     _u.value.~tuple();
        // }
        // _state = state::invalid;
    //}
};

template <>
struct future_state<> {
    static_assert(sizeof(std::exception_ptr) == sizeof(void*), "exception_ptr not a pointer");
    static_assert(std::is_nothrow_copy_constructible<std::exception_ptr>::value,
                  "std::exception_ptr's copy constructor must not throw");
    static_assert(std::is_nothrow_move_constructible<std::exception_ptr>::value,
                  "std::exception_ptr's move constructor must not throw");
    static constexpr bool copy_noexcept = true;
    enum class state : uintptr_t {
         invalid = 0,
         future = 1,
         result = 2,
         exception_min = 3,  // or anything greater
    };
    union any {
        any() { st = state::future; }
        ~any() {}
        state st;
        std::exception_ptr ex;
    } _u;
    future_state() noexcept {}
    [[gnu::always_inline]]
    future_state(future_state&& x) noexcept {
        if (x._u.st < state::exception_min) {
            _u.st = x._u.st;
        } else {
            // Move ex out so future::~future() knows we've handled it
            // Moving it will reset us to invalid state
            new (&_u.ex) std::exception_ptr(std::move(x._u.ex));
            x._u.ex.~exception_ptr();
        }
        x._u.st = state::invalid;
    }
    [[gnu::always_inline]]
    ~future_state() noexcept {
        if (_u.st >= state::exception_min) {
            _u.ex.~exception_ptr();
        }
    }
    future_state& operator=(future_state&& x) noexcept {
        if (this != &x) {
            this->~future_state();
            new (this) future_state(std::move(x));
        }
        return *this;
    }
    bool available() const noexcept { return _u.st == state::result || _u.st >= state::exception_min; }
    bool failed() const noexcept { return _u.st >= state::exception_min; }
    void set(const std::tuple<>& value) noexcept {
        assert(_u.st == state::future);
        _u.st = state::result;
    }
    void set(std::tuple<>&& value) noexcept {
        assert(_u.st == state::future);
        _u.st = state::result;
    }
    void set() {
        assert(_u.st == state::future);
        _u.st = state::result;//
    }
    void set_exception(std::exception_ptr ex) noexcept {
        assert(_u.st == state::future);
        new (&_u.ex) std::exception_ptr(ex);
        assert(_u.st >= state::exception_min);
    }
    std::tuple<> get() && {
        assert(_u.st != state::future);
        if (_u.st >= state::exception_min) {
            // Move ex out so future::~future() knows we've handled it
            // Moving it will reset us to invalid state
            std::rethrow_exception(std::move(_u.ex));
        }
        return {};
    }
    std::tuple<> get() const& {
        assert(_u.st != state::future);
        if (_u.st >= state::exception_min) {
            std::rethrow_exception(_u.ex);
        }
        return {};
    }
    void ignore() noexcept {
        assert(_u.st != state::future);
        this->~future_state();
        _u.st = state::invalid;
    }
    using get0_return_type = void;
    static get0_return_type get0(std::tuple<>&&) {
        return;
    }
    std::exception_ptr get_exception() && noexcept {
        assert(_u.st >= state::exception_min);
        // Move ex out so future::~future() knows we've handled it
        // Moving it will reset us to invalid state
        return std::move(_u.ex);
    }
    std::exception_ptr get_exception() const& noexcept {
        assert(_u.st >= state::exception_min);
        return _u.ex;
    }
    std::tuple<> get_value() const noexcept {
        assert(_u.st == state::result);
        return {};
    }
    void forward_to(promise<>& pr) noexcept;
    //     assert(_u.st != state::future && _u.st != state::invalid);
    //     if (_u.st >= state::exception_min) {
    //         pr.set_urgent_exception(std::move(_u.ex));
    //         _u.ex.~exception_ptr();
    //     }else{
    //         pr.set_urgent_value(std::tuple<>());
    //     }
    //     _u.st = state::invalid;
    // }
};

template <typename Func, typename... T>
struct continuation final : task {
    continuation(Func&& func, future_state<T...>&& state) : _state(std::move(state)), _func(std::move(func)) {}
    continuation(Func&& func) : _func(std::move(func)) {}
    virtual void run() noexcept override {
        _func(std::move(_state));
    }
    future_state<T...> _state;
    Func _func;
};

template <typename... T>
class future;
template <typename... T> struct is_future : std::false_type {};
template <typename... T> struct is_future<future<T...>> : std::true_type {};
struct ready_future_marker {};
struct ready_future_from_tuple_marker {};
struct exception_future_marker {};
template <typename T>
struct futurize;

template <typename T>
using futurize_t = typename futurize<T>::type;


template <typename T>
struct futurize {
    /// If \c T is a future, \c T; otherwise \c future<T>
    using type = future<T>;
    /// The promise type associated with \c type.
    using promise_type = promise<T>;
    /// The value tuple type associated with \c type
    using value_type = std::tuple<T>;

    /// Apply a function to an argument list (expressed as a tuple)
    /// and return the result, as a future (if it wasn't already).
    template<typename Func, typename... FuncArgs>
    static inline type apply(Func&& func, std::tuple<FuncArgs...>&& args) noexcept;

    /// Apply a function to an argument list
    /// and return the result, as a future (if it wasn't already).
    template<typename Func, typename... FuncArgs>
    static inline type apply(Func&& func, FuncArgs&&... args) noexcept;
    /// Convert a value or a future to a future
    static inline type convert(T&& value) {  
        return make_ready_future<T>(std::move(value)); 
    }
    // 如果convert传入的是值, 使用 make_ready_future 转为future类型

    static inline type convert(type&& value){ 
        return std::move(value); 
    }
    // 如果convert传入的是future，直接把future使用std::move变为右值.

    /// Convert the tuple representation into a future
    static type from_tuple(value_type&& value);
    /// Convert the tuple representation into a future
    static type from_tuple(const value_type& value);

    /// Makes an exceptional future of type \ref type.
    template <typename Arg>
    static type make_exception_future(Arg&& arg);
};

/// \cond internal
template <>
struct futurize<void> {
    using type = future<>;
    using promise_type = promise<>;
    using value_type = std::tuple<>;

    template<typename Func, typename... FuncArgs>
    static inline type apply(Func&& func, std::tuple<FuncArgs...>&& args) noexcept;

    template<typename Func, typename... FuncArgs>
    static inline type apply(Func&& func, FuncArgs&&... args) noexcept;

    static inline type from_tuple(value_type&& value);
    static inline type from_tuple(const value_type& value);

    template <typename Arg>
    static type make_exception_future(Arg&& arg);
};

template <typename... Args>
struct futurize<future<Args...>> {
    using type = future<Args...>;
    using promise_type = promise<Args...>;

    template<typename Func, typename... FuncArgs>
    static inline type apply(Func&& func, std::tuple<FuncArgs...>&& args) noexcept;

    template<typename Func, typename... FuncArgs>
    static inline type apply(Func&& func, FuncArgs&&... args) noexcept;

    static inline type convert(Args&&... values) { return make_ready_future<Args...>(std::move(values)...); }
    static inline type convert(type&& value) { return std::move(value); }

    template <typename Arg>
    static type make_exception_future(Arg&& arg);
};


GCC6_CONCEPT(
template <typename T>
concept Future = is_future<T>::value;

template <typename Func, typename... T>
concept CanApply = requires (Func f, T... args) {
    f(std::forward<T>(args)...);
};

template <typename Func, typename Return, typename... T>
concept ApplyReturns = requires (Func f, T... args) {
    { f(std::forward<T>(args)...) } -> std::convertible_to<Return>;
};

template <typename Func, typename... T>
concept ApplyReturnsAnyFuture = requires (Func f, T... args) {
    requires is_future<decltype(f(std::forward<T>(args)...))>::value;
};
)

void engine_exit(std::exception_ptr eptr = {});
void report_failed_future(std::exception_ptr ex);

template <typename... T>
class future {
    promise<T...>* _promise;
    future_state<T...> _local_state;  // valid if !_promise
    static constexpr bool copy_noexcept = future_state<T...>::copy_noexcept;
private:
    future(promise<T...>* pr) noexcept : _promise(pr) {
        _promise->_future = this;
    }
    template <typename... A>
    future(ready_future_marker, A&&... a) : _promise(nullptr) {
        _local_state.set(std::forward<A>(a)...);
    }
    template <typename... A>
    future(ready_future_from_tuple_marker, std::tuple<A...>&& data) : _promise(nullptr) {
        _local_state.set(std::move(data));
    }
    future(exception_future_marker, std::exception_ptr ex) noexcept : _promise(nullptr) {
        _local_state.set_exception(std::move(ex));
    }
    [[gnu::always_inline]]
    explicit future(future_state<T...>&& state) noexcept
            : _promise(nullptr), _local_state(std::move(state)) {
    }
    [[gnu::always_inline]]
    future_state<T...>* state() noexcept {
        return _promise ? _promise->_state : &_local_state;
    }

    template <typename Func>
    void schedule(Func&& func) {
        if (state()->available()) {
            std::cout<<"schedule state available."<<std::endl;
            ::schedule_normal(std::make_unique<continuation<Func, T...>>(std::move(func), std::move(*state())));
        } else {
            //走这一条
            assert(_promise);
            std::cout<<"schedule state unavailable."<<std::endl;
            _promise->schedule(std::move(func));
            _promise->_future = nullptr;
            _promise = nullptr;
        }
    }

    [[gnu::always_inline]]
    future_state<T...> get_available_state() noexcept {
        auto st = state();
        if (_promise) {
            _promise->_future = nullptr;
            _promise = nullptr;
        }
        return std::move(*st);
    }

    [[gnu::noinline]]
    future<T...> rethrow_with_nested() {
        if (!failed()) {
            return make_exception_future<T...>(std::current_exception());
        } else {
            std::nested_exception f_ex;
            try {
                get();
            } catch (...) {
                std::throw_with_nested(f_ex);
            }
        }
        assert(0 && "we should not be here");
    }

    template<typename... U>
    friend class shared_future;
public:
    /// \brief The data type carried by the future.
    using value_type = std::tuple<T...>;
    /// \brief The data type carried by the future.
    using promise_type = promise<T...>;
    /// \brief Moves the future into a new object.
    [[gnu::always_inline]]
    future(future&& x) noexcept : _promise(x._promise) {
        if (!_promise) {
            _local_state = std::move(x._local_state);
        }
        x._promise = nullptr;
        if (_promise) {
            _promise->_future = this;
        }
    }
    future(const future&) = delete;
    future& operator=(future&& x) noexcept {
        if (this != &x) {
            this->~future();
            new (this) future(std::move(x));
        }
        return *this;
    }
    void operator=(const future&) = delete;
    __attribute__((always_inline))
    ~future() {
        if (_promise) {
            _promise->_future = nullptr;
        }
        if (failed()) {
            report_failed_future(state()->get_exception());
        }
    }
    std::tuple<T...> get();

     std::exception_ptr get_exception() {
        return get_available_state().get_exception();
    }
    typename future_state<T...>::get0_return_type get0() {
        return future_state<T...>::get0(get());
    }

    /// \cond internal
    void wait();
    [[gnu::always_inline]]
    bool available() noexcept {
        return state()->available();
    }
    [[gnu::always_inline]]
    bool failed() noexcept {
        return state()->failed();
    }
    template <typename Func, typename Result = futurize_t<std::result_of_t<Func(T&&...)>>>
    GCC6_CONCEPT( requires CanApply<Func, T...> )
    Result
    then(Func&& func) noexcept {
        using futurator = futurize<std::result_of_t<Func(T&&...)>>;
        // 如果当前 future,已经完成且不需要抢占.
        if (available() && !need_preempt()) {
            //调试的时候这里不会执行到,need_preempt永远为true.
            if (failed()) {
                // 如果失败，传播异常
                return futurator::make_exception_future(get_available_state().get_exception());
            } else {
                // 如果成功，执行回调函数
                return futurator::apply(std::forward<Func>(func), get_available_state().get_value());
            }
        }
        // 如果 future 还未完成，创建新的 promise 和 future
        typename futurator::promise_type pr; // 这行代码是什么意思?
        auto fut = pr.get_future();
        try {
            std::cout<<"开始执行schedule"<<std::endl;
            //schedule接受一个lambda函数,捕捉pr和func,参数为state
            schedule([pr = std::move(pr), func = std::forward<Func>(func)] (auto&& state) mutable
            {
                //这个地方看不懂.auto &&state和state()有什么区别？为什么state不用引用捕获？
                if (state.failed()) {
                    pr.set_exception(std::move(state).get_exception());
                }
                else{
                    // 执行这个
                    futurator::apply(std::forward<Func>(func), std::move(state).get_value()).forward_to(std::move(pr));
                    // futuator::apply首先执行func(value)，返回类型是T.
                    // 返回一个 future<T>. 然后调用future的forward_to.
                }
            });
        } catch (...) {
            abort();
        }
        return fut;
    }

    template <typename Func, typename Result = futurize_t<std::result_of_t<Func(future)>>>
    GCC6_CONCEPT( requires CanApply<Func, future> )
    Result
    then_wrapped(Func&& func) noexcept {
        using futurator = futurize<std::result_of_t<Func(future)>>;
        if (available() && !need_preempt()) {
            return futurator::apply(std::forward<Func>(func), future(get_available_state()));
        }
        typename futurator::promise_type pr;
        auto fut = pr.get_future();
        try {
            schedule([pr = std::move(pr), func = std::forward<Func>(func)] (auto&& state) mutable {
                futurator::apply(std::forward<Func>(func), future(std::move(state))).forward_to(std::move(pr));
            });
        } catch (...) {
            abort();
        }
        return fut;
    }
    void forward_to(promise<T...>&& pr) noexcept {
        if (state()->available()) {
            std::cout<<"state available future调用forward_to"<<std::endl;
            state()->forward_to(pr);
            
        } else {
            std::cout<<"state unavailable future调用forward_to"<<std::endl;
            _promise->_future = nullptr;
            *_promise = std::move(pr);
            _promise = nullptr;
        }
    }

    template <typename Func>
    GCC6_CONCEPT( requires CanApply<Func> )
    future<T...> finally(Func&& func) noexcept {
        return then_wrapped(finally_body<Func, is_future<std::result_of_t<Func()>>::value>(std::forward<Func>(func)));
    }


    template <typename Func, bool FuncReturnsFuture>
    struct finally_body;

    template <typename Func>
    struct finally_body<Func, true> {
        Func _func;

        finally_body(Func&& func) : _func(std::forward<Func>(func))
        { }

        future<T...> operator()(future<T...>&& result) {
            using futurator = futurize<std::result_of_t<Func()>>;
            return futurator::apply(_func).then_wrapped([result = std::move(result)](auto f_res) mutable {
                if (!f_res.failed()) {
                    return std::move(result);
                } else {
                    try {
                        f_res.get();
                    } catch (...) {
                        return result.rethrow_with_nested();
                    }
                    assert(0 && "we should not be here");
                }
            });
        }
    };

    template <typename Func>
    struct finally_body<Func, false> {
        Func _func;
        finally_body(Func&& func) : _func(std::forward<Func>(func))
        {}
        future<T...> operator()(future<T...>&& result) {
            try {
                _func();
                return std::move(result);
            } catch (...) {
                return result.rethrow_with_nested();
            }
        };
    };

    future<> or_terminate() noexcept {
        return then_wrapped([] (auto&& f) {
            try {
                f.get();
            } catch (...) {
                engine_exit(std::current_exception());
            }
        });
    }

    future<> discard_result() noexcept {
        return then([] (T&&...) {});
    }
    template <typename Func>
    future<T...> handle_exception(Func&& func) noexcept {
        using func_ret = std::result_of_t<Func(std::exception_ptr)>;
        return then_wrapped([func = std::forward<Func>(func)]
                             (auto&& fut) -> future<T...> {
            if (!fut.failed()) {
                return make_ready_future<T...>(fut.get());
            } else {
                return futurize<func_ret>::apply(func, fut.get_exception());
            }
        });
    }
    template <typename Func>
    future<T...> handle_exception_type(Func&& func) noexcept {
        using trait = function_traits<Func>;
        static_assert(trait::arity == 1, "func can take only one parameter");
        using ex_type = typename trait::template arg<0>::type;
        using func_ret = typename trait::return_type;
        return then_wrapped([func = std::forward<Func>(func)]
                             (auto&& fut) -> future<T...> {
            try {
                return make_ready_future<T...>(fut.get());
            } catch(ex_type& ex) {
                return futurize<func_ret>::apply(func, ex);
            }
        });
    }
    void ignore_ready_future() noexcept {
        state()->ignore();
    }
    /// \cond internal
    template <typename... U>
    friend class promise;
    template <typename... U, typename... A>
    friend future<U...> make_ready_future(A&&... value);
    template <typename... U>
    friend future<U...> make_exception_future(std::exception_ptr ex) noexcept;
    template <typename... U, typename Exception>
    friend future<U...> make_exception_future(Exception&& ex) noexcept;
    /// \endcond
};







template <typename... T>
class promise {
public:
    enum class urgent { no, yes };
    future<T...>* _future = nullptr;
    future_state<T...> _local_state;
    future_state<T...>* _state;
    std::unique_ptr<task> _task;
    static constexpr bool copy_noexcept = future_state<T...>::copy_noexcept;
    /// \brief Constructs an empty \c promise.
    ///
    /// Creates promise with no associated future yet (see get_future()).
    promise() noexcept : _state(&_local_state) {}

    /// \brief Moves a \c promise object.
    promise(promise&& x) noexcept : _future(x._future), _state(x._state), _task(std::move(x._task)) {
        if (_state == &x._local_state) {
            _state = &_local_state;
            _local_state = std::move(x._local_state);
        }
        x._future = nullptr;
        x._state = nullptr;
        migrated();
    }
    promise(const promise&) = delete;
    __attribute__((always_inline))
    ~promise() noexcept {
        abandoned();
    }
    promise& operator=(promise&& x) noexcept {
        if (this != &x) {
            this->~promise();
            new (this) promise(std::move(x));
        }
        return *this;
    }
    void operator=(const promise&) = delete;

    future<T...> get_future() noexcept;

    void set_value(const std::tuple<T...>& result) noexcept(copy_noexcept) {
        do_set_value<urgent::no>(result);
    }

    void set_value(std::tuple<T...>&& result) noexcept {
        do_set_value<urgent::no>(std::move(result));
    }

    template <typename... A>
    void set_value(A&&... a) noexcept {
        assert(_state);
        _state->set(std::forward<A>(a)...);
        make_ready<urgent::no>();
    }

    void set_exception(std::exception_ptr ex) noexcept {
        do_set_exception<urgent::no>(std::move(ex));
    }

    template<typename Exception>
    void set_exception(Exception&& e) noexcept {
        set_exception(make_exception_ptr(std::forward<Exception>(e)));
    }
    template<urgent Urgent>
    void do_set_value(std::tuple<T...> result) noexcept {
        assert(_state);
        _state->set(std::move(result));
        make_ready<Urgent>();
    }

    void set_urgent_value(const std::tuple<T...>& result) noexcept(copy_noexcept) {
        do_set_value<urgent::yes>(result);
    }

    void set_urgent_value(std::tuple<T...>&& result) noexcept {
        do_set_value<urgent::yes>(std::move(result));
    }

    template<urgent Urgent>
    void do_set_exception(std::exception_ptr ex) noexcept {
        assert(_state);
        _state->set_exception(std::move(ex));
        make_ready<Urgent>();
    }

    void set_urgent_exception(std::exception_ptr ex) noexcept {
        do_set_exception<urgent::yes>(std::move(ex));
    }
    template <typename Func>
    void schedule(Func&& func) {
        auto tws = std::make_unique<continuation<Func, T...>>(std::move(func));
        _state = &tws->_state;
        _task = std::move(tws); 
    }
    template<urgent Urgent>
    void make_ready() noexcept;
    void migrated() noexcept;
    void abandoned() noexcept;
    template <typename... U>
    friend class future;
    friend class future_state<T...>;
};

template<>
class promise<void> : public promise<> {};






#include <iosfwd>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <system_error>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <cstring>
#include <utility>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/mman.h>
#include <signal.h>
#include <boost/optional.hpp>
#include <pthread.h>
#include <signal.h>
#include <memory>
#include <chrono>
#include <string>
#include <sys/uio.h>
#include "../util/unaligned.hh"


void throw_system_error_on(bool condition, const char* what_arg = "!") {
    if (condition) {
        throw std::system_error(errno, std::system_category(), what_arg);
    }
}


#include <arpa/inet.h>
#include <iosfwd>
#include <utility>
inline uint64_t ntohq(uint64_t v) {
    return __builtin_bswap64(v);
}
inline uint64_t htonq(uint64_t v) {
    return __builtin_bswap64(v);
}

namespace net {
inline void ntoh() {}
inline void hton() {}
inline uint8_t ntoh(uint8_t x) { return x; }
inline uint8_t hton(uint8_t x) { return x; }
inline uint16_t ntoh(uint16_t x) { return ntohs(x); }
inline uint16_t hton(uint16_t x) { return htons(x); }
inline uint32_t ntoh(uint32_t x) { return ntohl(x); }
inline uint32_t hton(uint32_t x) { return htonl(x); }
inline uint64_t ntoh(uint64_t x) { return ntohq(x); }
inline uint64_t hton(uint64_t x) { return htonq(x); }
inline int8_t ntoh(int8_t x) { return x; }
inline int8_t hton(int8_t x) { return x; }
inline int16_t ntoh(int16_t x) { return ntohs(x); }
inline int16_t hton(int16_t x) { return htons(x); }
inline int32_t ntoh(int32_t x) { return ntohl(x); }
inline int32_t hton(int32_t x) { return htonl(x); }
inline int64_t ntoh(int64_t x) { return ntohq(x); }
inline int64_t hton(int64_t x) { return htonq(x); }
// Deprecated alias net::packed<> for unaligned<> from unaligned.hh.
// TODO: get rid of this alias.
template <typename T> using packed = unaligned<T>;

template <typename T>
inline T ntoh(const packed<T>& x) {
    T v = x;
    return ntoh(v);
}

template <typename T>
inline T hton(const packed<T>& x) {
    T v = x;
    return hton(v);
}

template <typename T>
inline std::ostream& operator<<(std::ostream& os, const packed<T>& v) {
    auto x = v.raw;
    return os << x;
}

inline void ntoh_inplace() {}
inline
void hton_inplace() {};

template <typename First, typename... Rest>
inline
void ntoh_inplace(First& first, Rest&... rest) {
    first = ntoh(first);
    ntoh_inplace(std::forward<Rest&>(rest)...);
}

template <typename First, typename... Rest>
inline
void hton_inplace(First& first, Rest&... rest) {
    first = hton(first);
    hton_inplace(std::forward<Rest&>(rest)...);
}

template <class T>
inline
T ntoh(const T& x) {
    T tmp = x;
    tmp.adjust_endianness([] (auto&&... what) { ntoh_inplace(std::forward<decltype(what)&>(what)...); });
    return tmp;
}

template <class T>
inline
T hton(const T& x) {
    T tmp = x;
    tmp.adjust_endianness([] (auto&&... what) { hton_inplace(std::forward<decltype(what)&>(what)...); });
    return tmp;
}
}

enum class transport {
    TCP = IPPROTO_TCP,
    SCTP = IPPROTO_SCTP
};

namespace net {
class inet_address;
}

struct listen_options {
    transport proto = transport::TCP;
    bool reuse_address = false;
    listen_options(bool rua = false)
        : reuse_address(rua)
    {}
};


namespace net {


class inet_address {
public:
    enum class family {
        INET = AF_INET, INET6 = AF_INET6
    };
private:
    family _in_family;
    union {
        ::in_addr _in;
        ::in6_addr _in6;
    };
public:
    inet_address();
    inet_address(::in_addr i);
    inet_address(::in6_addr i);
    // NOTE: does _not_ resolve the address. Only parses
    // ipv4/ipv6 numerical address
    inet_address(const std::string&);
    inet_address(inet_address&&) = default;
    inet_address(const inet_address&) = default;
    inet_address& operator=(const inet_address&) = default;
    bool operator==(const inet_address&) const;
    family in_family() const {
        return _in_family;
    }
    size_t size() const;
    const void * data() const;
    operator const ::in_addr&() const;
    operator const ::in6_addr&() const;
    future<std::string> hostname() const;
    future<std::vector<std::string>> aliases() const;
    static future<inet_address> find(const std::string&);
    static future<inet_address> find(const std::string&, family);
    static future<std::vector<inet_address>> find_all(const std::string&);
    static future<std::vector<inet_address>> find_all(const std::string&, family);
};
std::ostream& operator<<(std::ostream&, const inet_address&);
std::ostream& operator<<(std::ostream&, const inet_address::family&);
}

class unknown_host : public std::invalid_argument {
public:
    using invalid_argument::invalid_argument;
};

class ipv4_addr;
class socket_address {
public:
    union {
        ::sockaddr_storage sas;
        ::sockaddr sa;
        ::sockaddr_in in;
    } u;
    socket_address(sockaddr_in sa) {
        u.in = sa;
    }
    socket_address(ipv4_addr);
    socket_address() = default;
    ::sockaddr& as_posix_sockaddr() { return u.sa; }
    ::sockaddr_in& as_posix_sockaddr_in() { return u.in; }
    const ::sockaddr& as_posix_sockaddr() const { return u.sa; }
    const ::sockaddr_in& as_posix_sockaddr_in() const { return u.in; }

    bool operator==(const socket_address&) const;
};

struct ipv4_addr {
    uint32_t ip;
    uint16_t port;

    ipv4_addr() : ip(0), port(0) {}
    ipv4_addr(uint32_t ip, uint16_t port) : ip(ip), port(port) {}
    ipv4_addr(uint16_t port) : ip(0), port(port) {}
    ipv4_addr(const std::string &addr);
    ipv4_addr(const std::string &addr, uint16_t port);
    ipv4_addr(const net::inet_address&, uint16_t);
    ipv4_addr(const socket_address &sa) {
        ip = net::ntoh(sa.u.in.sin_addr.s_addr);
        port = net::ntoh(sa.u.in.sin_port);
    }
    ipv4_addr(socket_address &&sa) : ipv4_addr(sa) {}
};


class file_desc {
    int _fd;
public:
    file_desc() = delete;
    file_desc(const file_desc&) = delete;
    file_desc(file_desc&& x) : _fd(x._fd) { x._fd = -1; }
    ~file_desc() { if (_fd != -1) { ::close(_fd); } }
    void operator=(const file_desc&) = delete;
    file_desc& operator=(file_desc&& x) {
        if (this != &x) {
            std::swap(_fd, x._fd);
            if (x._fd != -1) {
                x.close();
            }
        }
        return *this;
    }
    void close() {
        assert(_fd != -1);
        auto r = ::close(_fd);
        throw_system_error_on(r == -1, "close");
        _fd = -1;
    }
    int get() const { return _fd; }
    static file_desc open(std::string name, int flags, mode_t mode = 0) {
        int fd = ::open(name.c_str(), flags, mode);
        throw_system_error_on(fd == -1, "open");
        return file_desc(fd);
    }
    static file_desc socket(int family, int type, int protocol = 0) {
        int fd = ::socket(family, type, protocol);
        throw_system_error_on(fd == -1, "socket");
        return file_desc(fd);
    }
    static file_desc eventfd(unsigned initval, int flags) {
        int fd = ::eventfd(initval, flags);
        throw_system_error_on(fd == -1, "eventfd");
        return file_desc(fd);
    }
    static file_desc epoll_create(int flags = 0) {
        int fd = ::epoll_create1(flags);
        throw_system_error_on(fd == -1, "epoll_create1");
        return file_desc(fd);
    }
    static file_desc timerfd_create(int clockid, int flags) {
        int fd = ::timerfd_create(clockid, flags);
        throw_system_error_on(fd == -1, "timerfd_create");
        return file_desc(fd);
    }
    static file_desc temporary(std::string directory);
    file_desc dup() const {
        int fd = ::dup(get());
        throw_system_error_on(fd == -1, "dup");
        return file_desc(fd);
    }
    file_desc accept(sockaddr& sa, socklen_t& sl, int flags = 0) {
        auto ret = ::accept4(_fd, &sa, &sl, flags);
        throw_system_error_on(ret == -1, "accept4");
        return file_desc(ret);
    }
    void shutdown(int how) {
        auto ret = ::shutdown(_fd, how);
        if (ret == -1 && errno != ENOTCONN) {
            throw_system_error_on(ret == -1, "shutdown");
        }
    }
    void truncate(size_t size) {
        auto ret = ::ftruncate(_fd, size);
        throw_system_error_on(ret, "ftruncate");
    }
    int ioctl(int request) {
        return ioctl(request, 0);
    }
    int ioctl(int request, int value) {
        int r = ::ioctl(_fd, request, value);
        throw_system_error_on(r == -1, "ioctl");
        return r;
    }
    int ioctl(int request, unsigned int value) {
        int r = ::ioctl(_fd, request, value);
        throw_system_error_on(r == -1, "ioctl");
        return r;
    }
    template <class X>
    int ioctl(int request, X& data) {
        int r = ::ioctl(_fd, request, &data);
        throw_system_error_on(r == -1, "ioctl");
        return r;
    }
    template <class X>
    int ioctl(int request, X&& data) {
        int r = ::ioctl(_fd, request, &data);
        throw_system_error_on(r == -1, "ioctl");
        return r;
    }
    template <class X>
    int setsockopt(int level, int optname, X&& data) {
        int r = ::setsockopt(_fd, level, optname, &data, sizeof(data));
        throw_system_error_on(r == -1, "setsockopt");
        return r;
    }
    int setsockopt(int level, int optname, const char* data) {
        int r = ::setsockopt(_fd, level, optname, data, strlen(data) + 1);
        throw_system_error_on(r == -1, "setsockopt");
        return r;
    }
    template <typename Data>
    Data getsockopt(int level, int optname) {
        Data data;
        socklen_t len = sizeof(data);
        memset(&data, 0, len);
        int r = ::getsockopt(_fd, level, optname, &data, &len);
        throw_system_error_on(r == -1, "getsockopt");
        return data;
    }
    int getsockopt(int level, int optname, char* data, socklen_t len) {
        int r = ::getsockopt(_fd, level, optname, data, &len);
        throw_system_error_on(r == -1, "getsockopt");
        return r;
    }
    size_t size() {
        struct stat buf;
        auto r = ::fstat(_fd, &buf);
        throw_system_error_on(r == -1, "fstat");
        return buf.st_size;
    }
    boost::optional<size_t> read(void* buffer, size_t len) {
        auto r = ::read(_fd, buffer, len);
        if (r == -1 && errno == EAGAIN) {
            return {};
        }
        throw_system_error_on(r == -1, "read");
        return { size_t(r) };
    }
    boost::optional<ssize_t> recv(void* buffer, size_t len, int flags) {
        auto r = ::recv(_fd, buffer, len, flags);
        if (r == -1 && errno == EAGAIN) {
            return {};
        }
        throw_system_error_on(r == -1, "recv");
        return { ssize_t(r) };
    }
    boost::optional<size_t> recvmsg(msghdr* mh, int flags) {
        auto r = ::recvmsg(_fd, mh, flags);
        if (r == -1 && errno == EAGAIN) {
            return {};
        }
        throw_system_error_on(r == -1, "recvmsg");
        return { size_t(r) };
    }
    boost::optional<size_t> send(const void* buffer, size_t len, int flags) {
        auto r = ::send(_fd, buffer, len, flags);
        if (r == -1 && errno == EAGAIN) {
            return {};
        }
        throw_system_error_on(r == -1, "send");
        return { size_t(r) };
    }
    boost::optional<size_t> sendto(socket_address& addr, const void* buf, size_t len, int flags) {
        auto r = ::sendto(_fd, buf, len, flags, &addr.u.sa, sizeof(addr.u.sas));
        if (r == -1 && errno == EAGAIN) {
            return {};
        }
        throw_system_error_on(r == -1, "sendto");
        return { size_t(r) };
    }
    boost::optional<size_t> sendmsg(const msghdr* msg, int flags) {
        auto r = ::sendmsg(_fd, msg, flags);
        if (r == -1 && errno == EAGAIN) {
            return {};
        }
        throw_system_error_on(r == -1, "sendmsg");
        return { size_t(r) };
    }
    void bind(sockaddr& sa, socklen_t sl) {
        auto r = ::bind(_fd, &sa, sl);
        throw_system_error_on(r == -1, "bind");
    }
    void connect(sockaddr& sa, socklen_t sl) {
        auto r = ::connect(_fd, &sa, sl);
        if (r == -1 && errno == EINPROGRESS) {
            return;
        }
        throw_system_error_on(r == -1, "connect");
    }
    socket_address get_address() {
        socket_address addr;
        auto len = (socklen_t) sizeof(addr.u.sas);
        auto r = ::getsockname(_fd, &addr.u.sa, &len);
        throw_system_error_on(r == -1, "getsockname");
        return addr;
    }
    void listen(int backlog) {
        auto fd = ::listen(_fd, backlog);
        throw_system_error_on(fd == -1, "listen");
    }
    boost::optional<size_t> write(const void* buf, size_t len) {
        auto r = ::write(_fd, buf, len);
        if (r == -1 && errno == EAGAIN) {
            return {};
        }
        throw_system_error_on(r == -1, "write");
        return { size_t(r) };
    }
    boost::optional<size_t> writev(const iovec *iov, int iovcnt) {
        auto r = ::writev(_fd, iov, iovcnt);
        if (r == -1 && errno == EAGAIN) {
            return {};
        }
        throw_system_error_on(r == -1, "writev");
        return { size_t(r) };
    }
    size_t pread(void* buf, size_t len, off_t off) {
        auto r = ::pread(_fd, buf, len, off);
        throw_system_error_on(r == -1, "pread");
        return size_t(r);
    }
    void timerfd_settime(int flags, const itimerspec& its) {
        auto r = ::timerfd_settime(_fd, flags, &its, NULL);
        throw_system_error_on(r == -1, "timerfd_settime");
    }

    mmap_area map(size_t size, unsigned prot, unsigned flags, size_t offset,
            void* addr = nullptr) {
        void *x = mmap(addr, size, prot, flags, _fd, offset);
        throw_system_error_on(x == MAP_FAILED, "mmap");
        return mmap_area(static_cast<char*>(x), mmap_deleter{size});
    }

    mmap_area map_shared_rw(size_t size, size_t offset) {
        return map(size, PROT_READ | PROT_WRITE, MAP_SHARED, offset);
    }

    mmap_area map_shared_ro(size_t size, size_t offset) {
        return map(size, PROT_READ, MAP_SHARED, offset);
    }

    mmap_area map_private_rw(size_t size, size_t offset) {
        return map(size, PROT_READ | PROT_WRITE, MAP_PRIVATE, offset);
    }

    mmap_area map_private_ro(size_t size, size_t offset) {
        return map(size, PROT_READ, MAP_PRIVATE, offset);
    }

private:
    file_desc(int fd) : _fd(fd) {}
 };

file_desc
file_desc::temporary(std::string directory) {
    // FIXME: add O_TMPFILE support one day
    directory += "/XXXXXX";
    std::vector<char> templat(directory.c_str(), directory.c_str() + directory.size() + 1);
    int fd = ::mkstemp(templat.data());

    int r = ::unlink(templat.data());
    // throw_system_error_on(r == -1); // leaks created file, but what can we do?
    return file_desc(fd);
}







/*---------------------------------------------------memory相关-----------------------------------------------------*/

namespace memory {

/// \cond internal
// TODO: Use getpagesize() in order to learn a size of a system PAGE.
static constexpr size_t page_bits = 12;
static constexpr size_t page_size = 1 << page_bits;       // 4K
static constexpr size_t huge_page_size = 512 * page_size; // 2M

void configure(std::vector<resource::memory> m,
        std::optional<std::string> hugetlbfs_path = {});

void enable_abort_on_allocation_failure();

class disable_abort_on_alloc_failure_temporarily {
public:
    disable_abort_on_alloc_failure_temporarily();
    ~disable_abort_on_alloc_failure_temporarily() noexcept;
};

void set_heap_profiling_enabled(bool);

enum class reclaiming_result {
    reclaimed_nothing,
    reclaimed_something
};

enum class reclaimer_scope {
    async,
    sync
};

class reclaimer {
public:
    using reclaim_fn = std::function<reclaiming_result ()>;
private:
    reclaim_fn _reclaim;
    reclaimer_scope _scope;
public:
    reclaimer(reclaim_fn reclaim, reclaimer_scope scope = reclaimer_scope::async);
    ~reclaimer();
    reclaiming_result do_reclaim() { return _reclaim(); }
    reclaimer_scope scope() const { return _scope; }
};

bool drain_cross_cpu_freelist();

void set_reclaim_hook(
        std::function<void (std::function<void ()>)> hook);

using physical_address = uint64_t;

struct translation {
    translation() = default;
    translation(physical_address a, size_t s) : addr(a), size(s) {}
    physical_address addr = 0;
    size_t size = 0;
};

translation translate(const void* addr, size_t size);
class statistics;
statistics stats();

/// Memory allocation statistics.
class statistics {
    uint64_t _mallocs;
    uint64_t _frees;
    uint64_t _cross_cpu_frees;
    size_t _total_memory;
    size_t _free_memory;
    uint64_t _reclaims;
private:
    statistics(uint64_t mallocs, uint64_t frees, uint64_t cross_cpu_frees,
            uint64_t total_memory, uint64_t free_memory, uint64_t reclaims)
        : _mallocs(mallocs), _frees(frees), _cross_cpu_frees(cross_cpu_frees)
        , _total_memory(total_memory), _free_memory(free_memory), _reclaims(reclaims) {}
public:
    uint64_t mallocs() const { return _mallocs; }
    uint64_t frees() const { return _frees; }
    uint64_t cross_cpu_frees() const { return _cross_cpu_frees; }
    /// Total number of objects which were allocated but not freed.
    size_t live_objects() const { return mallocs() - frees(); }
    size_t free_memory() const { return _free_memory; }
    /// Total allocated memory (in bytes)
    size_t allocated_memory() const { return _total_memory - _free_memory; }
    /// Total memory (in bytes)
    size_t total_memory() const { return _total_memory; }
    /// Number of reclaims performed due to low memory
    uint64_t reclaims() const { return _reclaims; }
    friend statistics stats();
};

struct memory_layout {
    uintptr_t start;
    uintptr_t end;
};

// Discover virtual address range used by the allocator on current shard.
// Supported only when seastar allocator is enabled.
memory_layout get_memory_layout();

/// Returns the value of free memory low water mark in bytes.
/// When free memory is below this value, reclaimers are invoked until it goes above again.
size_t min_free_memory();

/// Sets the value of free memory low water mark in memory::page_size units.
void set_min_free_pages(size_t pages);
// Memory allocation functions
    void* allocate(size_t size);
    void* allocate_aligned(size_t align, size_t size);
    void* allocate_large(size_t size);
    void* allocate_large_aligned(size_t align, size_t size);
    void free(void* ptr);
    void free(void* ptr, size_t size);
    void free_large(void* ptr);
    size_t object_size(void* ptr);
    void shrink(void* ptr, size_t new_size);

}

class with_alignment {
    size_t _align;
public:
    with_alignment(size_t align) : _align(align) {}
    size_t alignment() const { return _align; }
};

void* operator new(size_t size, with_alignment wa);
void* operator new[](size_t size, with_alignment wa);
void operator delete(void* ptr, with_alignment wa);
void operator delete[](void* ptr, with_alignment wa);



template <typename... T, typename... A>
inline
future<T...> make_ready_future(A&&... value) {
    return future<T...>(ready_future_marker(), std::forward<A>(value)...);
}

template <typename... T>
inline
future<T...> make_exception_future(std::exception_ptr ex) noexcept {
    return future<T...>(exception_future_marker(), std::move(ex));
}

class timed_out_error : public std::exception {
public:
    virtual const char* what() const noexcept {
        return "timedout";
    }
};

template<typename T>
struct dummy_expiry {
    void operator()(T&) noexcept {};
};
template<typename... T>
struct promise_expiry {
    void operator()(promise<T...>& pr) noexcept {
        pr.set_exception(std::make_exception_ptr(timed_out_error()));
    };
};

template <typename T, typename OnExpiry = dummy_expiry<T>, typename Clock = lowres_clock>
class expiring_fifo {
public:
    using clock = Clock;
    using time_point = typename Clock::time_point;
private:
    struct entry {
        std::optional<T> payload;
        timer<Clock> tr;
        entry(T&& payload_) : payload(std::move(payload_)) {}
        entry(const T& payload_) : payload(payload_) {}
        entry(T payload_, expiring_fifo& ef, time_point timeout)
                : payload(std::move(payload_))
                , tr([this, &ef] {
                    ef._on_expiry(*payload);
                    payload = std::nullopt;
                    --ef._size;
                    ef.drop_expired_front();
                })
        {
            tr.arm(timeout);
        }
        entry(entry&& x) = delete;
        entry(const entry& x) = delete;
    };

    std::deque<entry> _list;
    OnExpiry _on_expiry;
    size_t _size = 0;

    void drop_expired_front() {
        while (!_list.empty() && !_list.front().payload) {
            _list.pop_front();
        }
    }
public:
    expiring_fifo() = default;
    expiring_fifo(OnExpiry on_expiry) : _on_expiry(std::move(on_expiry)) {}

    bool empty() const {
        return _size == 0;
    }

    explicit operator bool() const {
        return !empty();
    }

    T& front() {
        return *_list.front().payload;
    }

    const T& front() const {
        return *_list.front().payload;
    }

    size_t size() const {
        return _size;
    }
    void reserve(size_t size) {
        return _list.reserve(size);
    }
    void push_back(const T& payload) {
        _list.emplace_back(payload);
        ++_size;
    }
    void push_back(T&& payload) {
        _list.emplace_back(std::move(payload));
        ++_size;
    }
    void push_back(T payload, time_point timeout) {
        if (timeout < time_point::max()) {
            _list.emplace_back(std::move(payload), *this, timeout);
        } else {
            _list.emplace_back(std::move(payload));
        }
        ++_size;
    }
    void pop_front() {
        _list.pop_front();
        --_size;
        drop_expired_front();
    }
};




class broken_semaphore : public std::exception {
public:
    /// Reports the exception reason.
    virtual const char* what() const noexcept {
        return "Semaphore broken";
    }
};
class semaphore_timed_out : public std::exception {
public:
    /// Reports the exception reason.
    virtual const char* what() const noexcept {
        return "Semaphore timedout";
    }
};
struct semaphore_default_exception_factory {
    static semaphore_timed_out timeout() {
        return semaphore_timed_out();
    }
    static broken_semaphore broken() {
        return broken_semaphore();
    }
};


template<typename ExceptionFactory, typename Clock = typename timer<>::clock>
class basic_semaphore {
public:
    using duration = typename timer<Clock>::duration;
    using clock = typename timer<Clock>::clock;
    using time_point = typename timer<Clock>::time_point;
private:
    ssize_t _count;
    std::exception_ptr _ex;
    struct entry {
        promise<> pr;
        size_t nr;
        entry(promise<>&& pr_, size_t nr_) : pr(std::move(pr_)), nr(nr_) {}
    };
    struct expiry_handler {
        void operator()(entry& e) noexcept {
            e.pr.set_exception(std::make_exception_ptr(ExceptionFactory::timeout()));
        }
    };
    expiring_fifo<entry, expiry_handler, clock> _wait_list;
    bool has_available_units(size_t nr) const {
        return _count >= 0 && (static_cast<size_t>(_count) >= nr);
    }
    bool may_proceed(size_t nr) const {
        return has_available_units(nr) && _wait_list.empty();
    }
public:
    static constexpr size_t max_counter() {
        return std::numeric_limits<decltype(_count)>::max();
    }
    basic_semaphore(size_t count) : _count(count) {}
    future<> wait(size_t nr = 1) {
        return wait(time_point::max(), nr);
    }

    future<> wait(duration timeout, size_t nr = 1) {
        return wait(Clock::now() + timeout, nr);
    }
    future<> wait(time_point timeout, size_t nr = 1) {
        if (may_proceed(nr)) {
            _count -= nr;
            return make_ready_future<>();
        }
        if (_ex) {
            return make_exception_future(_ex);
        }
        promise<> pr;
        auto fut = pr.get_future();
        _wait_list.push_back(entry(std::move(pr), nr), timeout);
        return fut;
    }
     void signal(size_t nr = 1) {
        if (_ex) {
            return;
        }
        _count += nr;
        while (!_wait_list.empty() && has_available_units(_wait_list.front().nr)) {
            auto& x = _wait_list.front();
            _count -= x.nr;
            x.pr.set_value();
            _wait_list.pop_front();
        }
    }

    void consume(size_t nr = 1) {
        if (_ex) {
            return;
        }
        _count -= nr;
    }
    bool try_wait(size_t nr = 1) {
        if (may_proceed(nr)) {
            _count -= nr;
            return true;
        } else {
            return false;
        }
    }
    size_t current() const { return std::max(_count, ssize_t(0)); }
    ssize_t available_units() const { return _count; }
    size_t waiters() const { return _wait_list.size(); }
    void broken() { broken(std::make_exception_ptr(ExceptionFactory::broken())); }
    template <typename Exception>
    void broken(const Exception& ex) {
        broken(std::make_exception_ptr(ex));
    }
    void broken(std::exception_ptr ex);
    void ensure_space_for_waiters(size_t n) {
        _wait_list.reserve(n);
    }
};

template<typename ExceptionFactory = semaphore_default_exception_factory, typename Clock = typename timer<>::clock>
class semaphore_units {
    basic_semaphore<ExceptionFactory, Clock>& _sem;
    size_t _n;
public:
    semaphore_units(basic_semaphore<ExceptionFactory, Clock>& sem, size_t n) noexcept : _sem(sem), _n(n) {}
    semaphore_units(semaphore_units&& o) noexcept : _sem(o._sem), _n(o._n) {
        o._n = 0;
    }
    semaphore_units& operator=(semaphore_units&& o) noexcept {
        if (this != &o) {
            this->~semaphore_units();
            new (this) semaphore_units(std::move(o));
        }
        return *this;
    }
    semaphore_units(const semaphore_units&) = delete;
    ~semaphore_units() noexcept {
        if (_n) {
            _sem.signal(_n);
        }
    }
    /// Releases ownership of the units. The semaphore will not be signalled.
    ///
    /// \return the number of units held
    size_t release() {
        return std::exchange(_n, 0);
    }
};

using semaphore = basic_semaphore<semaphore_default_exception_factory>;

class broken_condition_variable : public std::exception {
public:
    /// Reports the exception reason.
    virtual const char* what() const noexcept {
        return "Condition variable is broken";
    }
};

class condition_variable_timed_out : public std::exception {
public:
    /// Reports the exception reason.
    virtual const char* what() const noexcept {
        return "Condition variable timed out";
    }
};


class condition_variable {
    using duration = semaphore::duration;
    using clock = semaphore::clock;
    using time_point = semaphore::time_point;
    struct condition_variable_exception_factory {
        static condition_variable_timed_out timeout() {
            return condition_variable_timed_out();
        }
        static broken_condition_variable broken() {
            return broken_condition_variable();
        }
    };
    basic_semaphore<condition_variable_exception_factory> _sem;
public:
    condition_variable() : _sem(0) {}
    future<> wait() {
        return _sem.wait();
    }

    future<> wait(time_point timeout) {
        return _sem.wait(timeout);
    }
    future<> wait(duration timeout) {
        return _sem.wait(timeout);
    }
    template<typename Pred>
    future<> wait(Pred&& pred) {
        return do_until(std::forward<Pred>(pred), [this] {
            return wait();
        });
    }
    template<typename Pred>
    future<> wait(time_point timeout, Pred&& pred) {
        return do_until(std::forward<Pred>(pred), [this, timeout] () mutable {
            return wait(timeout);
        });
    }
    template<typename Pred>
    future<> wait(duration timeout, Pred&& pred) {
        return wait(clock::now() + timeout, std::forward<Pred>(pred));
    }
    void signal() {
        if (_sem.waiters()) {
            _sem.signal();
        }
    }
    void broadcast() {
        _sem.signal(_sem.waiters());
    }
    void broken() {
        _sem.broken();
    }
};



/*----------------------------metrics---------------------------------------*/

// namespace metrics {
// struct histogram_bucket {
//     uint64_t count = 0; // number of events.
//     double upper_bound = 0;      // Inclusive.
// };

// struct histogram {
//     uint64_t sample_count = 0;
//     double sample_sum = 0;
//     std::vector<histogram_bucket> buckets; // Ordered in increasing order of upper_bound, +Inf bucket is optional.
//     histogram& operator+=(const histogram& h);
//     histogram operator+(const histogram& h) const;
//     histogram operator+(histogram&& h) const;
// };
// }
// #include <boost/variant.hpp>

// namespace metrics {
// namespace impl {
// class metric_groups_def;
// struct metric_definition_impl;
// class metric_groups_impl;
// }

// using group_name_type = std::string; 
// class metric_groups;

// class metric_definition {
//     std::unique_ptr<impl::metric_definition_impl> _impl;
// public:
//     metric_definition(const impl::metric_definition_impl& impl) noexcept;
//     metric_definition(metric_definition&& m) noexcept;
//     ~metric_definition();
//     friend metric_groups;
//     friend impl::metric_groups_impl;
// };
// class metric_group_definition {
// public:
//     group_name_type name;
//     std::initializer_list<metric_definition> metrics;
//     metric_group_definition(const group_name_type& name, std::initializer_list<metric_definition> l);
//     metric_group_definition(const metric_group_definition&) = delete;
//     ~metric_group_definition();
// };

// class metric_groups {
//     std::unique_ptr<impl::metric_groups_def> _impl;
// public:
//     metric_groups() noexcept;
//     metric_groups(metric_groups&&) = default;
//     virtual ~metric_groups();
//     metric_groups& operator=(metric_groups&&) = default;
//     metric_groups(std::initializer_list<metric_group_definition> mg);
//     metric_groups& add_group(const group_name_type& name, const std::initializer_list<metric_definition>& l);
//     void clear();
// };


// class metric_group : public metric_groups {
// public:
//     metric_group() noexcept;
//     metric_group(const metric_group&) = delete;
//     metric_group(metric_group&&) = default;
//     virtual ~metric_group();
//     metric_group& operator=(metric_group&&) = default;
//     /*!
//      * \brief add metrics belong to the same group in the constructor.
//      *
//      *
//      */
//     metric_group(const group_name_type& name, std::initializer_list<metric_definition> l);
// };
// }

// namespace metrics {

// using metric_type_def = std::string;
// using metric_name_type = std::string; 
// using instance_id_type = std::string; 


// class description {
// public:
//     description(std::string s) : _s(std::move(s))
//     {}
//     const std::string& str() const {
//         return _s;
//     }
// private:
//     std::string _s;
// };//这个地方有问题

// class label_instance {
//     std::string _key;
//     std::string _value;
// public:
//     template<typename T>
//     label_instance(const std::string& key, T v) : _key(key), _value(boost::lexical_cast<std::string>(v)){}

//     const std::string key() const {
//         return _key;
//     }
//     const std::string value() const {
//         return _value;
//     }
//     bool operator<(const label_instance&) const;
//     bool operator==(const label_instance&) const;
//     bool operator!=(const label_instance&) const;
// };

// class label {
//     std::string key;
// public:
//     using instance = label_instance;
//     explicit label(const std::string& key) : key(key) {
//     }
//     template<typename T>
//     instance operator()(T value) const {
//         return label_instance(key, std::forward<T>(value));
//     }
//     const std::string& name() const {
//         return key;
//     }
// };

// namespace impl {
// // The value binding data types
// enum class data_type : uint8_t {
//     COUNTER, // unsigned int 64
//     GAUGE, // double
//     DERIVE, // signed int 64
//     ABSOLUTE, // unsigned int 64
//     HISTOGRAM,
// };
// /*!
//  * \breif A helper class that used to return metrics value.
//  * Do not use directly @see metrics_creation
//  */
// struct metric_value {
//     boost::variant<double, histogram> u;
//     data_type _type;
//     data_type type() const {
//         return _type;
//     }
//     double d() const {
//         return boost::get<double>(u);
//     }
//     uint64_t ui() const {
//         return boost::get<double>(u);
//     }

//     int64_t i() const {
//         return boost::get<double>(u);
//     }

//     metric_value()
//             : _type(data_type::GAUGE) {
//     }

//     metric_value(histogram&& h, data_type t = data_type::HISTOGRAM) :
//         u(std::move(h)), _type(t) {
//     }
//     metric_value(const histogram& h, data_type t = data_type::HISTOGRAM) :
//         u(h), _type(t) {
//     }

//     metric_value(double d, data_type t)
//             : u(d), _type(t) {
//     }

//     metric_value& operator=(const metric_value& c) = default;

//     metric_value& operator+=(const metric_value& c) {
//         *this = *this + c;
//         return *this;
//     }

//     metric_value operator+(const metric_value& c);
//     const histogram& get_histogram() const {
//         return boost::get<histogram>(u);
//     }
// };

// using metric_function = std::function<metric_value()>;

// struct metric_type {
//     data_type base_type;
//     metric_type_def type_name;
// };

// struct metric_definition_impl {
//     metric_name_type name;
//     metric_type type;
//     metric_function f;
//     description d;
//     bool enabled = true;
//     std::map<std::string, std::string> labels;
//     metric_definition_impl& operator ()(bool enabled);
//     metric_definition_impl& operator ()(const label_instance& label);
//     metric_definition_impl(
//         metric_name_type name,
//         metric_type type,
//         metric_function f,
//         description d,
//         std::vector<label_instance> labels);
// };

// class metric_groups_def {
// public:
//     metric_groups_def() = default;
//     virtual ~metric_groups_def() = default;
//     metric_groups_def(const metric_groups_def&) = delete;
//     metric_groups_def(metric_groups_def&&) = default;
//     virtual metric_groups_def& add_metric(group_name_type name, const metric_definition& md) = 0;
//     virtual metric_groups_def& add_group(group_name_type name, const std::initializer_list<metric_definition>& l) = 0;
//     virtual metric_groups_def& add_group(group_name_type name, const std::vector<metric_definition>& l) = 0;
// };

// instance_id_type shard();

// template<typename T, typename En = std::true_type>
// struct is_callable;

// template<typename T>
// struct is_callable<T, typename std::integral_constant<bool, !std::is_void<typename std::result_of<T()>::type>::value>::type> : public std::true_type {
// };

// template<typename T>
// struct is_callable<T, typename std::enable_if<std::is_fundamental<T>::value, std::true_type>::type> : public std::false_type {
// };

// template<typename T, typename = std::enable_if_t<is_callable<T>::value>>
// metric_function make_function(T val, data_type dt) {
//     return [dt, val] {
//         return metric_value(val(), dt);
//     };
// }

// template<typename T, typename = std::enable_if_t<!is_callable<T>::value>>
// metric_function make_function(T& val, data_type dt) {
//     return [dt, &val] {
//         return metric_value(val, dt);
//     };
// }
// }

// extern const bool metric_disabled;

// extern label shard_label;
// extern label type_label;

// template<typename T>
// impl::metric_definition_impl make_gauge(metric_name_type name,
//         T&& val, description d=description(), std::vector<label_instance> labels = {}) {
//     return {name, {impl::data_type::GAUGE, "gauge"}, make_function(std::forward<T>(val), impl::data_type::GAUGE), d, labels};
// }

// template<typename T>
// impl::metric_definition_impl make_gauge(metric_name_type name,
//         description d, T&& val) {
//     return {name, {impl::data_type::GAUGE, "gauge"}, make_function(std::forward<T>(val), impl::data_type::GAUGE), d, {}};
// }

// template<typename T>
// impl::metric_definition_impl make_gauge(metric_name_type name,
//         description d, std::vector<label_instance> labels, T&& val) {
//     return {name, {impl::data_type::GAUGE, "gauge"}, make_function(std::forward<T>(val), impl::data_type::GAUGE), d, labels};
// }

// template<typename T>
// impl::metric_definition_impl make_derive(metric_name_type name,
//         T&& val, description d=description(), std::vector<label_instance> labels = {}) {
//     return {name, {impl::data_type::DERIVE, "derive"}, make_function(std::forward<T>(val), impl::data_type::DERIVE), d, labels};
// }

// template<typename T>
// impl::metric_definition_impl make_derive(metric_name_type name, description d,
//         T&& val) {
//     return {name, {impl::data_type::DERIVE, "derive"}, make_function(std::forward<T>(val), impl::data_type::DERIVE), d, {}};
// }

// template<typename T>
// impl::metric_definition_impl make_derive(metric_name_type name, description d, std::vector<label_instance> labels,
//         T&& val) {
//     return {name, {impl::data_type::DERIVE, "derive"}, make_function(std::forward<T>(val), impl::data_type::DERIVE), d, labels};
// }

// template<typename T>
// impl::metric_definition_impl make_counter(metric_name_type name,
//         T&& val, description d=description(), std::vector<label_instance> labels = {}) {
//     return {name, {impl::data_type::COUNTER, "counter"}, make_function(std::forward<T>(val), impl::data_type::COUNTER), d, labels};
// }

// template<typename T>
// impl::metric_definition_impl make_absolute(metric_name_type name,
//         T&& val, description d=description(), std::vector<label_instance> labels = {}) {
//     return {name, {impl::data_type::ABSOLUTE, "absolute"}, make_function(std::forward<T>(val), impl::data_type::ABSOLUTE), d, labels};
// }

// template<typename T>
// impl::metric_definition_impl make_histogram(metric_name_type name,
//         T&& val, description d=description(), std::vector<label_instance> labels = {}) {
//     return  {name, {impl::data_type::HISTOGRAM, "histogram"}, make_function(std::forward<T>(val), impl::data_type::HISTOGRAM), d, labels};
// }

// template<typename T>
// impl::metric_definition_impl make_histogram(metric_name_type name,
//         description d, std::vector<label_instance> labels, T&& val) {
//     return  {name, {impl::data_type::HISTOGRAM, "histogram"}, make_function(std::forward<T>(val), impl::data_type::HISTOGRAM), d, labels};
// }

// template<typename T>
// impl::metric_definition_impl make_histogram(metric_name_type name,
//         description d, T&& val) {
//     return  {name, {impl::data_type::HISTOGRAM, "histogram"}, make_function(std::forward<T>(val), impl::data_type::HISTOGRAM), d, {}};
// }

// template<typename T>
// impl::metric_definition_impl make_total_bytes(metric_name_type name,
//         T&& val, description d=description(), std::vector<label_instance> labels = {},
//         instance_id_type instance = impl::shard()) {
//     return make_derive(name, std::forward<T>(val), d, labels)(type_label("total_bytes"));
// }

// template<typename T>
// impl::metric_definition_impl make_current_bytes(metric_name_type name,
//         T&& val, description d=description(), std::vector<label_instance> labels = {},
//         instance_id_type instance = impl::shard()) {
//     return make_derive(name, std::forward<T>(val), d, labels)(type_label("bytes"));
// }

// template<typename T>
// impl::metric_definition_impl make_queue_length(metric_name_type name,
//         T&& val, description d=description(), std::vector<label_instance> labels = {},
//         instance_id_type instance = impl::shard()) {
//     return make_gauge(name, std::forward<T>(val), d, labels)(type_label("queue_length"));
// }

// template<typename T>
// impl::metric_definition_impl make_total_operations(metric_name_type name,
//         T&& val, description d=description(), std::vector<label_instance> labels = {},
//         instance_id_type instance = impl::shard()) {
//     return make_derive(name, std::forward<T>(val), d, labels)(type_label("total_operations"));
// }
// }

// namespace metrics {
// namespace impl {
//     using labels_type = std::map<std::string, std::string>;
// }
// }

// namespace std {

// template<>
// struct hash<metrics::impl::labels_type> {
//     using argument_type = metrics::impl::labels_type;
//     using result_type = ::std::size_t;
//     result_type operator()(argument_type const& s) const {
//         result_type h = 0;
//         for (auto&& i : s) {
//             boost::hash_combine(h, std::hash<std::string>{}(i.second));
//         }
//         return h;
//     }
// };

// }

// namespace metrics {
// namespace impl {

// class metric_id {
// public:
//     metric_id() = default;
//     metric_id(group_name_type group, metric_name_type name,
//                     labels_type labels = {})
//                     : _group(std::move(group)), _name(
//                                     std::move(name)), _labels(labels) {
//     }

//     metric_id(metric_id &&) = default;
//     metric_id(const metric_id &) = default;
//     metric_id & operator=(metric_id &&) = default;
//     metric_id & operator=(const metric_id &) = default;
//  void unregister_metric(const metric_id & id);
//     const group_name_type & group_name() const {
//         return _group;
//     }

//     void group_name(const group_name_type & name) {
//         _group = name;
//     }

//     const instance_id_type & instance_id() const {
//         return _labels.at(shard_label.name());
//     }
//     const metric_name_type & name() const {
//         return _name;
//     }
//     const metrics::metric_type_def & inherit_type() const {
//         return _labels.at(type_label.name());
//     }
//     const labels_type& labels() const {
//         return _labels;
//     }
//     std::string full_name() const;
//     bool operator<(const metric_id&) const;
//     bool operator==(const metric_id&) const;
// private:
//     auto as_tuple() const {
//         return std::tie(group_name(), instance_id(), name(),
//                     inherit_type(), labels());
//     }
//     group_name_type _group;
//     metric_name_type _name;
//     labels_type _labels;
// };
// }


// using metrics_registration = std::vector<metric_id>;

// class metric_groups_impl : public metric_groups_def {
//     metrics_registration _registration;
// public:
//     metric_groups_impl() = default;
//     ~metric_groups_impl();
//     metric_groups_impl(const metric_groups_impl&) = delete;
//     metric_groups_impl(metric_groups_impl&&) = default;
//     metric_groups_impl& add_metric(group_name_type name, const metric_definition& md);
//     metric_groups_impl& add_group(group_name_type name, const std::initializer_list<metric_definition>& l);
//     metric_groups_impl& add_group(group_name_type name, const std::vector<metric_definition>& l);
// };

// class impl;
// class registered_metric {
//     data_type _type;
//     description _d;
//     bool _enabled;
//     metric_function _f;
//     shared_ptr<impl> _impl;
//     metric_id _id;
// public:
//     registered_metric(metric_id id, data_type type, metric_function f, description d = description(), bool enabled=true);
//     virtual ~registered_metric() {}
//     virtual metric_value operator()() const {
//         return _f();
//     }
//     data_type get_type() const {
//         return _type;
//     }

//     bool is_enabled() const {
//         return _enabled;
//     }

//     void set_enabled(bool b) {
//         _enabled = b;
//     }

//     const description& get_description() const {
//         return _d;
//     }

//     const metric_id& get_id() const {
//         return _id;
//     }
// };

// /*!
//  * \brief holds information that relevant to all metric instances
//  */
// struct metric_info {
//     data_type type;
// };

// using register_ref = shared_ptr<registered_metric>;
// using metric_instances = std::unordered_map<labels_type, register_ref>;

// class metric_family {
//     metric_instances _instances;
//     metric_info _info;
// public:
//     using iterator = metric_instances::iterator;
//     using const_iterator = metric_instances::const_iterator;

//     metric_family() = default;
//     metric_family(const metric_family&) = default;
//     metric_family(const metric_instances& instances) : _instances(instances) {
//     }
//     metric_family(const metric_instances& instances, const metric_info& info) : _instances(instances), _info(info) {
//     }
//     metric_family(metric_instances&& instances, metric_info&& info) : _instances(std::move(instances)), _info(std::move(info)) {
//     }
//     metric_family(metric_instances&& instances) : _instances(std::move(instances)) {
//     }

//     register_ref& operator[](const labels_type& l) {
//         return _instances[l];
//     }

//     const register_ref& at(const labels_type& l) const {
//         return _instances.at(l);
//     }

//     metric_info& info() {
//         return _info;
//     }

//     const metric_info& info() const {
//         return _info;
//     }

//     iterator find(const labels_type& l) {
//         return _instances.find(l);
//     }

//     const_iterator find(const labels_type& l) const {
//         return _instances.find(l);
//     }

//     iterator begin() {
//         return _instances.begin();
//     }

//     const_iterator begin() const {
//         return _instances.cbegin();
//     }

//     iterator end() {
//         return _instances.end();
//     }

//     bool empty() const {
//         return _instances.empty();
//     }

//     iterator erase(const_iterator position) {
//         return _instances.erase(position);
//     }

//     const_iterator end() const {
//         return _instances.cend();
//     }
// };

// using value_map = std::unordered_map<std::string, metric_family>;
// using value_holder = std::tuple<register_ref, metric_value>;
// using value_vector = std::vector<value_holder>;
// using values_copy = std::unordered_map<std::string, value_vector>;

// struct config {
//     std::string hostname;
// };
// class impl {
//     value_map _value_map;
//     config _config;
// public:
//     value_map& get_value_map() {
//         return _value_map;
//     }
//     const value_map& get_value_map() const {
//         return _value_map;
//     }
//     void add_registration(const metric_id& id, shared_ptr<registered_metric> rm);
//     future<> stop() {
//         return make_ready_future<>();
//     }
//     const config& get_config() const {
//         return _config;
//     }
//     void set_config(const config& c) {
//         _config = c;
//     }
// };
// const value_map& get_value_map();
// values_copy get_values();
// shared_ptr<impl> get_local_impl();
// void unregister_metric(const metric_id & id);

// std::unique_ptr<metric_groups_def> create_metric_groups();

// }

// future<> configure(const boost::program_options::variables_map & opts);

// /*!
//  * \brief get the metrics configuration desciprtion
//  */

// boost::program_options::options_description get_options_description();

// }


// namespace std {

// template<>
// struct hash<metrics::impl::metric_id>{
//     typedef metrics::impl::metric_id argument_type;
//     typedef ::std::size_t result_type;
//     result_type operator()(argument_type const& s) const
//     {
//         result_type const h1 ( std::hash<std::string>{}(s.group_name()) );
//         result_type const h2 ( std::hash<std::string>{}(s.instance_id()) );
//         return h1 ^ (h2 << 1); // or use boost::hash_combine
//     }
// };

// }

// namespace metrics {
// namespace impl {
// using metrics_registration = std::vector<metric_id>;

// class metric_groups_impl : public metric_groups_def {
//     metrics_registration _registration;
// public:
//     metric_groups_impl() = default;
//     ~metric_groups_impl();
//     metric_groups_impl(const metric_groups_impl&) = delete;
//     metric_groups_impl(metric_groups_impl&&) = default;
//     metric_groups_impl& add_metric(group_name_type name, const metric_definition& md);
//     metric_groups_impl& add_group(group_name_type name, const std::initializer_list<metric_definition>& l);
//     metric_groups_impl& add_group(group_name_type name, const std::vector<metric_definition>& l);
// };

// class impl;
// class registered_metric {
//     data_type _type;
//     description _d;
//     bool _enabled;
//     metric_function _f;
//     shared_ptr<impl> _impl;
//     metric_id _id;
// public:
//     registered_metric(metric_id id, data_type type, metric_function f, description d = description(), bool enabled=true);
//     virtual ~registered_metric() {}
//     virtual metric_value operator()() const {
//         return _f();
//     }
//     data_type get_type() const {
//         return _type;
//     }

//     bool is_enabled() const {
//         return _enabled;
//     }

//     void set_enabled(bool b) {
//         _enabled = b;
//     }

//     const description& get_description() const {
//         return _d;
//     }

//     const metric_id& get_id() const {
//         return _id;
//     }
// };

// /*!
//  * \brief holds information that relevant to all metric instances
//  */
// struct metric_info {
//     data_type type;
// };

// using register_ref = shared_ptr<registered_metric>;
// using metric_instances = std::unordered_map<labels_type, register_ref>;

// class metric_family {
//     metric_instances _instances;
//     metric_info _info;
// public:
//     using iterator = metric_instances::iterator;
//     using const_iterator = metric_instances::const_iterator;

//     metric_family() = default;
//     metric_family(const metric_family&) = default;
//     metric_family(const metric_instances& instances) : _instances(instances) {
//     }
//     metric_family(const metric_instances& instances, const metric_info& info) : _instances(instances), _info(info) {
//     }
//     metric_family(metric_instances&& instances, metric_info&& info) : _instances(std::move(instances)), _info(std::move(info)) {
//     }
//     metric_family(metric_instances&& instances) : _instances(std::move(instances)) {
//     }

//     register_ref& operator[](const labels_type& l) {
//         return _instances[l];
//     }

//     const register_ref& at(const labels_type& l) const {
//         return _instances.at(l);
//     }

//     metric_info& info() {
//         return _info;
//     }

//     const metric_info& info() const {
//         return _info;
//     }

//     iterator find(const labels_type& l) {
//         return _instances.find(l);
//     }

//     const_iterator find(const labels_type& l) const {
//         return _instances.find(l);
//     }

//     iterator begin() {
//         return _instances.begin();
//     }

//     const_iterator begin() const {
//         return _instances.cbegin();
//     }

//     iterator end() {
//         return _instances.end();
//     }

//     bool empty() const {
//         return _instances.empty();
//     }

//     iterator erase(const_iterator position) {
//         return _instances.erase(position);
//     }

//     const_iterator end() const {
//         return _instances.cend();
//     }
// };

// using value_map = std::unordered_map<std::string, metric_family>;
// using value_holder = std::tuple<register_ref, metric_value>;
// using value_vector = std::vector<value_holder>;
// using values_copy = std::unordered_map<std::string, value_vector>;

// struct config {
//     std::string hostname;
// };
// class impl {
//     value_map _value_map;
//     config _config;
// public:
//     value_map& get_value_map() {
//         return _value_map;
//     }
//     const value_map& get_value_map() const {
//         return _value_map;
//     }
//     void add_registration(const metric_id& id, shared_ptr<registered_metric> rm);
//     future<> stop() {
//         return make_ready_future<>();
//     }
//     const config& get_config() const {
//         return _config;
//     }
//     void set_config(const config& c) {
//         _config = c;
//     }
// };
// const value_map& get_value_map();
// values_copy get_values();
// shared_ptr<impl> get_local_impl();
// void unregister_metric(const metric_id & id);

// std::unique_ptr<metric_groups_def> create_metric_groups();

// }

// future<> configure(const boost::program_options::variables_map & opts);

// /*!
//  * \brief get the metrics configuration desciprtion
//  */

// boost::program_options::options_description get_options_description();

// }



class priority_class {
    struct request {
        promise<> pr;
        unsigned weight;
    };
    friend class fair_queue;
    uint32_t _shares = 0;
    float _accumulated = 0;
    std::deque<request> _queue;
    bool _queued = false;
    friend struct shared_ptr_no_esft<priority_class>;
    explicit priority_class(uint32_t shares) : _shares(shares) {}
};
using priority_class_ptr = lw_shared_ptr<priority_class>;


class fair_queue {
    friend priority_class;
    struct class_compare {
        bool operator() (const priority_class_ptr& lhs, const priority_class_ptr& rhs) const {
            return lhs->_accumulated > rhs->_accumulated;
        }
    };
    semaphore _sem;
    unsigned _capacity;
    using clock_type = std::chrono::steady_clock::time_point;
    clock_type _base;
    std::chrono::microseconds _tau;
    using prioq = std::priority_queue<priority_class_ptr, std::vector<priority_class_ptr>, class_compare>;
    prioq _handles;
    std::unordered_set<priority_class_ptr> _all_classes;
    void push_priority_class(priority_class_ptr pc) {
        if (!pc->_queued) {
            _handles.push(pc);
            pc->_queued = true;
        }
    }
    priority_class_ptr pop_priority_class() {
        assert(!_handles.empty());
        auto h = _handles.top();
        _handles.pop();
        assert(h->_queued);
        h->_queued = false;
        return h;
    }
    void execute_one() {
        _sem.wait().then([this] {
            priority_class_ptr h;
            do {
                h = pop_priority_class();
            } while (h->_queue.empty());

            auto req = std::move(h->_queue.front());
            h->_queue.pop_front();
            req.pr.set_value();
            auto delta = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - _base);
            auto req_cost  = float(req.weight) / h->_shares;
            auto cost  = expf(1.0f/_tau.count() * delta.count()) * req_cost;
            float next_accumulated = h->_accumulated + cost;
            while (std::isinf(next_accumulated)) {
                normalize_stats();
                // If we have renormalized, our time base will have changed. This should happen very infrequently
                delta = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - _base);
                cost  = expf(1.0f/_tau.count() * delta.count()) * req_cost;
                next_accumulated = h->_accumulated + cost;
            }
            h->_accumulated = next_accumulated;
            if (!h->_queue.empty()) {
                push_priority_class(h);
            }
            return make_ready_future<>();
        });
    }
    float normalize_factor() const {
        return std::numeric_limits<float>::min();
    }
    void normalize_stats() {
        auto time_delta = std::log(normalize_factor()) * _tau;
        // time_delta is negative; and this may advance _base into the future
        _base -= std::chrono::duration_cast<clock_type::duration>(time_delta);
        for (auto& pc: _all_classes) {
            pc->_accumulated *= normalize_factor();
        }
    }
public:
    explicit fair_queue(unsigned capacity, std::chrono::microseconds tau = std::chrono::milliseconds(100))
                                           : _sem(capacity)
                                           , _capacity(capacity)
                                           , _base(std::chrono::steady_clock::now())
                                           , _tau(tau) {
    }
    priority_class_ptr register_priority_class(uint32_t shares) {
        priority_class_ptr pclass = make_lw_shared<priority_class>(shares);
        _all_classes.insert(pclass);
        return pclass;
    }
    void unregister_priority_class(priority_class_ptr pclass) {
        assert(pclass->_queue.empty());
        _all_classes.erase(pclass);
    }
    size_t waiters() const {
        return _sem.waiters();
    }

    template <typename Func>
    futurize_t<std::result_of_t<Func()>> queue(priority_class_ptr pc, unsigned weight, Func func) {
        // We need to return a future in this function on which the caller can wait.
        // Since we don't know which queue we will use to execute the next request - if ours or
        // someone else's, we need a separate promise at this point.
        promise<> pr;
        auto fut = pr.get_future();

        push_priority_class(pc);
        pc->_queue.push_back(priority_class::request{std::move(pr), weight});
        try {
            execute_one();
        } catch (...) {
            pc->_queue.pop_back();
            throw;
        }
        return fut.then([func = std::move(func)] {
            return func();
        }).finally([this] {
            _sem.signal();
        });
    }

    /// Updates the current shares of this priority class
    ///
    /// \param new_shares the new number of shares for this priority class
    static void update_shares(priority_class_ptr pc, uint32_t new_shares) {
        pc->_shares = new_shares;
    }
};

class io_queue;
class io_priority_class {
    unsigned val;
    friend io_queue;
public:
    unsigned id() const {
        return val;
    }
};

class smp;
class io_queue {
private:
    shard_id _coordinator;
    size_t _capacity;
    std::vector<shard_id> _io_topology;
    struct priority_class_data {
        priority_class_ptr ptr;
        size_t bytes;
        uint64_t ops;
        uint32_t nr_queued;
        std::chrono::duration<double> queue_time;
        // metrics::metric_groups _metric_groups;
        priority_class_data(std::string name, priority_class_ptr ptr, shard_id owner);
    };
    std::unordered_map<unsigned, lw_shared_ptr<priority_class_data>> _priority_classes;
    fair_queue _fq;
    static constexpr unsigned _max_classes = 1024;
    static std::array<std::atomic<uint32_t>, _max_classes> _registered_shares;
    static std::array<std::string, _max_classes> _registered_names;
    static io_priority_class register_one_priority_class(std::string name, uint32_t shares);
    priority_class_data& find_or_create_class(const io_priority_class& pc, shard_id owner);
    static void fill_shares_array();
    friend smp;
public:
    io_queue(shard_id coordinator, size_t capacity, std::vector<shard_id> topology);
    ~io_queue();
    template <typename Func>
    static future<io_event>
    queue_request(shard_id coordinator, const io_priority_class& pc, size_t len, Func do_io);
    size_t capacity() const {
        return _capacity;
    }
    size_t queued_requests() const {
        return _fq.waiters();
    }

    shard_id coordinator() const {
        return _coordinator;
    }
    shard_id coordinator_of_shard(shard_id shard) const {
        return _io_topology[shard];
    }
    friend class reactor;
};
std::array<std::atomic<uint32_t>, io_queue::_max_classes> io_queue::_registered_shares;
std::array<std::string, io_queue::_max_classes> io_queue::_registered_names;
io_queue::io_queue(shard_id coordinator, size_t capacity, std::vector<shard_id> topology)
        : _coordinator(coordinator)
        , _capacity(capacity)
        , _io_topology(std::move(topology))
        , _priority_classes()
        , _fq(capacity) {
}

io_queue::~io_queue() {
    // It is illegal to stop the I/O queue with pending requests.
    // Technically we would use a gate to guarantee that. But here, it is not
    // needed since this is expected to be destroyed only after the reactor is destroyed.
    //
    // And that will happen only when there are no more fibers to run. If we ever change
    // that, then this has to change.
    for (auto&& pclasses: _priority_classes) {
        _fq.unregister_priority_class(pclasses.second->ptr);
    }
}


void io_queue::fill_shares_array() {
    for (unsigned i = 0; i < _max_classes; ++i) {
        _registered_shares[i].store(0);
    }
}

io_priority_class io_queue::register_one_priority_class(std::string name, uint32_t shares) {
    for (unsigned i = 0; i < _max_classes; ++i) {
        uint32_t unused = 0;
        auto s = _registered_shares[i].compare_exchange_strong(unused, shares, std::memory_order_acq_rel);
        if (s) {
            io_priority_class p;
            _registered_names[i] = name;
            p.val = i;
            return std::move(p);
        };
    }
    throw std::runtime_error("No more room for new I/O priority classes");
}

// seastar::metrics::label io_queue_shard("ioshard");

io_queue::priority_class_data::priority_class_data(std::string name, priority_class_ptr ptr, shard_id owner)
    : ptr(ptr)
    , bytes(0)
    , ops(0)
    , nr_queued(0)
    , queue_time(1s){}

io_queue::priority_class_data& io_queue::find_or_create_class(const io_priority_class& pc, shard_id owner) {
    auto it_pclass = _priority_classes.find(pc.id());
    if (it_pclass == _priority_classes.end()) {
        auto shares = _registered_shares.at(pc.id()).load(std::memory_order_acquire);
        auto name = _registered_names.at(pc.id());
        auto ret = _priority_classes.emplace(pc.id(), make_lw_shared<priority_class_data>(name, _fq.register_priority_class(shares), owner));
        it_pclass = ret.first;
    }
    return *(it_pclass->second);
}
/*---------------------------------------------------socket相关----------------------------------------------------------------*/

static inline
bool is_ip_unspecified(ipv4_addr &addr) {
    return addr.ip == 0;
}

static inline
bool is_port_unspecified(ipv4_addr &addr) {
    return addr.port == 0;
}

static inline
std::ostream& operator<<(std::ostream &os, ipv4_addr addr) {
}

static inline
socket_address make_ipv4_address(ipv4_addr addr) {
    socket_address sa;
    sa.u.in.sin_family = AF_INET;
    sa.u.in.sin_port = htons(addr.port);
    sa.u.in.sin_addr.s_addr = htonl(addr.ip);
    return sa;
}
inline
socket_address make_ipv4_address(uint32_t ip, uint16_t port) {
    socket_address sa;
    sa.u.in.sin_family = AF_INET;
    sa.u.in.sin_port = htons(port);
    sa.u.in.sin_addr.s_addr = htonl(ip);
    return sa;
}

namespace net {
    
// see linux tcp(7) for parameter explanation
struct tcp_keepalive_params {
    std::chrono::seconds idle; // TCP_KEEPIDLE
    std::chrono::seconds interval; // TCP_KEEPINTVL
    unsigned count; // TCP_KEEPCNT
};

// see linux sctp(7) for parameter explanation
struct sctp_keepalive_params {
    std::chrono::seconds interval; // spp_hbinterval
    unsigned count; // spp_pathmaxrt
};

using keepalive_params = boost::variant<tcp_keepalive_params, sctp_keepalive_params>;

/// \cond internal
class connected_socket_impl;
class socket_impl;
class server_socket_impl;
class udp_channel_impl;
class get_impl;
/// \endcond

class udp_datagram_impl {
public:
    virtual ~udp_datagram_impl() {};
    virtual ipv4_addr get_src() = 0;
    virtual ipv4_addr get_dst() = 0;
    virtual uint16_t get_dst_port() = 0;
    virtual packet& get_data() = 0;
};

class udp_datagram final {
private:
    std::unique_ptr<udp_datagram_impl> _impl;
public:
    udp_datagram(std::unique_ptr<udp_datagram_impl>&& impl) : _impl(std::move(impl)) {};
    ipv4_addr get_src() { return _impl->get_src(); }
    ipv4_addr get_dst() { return _impl->get_dst(); }
    uint16_t get_dst_port() { return _impl->get_dst_port(); }
    packet& get_data() { return _impl->get_data(); }
};

class udp_channel {
private:
    std::unique_ptr<udp_channel_impl> _impl;
public:
    udp_channel();
    udp_channel(std::unique_ptr<udp_channel_impl>);
    ~udp_channel();

    udp_channel(udp_channel&&);
    udp_channel& operator=(udp_channel&&);

    future<udp_datagram> receive();
    future<> send(ipv4_addr dst, const char* msg);
    future<> send(ipv4_addr dst, packet p);
    bool is_closed() const;
    void close();
};
}


class connected_socket {
    friend class net::get_impl;
    std::unique_ptr<net::connected_socket_impl> _csi;
public:
    connected_socket();
    ~connected_socket();
    explicit connected_socket(std::unique_ptr<net::connected_socket_impl> csi);
    connected_socket(connected_socket&& cs) noexcept;
    connected_socket& operator=(connected_socket&& cs) noexcept;
    input_stream<char> input();
    output_stream<char> output(size_t buffer_size = 8192);
    void set_nodelay(bool nodelay);
    bool get_nodelay() const;
    void set_keepalive(bool keepalive);
    bool get_keepalive() const;
    void set_keepalive_parameters(const net::keepalive_params& p);
    net::keepalive_params get_keepalive_parameters() const;
    void shutdown_output();
    void shutdown_input();
};


class socket {
    std::unique_ptr<::net::socket_impl> _si;
public:
    ~socket();
    explicit socket(std::unique_ptr<::net::socket_impl> si);
    socket(socket&&) noexcept;
    seastar::socket& operator=(seastar::socket&&) noexcept;
    future<connected_socket> connect(socket_address sa, socket_address local = socket_address(::sockaddr_in{AF_INET, INADDR_ANY, {0}}), seastar::transport proto = seastar::transport::TCP);
    void shutdown();
};

class server_socket {
    std::unique_ptr<net::server_socket_impl> _ssi;
public:
    server_socket();
    explicit server_socket(std::unique_ptr<net::server_socket_impl> ssi);
    server_socket(server_socket&& ss) noexcept;
    ~server_socket();
    /// Move-assigns a \c server_socket object.
    server_socket& operator=(server_socket&& cs) noexcept;
    future<connected_socket, socket_address> accept();
    void abort_accept();
};

class network_stack {
public:
    virtual ~network_stack() {}
    virtual server_socket listen(socket_address sa, listen_options opts) = 0;
    // FIXME: local parameter assumes ipv4 for now, fix when adding other AF
    future<connected_socket> connect(socket_address sa, socket_address local = socket_address(::sockaddr_in{AF_INET, INADDR_ANY, {0}}), seastar::transport proto = seastar::transport::TCP) {
        return socket().connect(sa, local, proto);
    }
    virtual seastar::socket socket() = 0;
    virtual ::net::udp_channel make_udp_channel(ipv4_addr addr = {}) = 0;
    virtual future<> initialize() {
        return make_ready_future();
    }
    virtual bool has_per_core_namespace() = 0;
};




/*-------------------------------------------------reactor类定义----------------------------------------------------------------*/
#include "../resource/resource.hh"
#include <boost/thread/barrier.hpp>
#include <boost/range/irange.hpp>
#include <sys/syscall.h>

struct pollfn {
        virtual ~pollfn() {}
        // Returns true if work was done (false = idle)
        virtual bool poll() = 0;
        // Checks if work needs to be done, but without actually doing any
        // returns true if works needs to be done (false = idle)
        virtual bool pure_poll() = 0;
        // Tries to enter interrupt mode.
        //
        // If it returns true, then events from this poller will wake
        // a sleeping idle loop, and exit_interrupt_mode() must be called
        // to return to normal polling.
        //
        // If it returns false, the sleeping idle loop may not be entered.
        virtual bool try_enter_interrupt_mode() { return false; }
        virtual void exit_interrupt_mode() {}
};




struct reactor {
/*---------构造函数和析构函数----------------*/
    // reactor();
    reactor(unsigned int id);
    ~reactor();
    reactor(const reactor&) = delete;
    reactor& operator=(const reactor&) = delete;
    unsigned _id = 0;
    std::deque<double> _loads;
    double _load = 0;
/*----------定时器相关------------------------*/
    steady_clock_type::duration _total_idle;
    std::unique_ptr<lowres_clock> _lowres_clock;
    lowres_clock::time_point _lowres_next_timeout;
    timer_t _steady_clock_timer = {};
    timer_set<timer<steady_clock_type>> _timers;
    typename timer_set<timer<steady_clock_type>>::timer_list_t _expired_timers;
    timer_set<timer<lowres_clock>> _lowres_timers;
    typename timer_set<timer<lowres_clock>>::timer_list_t _expired_lowres_timers;
    timer_set<timer<manual_clock>> _manual_timers;
    typename timer_set<timer<manual_clock>>::timer_list_t _expired_manual_timers;
    using steady_timer = timer<steady_clock_type>;
    using lowres_timer = timer<lowres_clock>;
    using manual_timer = timer<manual_clock>;
    file_desc _task_quota_timer;
    std::optional<pollable_fd> _aio_eventfd;
/*---------------------------------------------*/
    void add_timer(steady_timer* tmr);
    bool queue_timer(steady_timer* tmr);
    void del_timer(steady_timer* tmr);
    void add_timer(lowres_timer* tmr);
    bool queue_timer(lowres_timer* tmr);
    void del_timer(lowres_timer* tmr);
    void add_timer(manual_timer* tmr);
    bool queue_timer(manual_timer* tmr);
    void del_timer(manual_timer* tmr);
    void enable_timer(steady_clock_type::time_point when);
    bool do_expire_lowres_timers();
    template <typename T, typename E, typename EnableFunc>
    void complete_timers(T&, E&, EnableFunc&& enable_fn);
    bool do_check_lowres_timers() const;
    void expire_manual_timers();
/*---------------信号处理相关------------------------*/
    signals _signals;
    bool _handle_sigint = true;
    void block_notifier(int); 
/*----------任务相关-------------------*/
    bool _stopping = false;
    bool _stopped = false;
    int _return = 0;
    unsigned _tasks_processed_report_threshold;
    std::chrono::duration<double> _task_quota;
    condition_variable _stop_requested;
    std::atomic<bool> _sleeping alignas(64);
    std::deque<std::unique_ptr<task>> _pending_tasks;
    void run_tasks(std::deque<std::unique_ptr<task>>& tasks);
    std::vector<std::function<future<> ()>> _exit_funcs;//为什么不用引用?
    void add_task(std::unique_ptr<task>&& t) { _pending_tasks.push_back(std::move(t)); }
    void add_urgent_task(std::unique_ptr<task>&& t) { _pending_tasks.push_front(std::move(t)); }
    void add_high_priority_task(std::unique_ptr<task>&& t){
            _pending_tasks.push_front(std::move(t));
            // break .then() chains
            g_need_preempt = true;
    }
    void force_poll() {
        g_need_preempt = true;
    }
    void at_exit(std::function<future<> ()> func);
    void exit(int ret);
    void stop();
    future<> run_exit_tasks();
    template <typename Func>
    future<io_event> submit_io(Func prepare_io);
    semaphore _io_context_available;
    static constexpr size_t max_aio = 128;
    semaphore _cpu_started;
    io_context_t _io_context;
    promise<> _start_promise;
    future<> when_started() { return _start_promise.get_future(); }
    /*----------------全局--------------*/
    int run();
    /*----------配置相关----------------*/
    static boost::program_options::options_description get_options_description();
    void configure(boost::program_options::variables_map config);
   /*----------------------资源分配相关-----------------------*/
    shard_id _io_coordinator;
    io_queue* _io_queue;
    std::unique_ptr<io_queue> my_io_queue = {};
    pthread_t _thread_id alignas(64) = pthread_self();
    shard_id cpu_id() const { return _id; }
    void wakeup() { pthread_kill(_thread_id, alarm_signal());}
    std::chrono::nanoseconds calculate_poll_time();
    /*-----------其他---------------*/
    unsigned _max_task_backlog = 1000;
    std::chrono::nanoseconds _max_poll_time = calculate_poll_time();
    bool _strict_o_direct = true;
    void set_strict_dma(bool value) {
        _strict_o_direct = value;
    }
    /*----------------------poller相关----------------------------------------------------*/
    std::vector<pollfn*> _pollers;
    thread_pool _thread_pool;
    
    void unregister_poller(pollfn* p);
    void register_poller(pollfn* p);
    struct poller {
        std::unique_ptr<pollfn> _pollfn;
        class registration_task;
        class deregistration_task;
        registration_task* _registration_task;
    public:
        template <typename Func> // signature: bool ()
        static poller simple(Func&& poll) {
            return poller(make_pollfn(std::forward<Func>(poll)));
        }
        poller(std::unique_ptr<pollfn> fn)
                : _pollfn(std::move(fn)) {
            do_register();
        }
        ~poller();
        poller(poller&& x);
        poller& operator=(poller&& x);
        void do_register();
        friend class reactor;
    };
    class io_pollfn;
    class signal_pollfn;
    class aio_batch_submit_pollfn;
    class batch_flush_pollfn;
    class smp_pollfn;
    class drain_cross_cpu_freelist_pollfn;
    class lowres_timer_pollfn;
    class manual_timer_pollfn;
    class epoll_pollfn;
    class syscall_pollfn;
    class execution_stage_pollfn;
    bool poll_once();
    bool pure_poll_once();
    /*----------------------------------IO相关--------------------------------------------*/
    void start_aio_eventfd_loop();
    server_socket listen(socket_address sa, listen_options opts = {});
    future<connected_socket> connect(socket_address sa);
    future<connected_socket> connect(socket_address, socket_address, transport proto = transport::TCP);
    pollable_fd posix_listen(socket_address sa, listen_options opts = {});
    bool posix_reuseport_available() const { return _reuseport; }
    lw_shared_ptr<pollable_fd> make_pollable_fd(socket_address sa, transport proto = transport::TCP);
    future<> posix_connect(lw_shared_ptr<pollable_fd> pfd, socket_address sa, socket_address local);
    future<pollable_fd, socket_address> accept(pollable_fd_state& listen_fd);
    future<size_t> read_some(pollable_fd_state& fd, void* buffer, size_t size);
    future<size_t> read_some(pollable_fd_state& fd, const std::vector<iovec>& iov);
    future<size_t> write_some(pollable_fd_state& fd, const void* buffer, size_t size);
    future<> write_all(pollable_fd_state& fd, const void* buffer, size_t size);
    future<file> open_file_dma(std::string name, open_flags flags, file_open_options options = {});
    future<file> open_directory(std::string name);
    future<> make_directory(std::string name);
    future<> touch_directory(std::string name);
    future<std::optional<directory_entry_type>> cfile_type(std::string name);
    future<uint64_t> file_size(std::string pathname);
    future<bool> file_exists(std::string pathname);
    future<fs_type> file_system_at(std::string pathname);
    future<> remove_file(std::string pathname);
    future<> rename_file(std::string old_pathname, std::string new_pathname);
    future<> link_file(std::string oldpath, std::string newpath);
    // In the following three methods, prepare_io is not guaranteed to execute in the same processor
    // in which it was generated. Therefore, care must be taken to avoid the use of objects that could
    // be destroyed within or at exit of prepare_io.
    template <typename Func>
    future<io_event> submit_io(Func prepare_io);
    template <typename Func>
    future<io_event> submit_io_read(const io_priority_class& priority_class, size_t len, Func prepare_io);
    template <typename Func>
    future<io_event> submit_io_write(const io_priority_class& priority_class, size_t len, Func prepare_io);
    /*-------------------------------------网络相关--------------------------------------------------------------------*/
    const bool _reuseport;
    std::unique_ptr<network_stack> _network_stack;
    promise<std::unique_ptr<network_stack>> _network_stack_ready_promise;
};

bool
reactor::pure_poll_once() {
    for (auto c : _pollers) {
        if (c->pure_poll()) {
            return true;
        }
    }
    return false;
}

class pollable_fd_state {
public:
    struct speculation {
        int events = 0;
        explicit speculation(int epoll_events_guessed = 0) : events(epoll_events_guessed) {}
    };
    ~pollable_fd_state();
    explicit pollable_fd_state(file_desc fd, speculation speculate = speculation())
        : fd(std::move(fd)), events_known(speculate.events) {}
    pollable_fd_state(const pollable_fd_state&) = delete;
    void operator=(const pollable_fd_state&) = delete;
    void speculate_epoll(int events) { events_known |= events; }
    file_desc fd;
    int events_requested = 0; // wanted by pollin/pollout promises
    int events_epoll = 0;     // installed in epoll
    int events_known = 0;     // returned from epoll
    promise<> pollin;
    promise<> pollout;
    friend class reactor;
    friend class pollable_fd;
};


class pollable_fd {
public:
    using speculation = pollable_fd_state::speculation;
    pollable_fd(file_desc fd, speculation speculate = speculation())
        : _s(std::make_unique<pollable_fd_state>(std::move(fd), speculate)) {}
public:
    pollable_fd(pollable_fd&&) = default;
    pollable_fd& operator=(pollable_fd&&) = default;
    future<size_t> read_some(char* buffer, size_t size);
    future<size_t> read_some(uint8_t* buffer, size_t size);
    future<size_t> read_some(const std::vector<iovec>& iov);
    future<> write_all(const char* buffer, size_t size);
    future<> write_all(const uint8_t* buffer, size_t size);
    future<size_t> write_some(net::packet& p);
    future<> write_all(net::packet& p);
    future<> readable();
    future<> writeable();
    void abort_reader(std::exception_ptr ex);
    void abort_writer(std::exception_ptr ex);
    future<pollable_fd, socket_address> accept();
    future<size_t> sendmsg(struct msghdr *msg);
    future<size_t> recvmsg(struct msghdr *msg);
    future<size_t> sendto(socket_address addr, const void* buf, size_t len);
    file_desc& get_file_desc() const { return _s->fd; }
    void shutdown(int how) { _s->fd.shutdown(how); }
    void close() { _s.reset(); }
protected:
    int get_fd() const { return _s->fd.get(); }
    friend class reactor;
    friend class readable_eventfd;
    friend class writeable_eventfd;
private:
    std::unique_ptr<pollable_fd_state> _s;
};


class readable_eventfd {
    pollable_fd _fd;
public:
    explicit readable_eventfd(size_t initial = 0) : _fd(try_create_eventfd(initial)) {}
    readable_eventfd(readable_eventfd&&) = default;
    writeable_eventfd write_side();
    future<size_t> wait();
    int get_write_fd() { return _fd.get_fd(); }
private:
    explicit readable_eventfd(file_desc&& fd) : _fd(std::move(fd)) {}
    static file_desc try_create_eventfd(size_t initial);

    friend class writeable_eventfd;
};

class writeable_eventfd {
    file_desc _fd;
public:
    explicit writeable_eventfd(size_t initial = 0) : _fd(try_create_eventfd(initial)) {}
    writeable_eventfd(writeable_eventfd&&) = default;
    readable_eventfd read_side();
    void signal(size_t nr);
    int get_read_fd() { return _fd.get(); }
private:
    explicit writeable_eventfd(file_desc&& fd) : _fd(std::move(fd)) {}
    static file_desc try_create_eventfd(size_t initial);

    friend class readable_eventfd;
};

void reactor::register_poller(pollfn* p) {
    _pollers.push_back(p);
}

void reactor::unregister_poller(pollfn* p) {
    _pollers.erase(std::find(_pollers.begin(), _pollers.end(), p));
}

class reactor::poller::registration_task : public task {
private:
    poller* _p;
public:
    explicit registration_task(poller* p) : _p(p) {}
    virtual void run() noexcept override {
        if (_p) {
            engine().register_poller(_p->_pollfn.get());
            _p->_registration_task = nullptr;
        }
    }
    void cancel() {
        _p = nullptr;
    }
    void moved(poller* p) {
        _p = p;
    }
};

class reactor::poller::deregistration_task : public task {
private:
    std::unique_ptr<pollfn> _p;
public:
    explicit deregistration_task(std::unique_ptr<pollfn>&& p) : _p(std::move(p)) {}
    virtual void run() noexcept override {
        engine().unregister_poller(_p.get());
    }
};


class reactor::io_pollfn final : public pollfn {
    reactor& _r;
public:
    io_pollfn(reactor& r) : _r(r) {}
    virtual bool poll() override final {
        return _r.process_io();
    }
    virtual bool pure_poll() override final {
        return poll(); // actually performs work, but triggers no user continuations, so okay
    }
    virtual bool try_enter_interrupt_mode() override {
        // aio cannot generate events if there are no inflight aios;
        // but if we enabled _aio_eventfd, we can always enter
        return _r._io_context_available.current() == reactor::max_aio
                || _r._aio_eventfd;
    }
    virtual void exit_interrupt_mode() override {
        // nothing to do
    }
};


class reactor::signal_pollfn final : public pollfn {
    reactor& _r;
public:
    signal_pollfn(reactor& r) : _r(r) {}
    virtual bool poll() final override {
        return _r._signals.poll_signal();
    }
    virtual bool pure_poll() override final {
        return _r._signals.pure_poll_signal();
    }
    virtual bool try_enter_interrupt_mode() override {
        // Signals will interrupt our epoll_pwait() call, but
        // disable them now to avoid a signal between this point
        // and epoll_pwait()
        sigset_t block_all;
        sigfillset(&block_all);
        ::pthread_sigmask(SIG_SETMASK, &block_all, &_r._active_sigmask);
        if (poll()) {
            // raced already, and lost
            exit_interrupt_mode();
            return false;
        }
        return true;
    }
    virtual void exit_interrupt_mode() override final {
        ::pthread_sigmask(SIG_SETMASK, &_r._active_sigmask, nullptr);
    }
};

class reactor::batch_flush_pollfn final : public pollfn {
    reactor& _r;
public:
    batch_flush_pollfn(reactor& r) : _r(r) {}
    virtual bool poll() final override {
        return _r.flush_tcp_batches();
    }
    virtual bool pure_poll() override final {
        return poll(); // actually performs work, but triggers no user continuations, so okay
    }
    virtual bool try_enter_interrupt_mode() override {
        // This is a passive poller, so if a previous poll
        // returned false (idle), there's no more work to do.
        return true;
    }
    virtual void exit_interrupt_mode() override final {

    }
};

class reactor::aio_batch_submit_pollfn final : public pollfn {
    reactor& _r;
public:
    aio_batch_submit_pollfn(reactor& r) : _r(r) {}
    virtual bool poll() final override {
        return _r.flush_pending_aio();
    }
    virtual bool pure_poll() override final {
        return poll(); // actually performs work, but triggers no user continuations, so okay
    }
    virtual bool try_enter_interrupt_mode() override {
        // This is a passive poller, so if a previous poll
        // returned false (idle), there's no more work to do.
        return true;
    }
    virtual void exit_interrupt_mode() override final {
    }
};

class reactor::drain_cross_cpu_freelist_pollfn final : public pollfn {
public:
    virtual bool poll() final override {
        return memory::drain_cross_cpu_freelist();
    }
    virtual bool pure_poll() override final {
        return poll(); // actually performs work, but triggers no user continuations, so okay
    }
    virtual bool try_enter_interrupt_mode() override {
        // Other cpus can queue items for us to free; and they won't notify
        // us about them.  But it's okay to ignore those items, freeing them
        // doesn't have any side effects.
        //
        // We'll take care of those items when we wake up for another reason.
        return true;
    }
    virtual void exit_interrupt_mode() override final {
    }
};

class reactor::lowres_timer_pollfn final : public pollfn {
    reactor& _r;
    // A highres timer is implemented as a waking  signal; so
    // we arm one when we have a lowres timer during sleep, so
    // it can wake us up.
    timer<> _nearest_wakeup { [this] { _armed = false; } };
    bool _armed = false;
public:
    lowres_timer_pollfn(reactor& r) : _r(r) {}
    virtual bool poll() final override {
        return _r.do_expire_lowres_timers();
    }
    virtual bool pure_poll() final override {
        return _r.do_check_lowres_timers();
    }
    virtual bool try_enter_interrupt_mode() override {
        // arm our highres timer so a signal will wake us up
        auto next = _r._lowres_next_timeout;
        if (next == lowres_clock::time_point()) {
            // no pending timers
            return true;
        }
        auto now = lowres_clock::now();
        if (next <= now) {
            // whoops, go back
            return false;
        }
        _nearest_wakeup.arm(next - now);
        _armed = true;
        return true;
    }
    virtual void exit_interrupt_mode() override final {
        if (_armed) {
            _nearest_wakeup.cancel();
            _armed = false;
        }
    }
};

class reactor::smp_pollfn final : public pollfn {
    reactor& _r;
    struct aligned_flag {
        std::atomic<bool> flag;
        char pad[63];
        bool try_lock() {
            return !flag.exchange(true, std::memory_order_relaxed);
        }
        void unlock() {
            flag.store(false, std::memory_order_relaxed);
        }
    };
    static aligned_flag _membarrier_lock;
public:
    smp_pollfn(reactor& r) : _r(r) {}
    virtual bool poll() final override {
        return smp::poll_queues();
    }
    virtual bool pure_poll() final override {
        return smp::pure_poll_queues();
    }
    virtual bool try_enter_interrupt_mode() override {
        // systemwide_memory_barrier() is very slow if run concurrently,
        // so don't go to sleep if it is running now.
        if (!_membarrier_lock.try_lock()) {
            return false;
        }
        _r._sleeping.store(true, std::memory_order_relaxed);
        systemwide_memory_barrier();
        _membarrier_lock.unlock();
        if (poll()) {
            // raced
            _r._sleeping.store(false, std::memory_order_relaxed);
            return false;
        }
        return true;
    }
    virtual void exit_interrupt_mode() override final {
        _r._sleeping.store(false, std::memory_order_relaxed);
    }
};

class reactor::execution_stage_pollfn final : public pollfn {
    internal::execution_stage_manager& _esm;
public:
    execution_stage_pollfn() : _esm(internal::execution_stage_manager::get()) { }

    virtual bool poll() override {
        return _esm.flush();
    }
    virtual bool pure_poll() override {
        return _esm.poll();
    }
    virtual bool try_enter_interrupt_mode() override {
        // This is a passive poller, so if a previous poll
        // returned false (idle), there's no more work to do.
        return true;
    }
    virtual void exit_interrupt_mode() override { }
};


class reactor::syscall_pollfn final : public pollfn {
    reactor& _r;
public:
    syscall_pollfn(reactor& r) : _r(r) {}
    virtual bool poll() final override {
        return _r._thread_pool.complete();
    }
    virtual bool pure_poll() override final {
        return poll(); // actually performs work, but triggers no user continuations, so okay
    }
    virtual bool try_enter_interrupt_mode() override {
        _r._thread_pool.enter_interrupt_mode();
        if (poll()) {
            // raced
            _r._thread_pool.exit_interrupt_mode();
            return false;
        }
        return true;
    }
    virtual void exit_interrupt_mode() override final {
        _r._thread_pool.exit_interrupt_mode();
    }
};


// alignas(64) reactor::smp_pollfn::aligned_flag reactor::smp_pollfn::_membarrier_lock;

class reactor::epoll_pollfn final : public pollfn {
    reactor& _r;
public:
    epoll_pollfn(reactor& r) : _r(r) {}
    virtual bool poll() final override {
        return _r.wait_and_process();
    }
    virtual bool pure_poll() override final {
        return poll(); // actually performs work, but triggers no user continuations, so okay
    }
    virtual bool try_enter_interrupt_mode() override {
        // Since we'll be sleeping in epoll, no need to do anything
        // for interrupt mode.
        return true;
    }
    virtual void exit_interrupt_mode() override final {
    }
};



inline
future<size_t> pollable_fd::read_some(char* buffer, size_t size) {
    return engine().read_some(*_s, buffer, size);
}

inline
future<size_t> pollable_fd::read_some(uint8_t* buffer, size_t size) {
    return engine().read_some(*_s, buffer, size);
}

inline
future<size_t> pollable_fd::read_some(const std::vector<iovec>& iov) {
    return engine().read_some(*_s, iov);
}

inline
future<> pollable_fd::write_all(const char* buffer, size_t size) {
    return engine().write_all(*_s, buffer, size);
}

inline
future<> pollable_fd::write_all(const uint8_t* buffer, size_t size) {
    return engine().write_all(*_s, buffer, size);
}

inline
future<size_t> pollable_fd::write_some(net::packet& p) {
    return engine().writeable(*_s).then([this, &p] () mutable {
        static_assert(offsetof(iovec, iov_base) == offsetof(net::fragment, base) &&
            sizeof(iovec::iov_base) == sizeof(net::fragment::base) &&
            offsetof(iovec, iov_len) == offsetof(net::fragment, size) &&
            sizeof(iovec::iov_len) == sizeof(net::fragment::size) &&
            alignof(iovec) == alignof(net::fragment) &&
            sizeof(iovec) == sizeof(net::fragment)
            , "net::fragment and iovec should be equivalent");

        iovec* iov = reinterpret_cast<iovec*>(p.fragment_array());
        msghdr mh = {};
        mh.msg_iov = iov;
        mh.msg_iovlen = p.nr_frags();
        auto r = get_file_desc().sendmsg(&mh, MSG_NOSIGNAL);
        if (!r) {
            return write_some(p);
        }
        if (size_t(*r) == p.len()) {
            _s->speculate_epoll(EPOLLOUT);
        }
        return make_ready_future<size_t>(*r);
    });
}

inline
future<> pollable_fd::write_all(net::packet& p) {
    return write_some(p).then([this, &p] (size_t size) {
        if (p.len() == size) {
            return make_ready_future<>();
        }
        p.trim_front(size);
        return write_all(p);
    });
}

inline
future<> pollable_fd::readable() {
    return engine().readable(*_s);
}

inline
future<> pollable_fd::writeable() {
    return engine().writeable(*_s);
}

inline
void
pollable_fd::abort_reader(std::exception_ptr ex) {
    engine().abort_reader(*_s, std::move(ex));
}

inline
void
pollable_fd::abort_writer(std::exception_ptr ex) {
    engine().abort_writer(*_s, std::move(ex));
}

inline
future<pollable_fd, socket_address> pollable_fd::accept() {
    return engine().accept(*_s);
}

inline
future<size_t> pollable_fd::recvmsg(struct msghdr *msg) {
    return engine().readable(*_s).then([this, msg] {
        auto r = get_file_desc().recvmsg(msg, 0);
        if (!r) {
            return recvmsg(msg);
        }
        // We always speculate here to optimize for throughput in a workload
        // with multiple outstanding requests. This way the caller can consume
        // all messages without resorting to epoll. However this adds extra
        // recvmsg() call when we hit the empty queue condition, so it may
        // hurt request-response workload in which the queue is empty when we
        // initially enter recvmsg(). If that turns out to be a problem, we can
        // improve speculation by using recvmmsg().
        _s->speculate_epoll(EPOLLIN);
        return make_ready_future<size_t>(*r);
    });
};

inline
future<size_t> pollable_fd::sendmsg(struct msghdr* msg) {
    return engine().writeable(*_s).then([this, msg] () mutable {
        auto r = get_file_desc().sendmsg(msg, 0);
        if (!r) {
            return sendmsg(msg);
        }
        // For UDP this will always speculate. We can't know if there's room
        // or not, but most of the time there should be so the cost of mis-
        // speculation is amortized.
        if (size_t(*r) == iovec_len(msg->msg_iov, msg->msg_iovlen)) {
            _s->speculate_epoll(EPOLLOUT);
        }
        return make_ready_future<size_t>(*r);
    });
}

inline
future<size_t> pollable_fd::sendto(socket_address addr, const void* buf, size_t len) {
    return engine().writeable(*_s).then([this, buf, len, addr] () mutable {
        auto r = get_file_desc().sendto(addr, buf, len, 0);
        if (!r) {
            return sendto(std::move(addr), buf, len);
        }
        // See the comment about speculation in sendmsg().
        if (size_t(*r) == len) {
            _s->speculate_epoll(EPOLLOUT);
        }
        return make_ready_future<size_t>(*r);
    });
}


void reactor::start_aio_eventfd_loop() {
    if (!_aio_eventfd) {
        return;
    }
    future<> loop_done = repeat([this] {
        return _aio_eventfd->readable().then([this] {
            char garbage[8];
            ::read(_aio_eventfd->get_fd(), garbage, 8); // totally uninteresting
            return _stopping ? stop_iteration::yes : stop_iteration::no;
        });
    });
    // must use make_lw_shared, because at_exit expects a copyable function
    at_exit([loop_done = make_lw_shared(std::move(loop_done))] {
        return std::move(*loop_done);
    });
}

/* not yet implemented for OSv. TODO: do the notification like we do class smp. */


readable_eventfd writeable_eventfd::read_side() {
    return readable_eventfd(_fd.dup());
}

file_desc writeable_eventfd::try_create_eventfd(size_t initial) {
    assert(size_t(int(initial)) == initial);
    return file_desc::eventfd(initial, EFD_CLOEXEC);
}

void writeable_eventfd::signal(size_t count) {
    uint64_t c = count;
    auto r = _fd.write(&c, sizeof(c));
    assert(r == sizeof(c));
}

writeable_eventfd readable_eventfd::write_side() {
    return writeable_eventfd(_fd.get_file_desc().dup());
}

file_desc readable_eventfd::try_create_eventfd(size_t initial) {
    assert(size_t(int(initial)) == initial);
    return file_desc::eventfd(initial, EFD_CLOEXEC | EFD_NONBLOCK);
}

future<size_t> readable_eventfd::wait() {
    return engine().readable(*_fd._s).then([this] {
        uint64_t count;
        int r = ::read(_fd.get_fd(), &count, sizeof(count));
        assert(r == sizeof(count));
        return make_ready_future<size_t>(count);
    });
}

void
reactor::block_notifier(int) {
    auto steps = engine()._tasks_processed_stalled.load(std::memory_order_relaxed);
    auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(engine()._task_quota * steps);

    backtrace_buffer buf;
    buf.append("Reactor stalled for ");
    buf.append_decimal(uint64_t(delta.count()));
    buf.append(" ms");
    // print_with_backtrace(buf);
}



std::chrono::nanoseconds
reactor::calculate_poll_time() {
    // In a non-virtualized environment, select a poll time
    // that is competitive with halt/unhalt.
    // In a virutalized environment, IPIs are slow and dominate
    // sleep/wake (mprotect/tgkill), so increase poll time to reduce
    // so we don't sleep in a request/reply workload
    return 200us; //200us是怎么得到的?
}

struct reactor_deleter {
    void operator()(reactor* p) {
        p->~reactor();
        free(p);
    }
};
void schedule_normal(std::unique_ptr<task> t) {
    std::cout<<"调用schedule normal"<<std::endl;
    engine().add_task(std::move(t));
}
void schedule_urgent(std::unique_ptr<task> t) {
    std::cout<<"调用schedule urgent"<<std::endl;
    engine().add_urgent_task(std::move(t));
}




template <typename T, size_t Capacity>
class spsc_queue {
private:
    // 使用缓存行对齐来避免伪共享
    alignas(64) std::atomic<size_t> _head{0}; // 
    alignas(64) std::atomic<size_t> _tail{0}; //
    // 环形缓冲区
    T _buffer[Capacity];
    // 帮助函数，计算下一个索引位置
    size_t next_index(size_t current) const {
        return (current + 1) % Capacity;
    }
public:
    spsc_queue() = default;
    // 禁止复制和移动
    spsc_queue(const spsc_queue&) = delete;
    spsc_queue& operator=(const spsc_queue&) = delete;
    spsc_queue(spsc_queue&&) = delete;
    spsc_queue& operator=(spsc_queue&&) = delete;
    // 检查队列是否为空
    bool empty() const {
        return _head.load(std::memory_order_relaxed) == _tail.load(std::memory_order_relaxed);
    }
    // 检查队列是否已满
    bool full() const {
        size_t next_tail = next_index(_tail.load(std::memory_order_relaxed));
        return next_tail == _head.load(std::memory_order_relaxed);
    }
    // 入队操作 - 生产者调用
    bool push(T item) {
        size_t current_tail = _tail.load(std::memory_order_relaxed);
        size_t next_tail = next_index(current_tail);
        if (next_tail == _head.load(std::memory_order_acquire)) {
            // 队列已满
            return false;
        }   
        _buffer[current_tail] = std::move(item);
        _tail.store(next_tail, std::memory_order_release);
        return true;
    }
    // 入队多个元素 - 返回成功入队的元素结束迭代器
    template <typename Iterator>
    Iterator push(Iterator begin, Iterator end) {
        Iterator current = begin;
        while (current != end) {
            if (!push(*current)) {
                break;
            }
            ++current;
        }
        return current;
    }
    // 出队操作 - 消费者调用
    bool pop(T& item) {
        size_t current_head = _head.load(std::memory_order_relaxed);
        if (current_head == _tail.load(std::memory_order_acquire)) {
            // 队列为空
            return false;
        }   
        item = std::move(_buffer[current_head]);
        _head.store(next_index(current_head), std::memory_order_release);
        return true;
    }
    // 批量出队 - 返回成功出队的元素数量
    template <size_t ArraySize>
    size_t pop(T (&items)[ArraySize]) {
        size_t popped = 0;
        while (popped < ArraySize && pop(items[popped])) {
            ++popped;
        }
        return popped;
    }
};

class smp_message_queue {
    static constexpr size_t queue_length = 128;
    static constexpr size_t batch_size = 16;
    static constexpr size_t prefetch_cnt = 2;
    struct work_item;
    struct lf_queue_remote {
        reactor* remote;
    };
    // 使用自定义的无锁队列替换boost::lockfree::spsc_queue
    using lf_queue_base = spsc_queue<work_item*, queue_length>;
    // 使用继承来控制布局顺序(?)
    struct lf_queue : lf_queue_remote, lf_queue_base {
        lf_queue(reactor* remote) : lf_queue_remote{remote} {}
        void maybe_wakeup();
    };
    lf_queue _pending;
    lf_queue _completed;
    struct alignas(64) {
        size_t _sent = 0;
        size_t _compl = 0;
        size_t _last_snt_batch = 0;
        size_t _last_cmpl_batch = 0;
        size_t _current_queue_length = 0;
    };
    // 在两个带有统计信息的结构体之间保持这个字段
    // 这确保它们之间至少有一个缓存行
    // 以便硬件预取器不会意外地预取另一个CPU使用的缓存行(硬件预取器是什么?)
    // metrics::metric_groups _metrics;
    struct alignas(64) {
        size_t _received = 0;
        size_t _last_rcv_batch = 0;
    };
    struct work_item {
        virtual ~work_item() {}
        virtual future<> process() = 0;
        virtual void complete() = 0;
    };
    template <typename Func>
    struct async_work_item : work_item {
        Func _func;
        using futurator = futurize<std::result_of_t<Func()>>;
        using future_type = typename futurator::type;
        using value_type = typename future_type::value_type;
        std::optional<value_type> _result;
        std::exception_ptr _ex; // if !_result
        typename futurator::promise_type _promise; // 在本地端使用
        async_work_item(Func&& func) : _func(std::move(func)) {}
        virtual future<> process() override {
            try {
                return futurator::apply(this->_func).then_wrapped([this] (auto&& f) {
                    try {
                        _result = f.get();
                    } catch (...) {
                        _ex = std::current_exception();
                    }
                });
            } catch (...) {
                _ex = std::current_exception();
                return make_ready_future();
            }
        }
        virtual void complete() override {
            if (_result) {
                _promise.set_value(std::move(*_result));
            } else {
                // FIXME: _ex was allocated on another cpu
                _promise.set_exception(std::move(_ex));
            }
        }
        future_type get_future() { return _promise.get_future(); }
    };
    union tx_side {
        tx_side() {}
        ~tx_side() {}
        void init() { new (&a) aa; }
        struct aa {
            std::deque<work_item*> pending_fifo;
        } a;
    } _tx;
    std::vector<work_item*> _completed_fifo;
public:
    smp_message_queue(reactor* from, reactor* to); // 使用reactor from 和reactor to初始化smp_message_queue.
    template <typename Func>
    futurize_t<std::result_of_t<Func()>> submit(Func&& func) {
        auto wi = std::make_unique<async_work_item<Func>>(std::forward<Func>(func));
        auto fut = wi->get_future();
        submit_item(std::move(wi));
        return fut;
    }
    void start(unsigned cpuid);
    template<size_t PrefetchCnt, typename Func>
    size_t process_queue(lf_queue& q, Func process);
    size_t process_incoming();
    size_t process_completions();
    void stop();
    
private:
    void work();
    void submit_item(std::unique_ptr<work_item> wi);
    void respond(work_item* wi);
    void move_pending();
    void flush_request_batch();
    void flush_response_batch();
    bool has_unflushed_responses() const;
    bool pure_poll_rx() const;
    bool pure_poll_tx() const;

    friend class smp;
};


timespec to_timespec(steady_clock_type::time_point t) {
    using ns = std::chrono::nanoseconds;
    auto n = std::chrono::duration_cast<ns>(t.time_since_epoch()).count();
    return { n / 1'000'000'000, n % 1'000'000'000 };
}

__thread reactor* local_engine;
reactor& engine(){
    return *local_engine;
}


bool queue_timer(timer<steady_clock_type>* tmr) {
    return engine().queue_timer(tmr);
}

void add_timer(timer<steady_clock_type>* tmr) {
    engine().add_timer(tmr);
}

void add_timer(timer<lowres_clock>* tmr) {
    engine().add_timer(tmr);
}

void add_timer(timer<manual_clock>* tmr) {
     engine().add_timer(tmr);
}

bool queue_timer(timer<manual_clock>* tmr) {
    return engine().queue_timer(tmr);
}

bool queue_timer(timer<lowres_clock>* tmr) {
    return engine().queue_timer(tmr);
}


void manual_clock::advance(manual_clock::duration d) {
    _now.fetch_add(d.count());
    // engine().schedule_urgent(make_task(&manual_clock::expire_timers));
    //smp::invoke_on_all(&manual_clock::expire_timers);
    return;
}

void del_timer(timer<lowres_clock>* tmr) {
    engine().del_timer(tmr);
}

void del_timer(timer<steady_clock_type>* tmr) {
    engine().del_timer(tmr);
}
void del_timer(timer<manual_clock>* tmr) {
    engine().del_timer(tmr);
}

template<int Signal, void(*Func)()>
void install_oneshot_signal_handler() {
    static bool handled = false;
    static util::spinlock lock;
    struct sigaction sa;
    sa.sa_sigaction = [](int sig, siginfo_t *info, void *p) {
        std::lock_guard<util::spinlock> g(lock);
        if (!handled) {
            handled = true;
            Func();
            signal(sig, SIG_DFL);
        }
    };
    sigfillset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO | SA_RESTART;
    if (Signal == SIGSEGV) {
        sa.sa_flags |= SA_ONSTACK;
    }
    auto r = ::sigaction(Signal, &sa, nullptr);
    // throw_system_error_on(false);//这是我改的.
}

static void sigsegv_action() noexcept {
    std::cout<<"Segmentation fault";
}

static void sigabrt_action() noexcept {
    std::cout<<"Aborting";
}

template<typename Clock>
struct with_clock {};
template <typename... T>
struct future_option_traits;
template <typename Clock, typename... T>
struct future_option_traits<with_clock<Clock>, T...> {
    using clock_type = Clock;
    template<template <typename...> class Class>
    struct parametrize {
        using type = Class<T...>;
    };
};

template <typename... T>
struct future_option_traits {
    using clock_type = lowres_clock;
    template<template <typename...> class Class>
    struct parametrize {
        using type = Class<T...>;
    };
};





inline
void pin_this_thread(unsigned cpu_id) {
    cpu_set_t cs;
    CPU_ZERO(&cs);
    CPU_SET(cpu_id, &cs);
    auto r = pthread_setaffinity_np(pthread_self(), sizeof(cs), &cs);
    assert(r == 0);
}

namespace bi = boost::intrusive;
template <typename... T>
class promise;

template <typename... T>
class future;
class thread;
class thread_attributes;
class thread_scheduling_group;
struct jmp_buf_link;
template <class... T>
class promise;
template <class... T> 
class future;
template<> 
class promise<void>;

template <typename... T, typename... A>
future<T...> make_ready_future(A&&... value);

template <typename... T>
future<T...> make_exception_future(std::exception_ptr value) noexcept;

template<typename... T>
class shared_future {
    template <typename... U> friend class shared_promise;
    using options = future_option_traits<T...>;
public:
    using clock = typename options::clock_type;
    using time_point = typename clock::time_point;
    using future_type = typename future_option_traits<T...>::template parametrize<future>::type;
    using promise_type = typename future_option_traits<T...>::template parametrize<promise>::type;
    using value_tuple_type = typename future_option_traits<T...>::template parametrize<std::tuple>::type;
private:
    using future_state_type = typename future_option_traits<T...>::template parametrize<future_state>::type;
    using promise_expiry = typename future_option_traits<T...>::template parametrize<promise_expiry>::type;

    class shared_state {
        future_state_type _future_state;
        expiring_fifo<promise_type, promise_expiry, clock> _peers;
    public:
        void resolve(future_type&& f) noexcept {
            _future_state = f.get_available_state();
            if (_future_state.failed()) {
                while (_peers) {
                    _peers.front().set_exception(_future_state.get_exception());
                    _peers.pop_front();
                }
            } else {
                while (_peers) {
                    auto& p = _peers.front();
                    try {
                        p.set_value(_future_state.get_value());
                    } catch (...) {
                        p.set_exception(std::current_exception());
                    }
                    _peers.pop_front();
                }
            }
        }

        future_type get_future(time_point timeout = time_point::max()) {
            if (!_future_state.available()) {
                promise_type p;
                auto f = p.get_future();
                _peers.push_back(std::move(p), timeout);
                return f;
            } else if (_future_state.failed()) {
                return future_type(exception_future_marker(), _future_state.get_exception());
            } else {
                try {
                    return future_type(ready_future_marker(), _future_state.get_value());
                } catch (...) {
                    return future_type(exception_future_marker(), std::current_exception());
                }
            }
        }
    };
    lw_shared_ptr<shared_state> _state;
public:
    shared_future(future_type&& f)
        : _state(make_lw_shared<shared_state>())
    {
        f.then_wrapped([s = _state] (future_type&& f) mutable {
            s->resolve(std::move(f));
        });
    }

    shared_future() = default;
    shared_future(const shared_future&) = default;
    shared_future& operator=(const shared_future&) = default;
    shared_future(shared_future&&) = default;
    shared_future& operator=(shared_future&&) = default;
    future_type get_future(time_point timeout = time_point::max()) const {
        return _state->get_future(timeout);
    }
    operator future_type() const {
        return get_future();
    }
    bool valid() const {
        return bool(_state);
    }
};
template <typename... T>
class shared_promise {
public:
    using shared_future_type = shared_future<T...>;
    using future_type = typename shared_future_type::future_type;
    using promise_type = typename shared_future_type::promise_type;
    using clock = typename shared_future_type::clock;
    using time_point = typename shared_future_type::time_point;
    using value_tuple_type = typename shared_future_type::value_tuple_type;
    using future_state_type = typename shared_future_type::future_state_type;
private:
    promise_type _promise;
    shared_future_type _shared_future;
    static constexpr bool copy_noexcept = future_state_type::copy_noexcept;
public:
    shared_promise(const shared_promise&) = delete;
    shared_promise(shared_promise&&) = default;
    shared_promise& operator=(shared_promise&&) = default;
    shared_promise() : _promise(), _shared_future(_promise.get_future()) {
    }
    /// \brief Gets new future associated with this promise.
    /// If the promise is not resolved before timeout the returned future will resolve with \ref timed_out_error.
    /// This instance doesn't have to be kept alive until the returned future resolves.
    future_type get_shared_future(time_point timeout = time_point::max()) {
        return _shared_future.get_future(timeout);
    }
    /// \brief Sets the shared_promise's value (as tuple; by copying), same as normal promise
    void set_value(const value_tuple_type& result) noexcept(copy_noexcept) {
        _promise.set_value(result);
    }
    /// \brief Sets the shared_promise's value (as tuple; by moving), same as normal promise
    void set_value(value_tuple_type&& result) noexcept {
        _promise.set_value(std::move(result));
    }
    /// \brief Sets the shared_promise's value (variadic), same as normal promise
    template <typename... A>
    void set_value(A&&... a) noexcept {
        _promise.set_value(std::forward<A>(a)...);
    }
    /// \brief Marks the shared_promise as failed, same as normal promise
    void set_exception(std::exception_ptr ex) noexcept {
        _promise.set_exception(std::move(ex));
    }
    /// \brief Marks the shared_promise as failed, same as normal promise
    template<typename Exception>
    void set_exception(Exception&& e) noexcept {
        set_exception(make_exception_ptr(std::forward<Exception>(e)));
    }
};



class smp {
public:
    static std::vector<posix_thread> _threads;
    static std::vector<std::function<void ()>> _thread_loops; // for dpdk
    static std::optional<boost::barrier> _all_event_loops_done;
    static std::vector<reactor*> _reactors;
    static smp_message_queue** _qs; 
    static std::thread::id _tmain;
    static bool _using_dpdk;

    template <typename Func>
    using returns_future = is_future<std::result_of_t<Func()>>;
    
    template <typename Func>
    using returns_void = std::is_same<std::result_of_t<Func()>, void>;
    
    static boost::program_options::options_description get_options_description();
    static void configure(boost::program_options::variables_map vm);
    static void cleanup();
    static void cleanup_cpu();
    static void arrive_at_event_loop_end();
    static void join_all();
    static bool main_thread() { return std::this_thread::get_id() == _tmain; }
    
    template <typename Func>
    static futurize_t<std::result_of_t<Func()>> submit_to(unsigned t, Func&& func);
    static bool poll_queues();
    static bool pure_poll_queues();
    static boost::integer_range<unsigned> all_cpus() {
        return boost::irange(0u, count);
    }
    
    template<typename Func>
    static future<> invoke_on_all(Func&& func);
    
    static void start_all_queues();
    static void pin(unsigned cpu_id);
    static void allocate_reactor(unsigned id);
    static void create_thread(std::function<void ()> thread_loop);
public:
    static unsigned count;
};


thread_local std::unique_ptr<reactor, reactor_deleter> reactor_holder;
std::vector<posix_thread> smp::_threads;
std::vector<std::function<void ()>> smp::_thread_loops;
std::optional<boost::barrier> smp::_all_event_loops_done;
std::vector<reactor*> smp::_reactors;
smp_message_queue** smp::_qs;//为什么是二级指针？
std::thread::id smp::_tmain;


/// \brief Creates a \ref future in an available, failed state.
///
/// Creates a \ref future object that is already resolved in a failed
/// state.  This no I/O needs to be performed to perform a computation
/// (for example, because the connection is closed and we cannot read
/// from it).
template <typename... T, typename Exception>
inline
future<T...> make_exception_future(Exception&& ex) noexcept {
    return make_exception_future<T...>(std::make_exception_ptr(std::forward<Exception>(ex)));
}

/// @}

/// \cond internal

template<typename T>
template<typename Func, typename... FuncArgs>
typename futurize<T>::type futurize<T>::apply(Func&& func, std::tuple<FuncArgs...>&& args) noexcept {
    try {
        return convert(std::apply(std::forward<Func>(func), std::move(args)));
        //执行这个函数,并返回结果.然后对结果执行convert。
    } catch (...) {
        return make_exception_future(std::current_exception());
    }
}

template<typename T>
template<typename Func, typename... FuncArgs>
typename futurize<T>::type futurize<T>::apply(Func&& func, FuncArgs&&... args) noexcept {
    try {
        return convert(func(std::forward<FuncArgs>(args)...));
    } catch (...) {
        return make_exception_future(std::current_exception());
    }
}

template<typename Func, typename... FuncArgs>
inline
std::enable_if_t<!is_future<std::result_of_t<Func(FuncArgs&&...)>>::value, future<>>
do_void_futurize_apply(Func&& func, FuncArgs&&... args) noexcept {
    try {
        func(std::forward<FuncArgs>(args)...);
        return make_ready_future<>();
    } catch (...) {
        return make_exception_future(std::current_exception());
    }
}

template<typename Func, typename... FuncArgs>
inline
std::enable_if_t<is_future<std::result_of_t<Func(FuncArgs&&...)>>::value, future<>>
do_void_futurize_apply(Func&& func, FuncArgs&&... args) noexcept {
    try {
        return func(std::forward<FuncArgs>(args)...);
    } catch (...) {
        return make_exception_future(std::current_exception());
    }
}

template<typename Func, typename... FuncArgs>
inline
std::enable_if_t<!is_future<std::result_of_t<Func(FuncArgs&&...)>>::value, future<>>
do_void_futurize_apply_tuple(Func&& func, std::tuple<FuncArgs...>&& args) noexcept {
    try {
        std::apply(std::forward<Func>(func), std::move(args));
        return make_ready_future<>();
    } catch (...) {
        return make_exception_future(std::current_exception());
    }
}

template<typename Func, typename... FuncArgs>
inline
std::enable_if_t<is_future<std::result_of_t<Func(FuncArgs&&...)>>::value, future<>>
do_void_futurize_apply_tuple(Func&& func, std::tuple<FuncArgs...>&& args) noexcept {
    try {
        return std::apply(std::forward<Func>(func), std::move(args));
    } catch (...) {
        return make_exception_future(std::current_exception());
    }
}

template<typename Func, typename... FuncArgs>
typename futurize<void>::type futurize<void>::apply(Func&& func, std::tuple<FuncArgs...>&& args) noexcept {
    return do_void_futurize_apply_tuple(std::forward<Func>(func), std::move(args));
}

template<typename Func, typename... FuncArgs>
typename futurize<void>::type futurize<void>::apply(Func&& func, FuncArgs&&... args) noexcept {
    return do_void_futurize_apply(std::forward<Func>(func), std::forward<FuncArgs>(args)...);
}

template<typename... Args>
template<typename Func, typename... FuncArgs>
typename futurize<future<Args...>>::type futurize<future<Args...>>::apply(Func&& func, std::tuple<FuncArgs...>&& args) noexcept {
    try {
        return std::apply(std::forward<Func>(func), std::move(args));
    } catch (...) {
        return make_exception_future(std::current_exception());
    }
}

template<typename... Args>
template<typename Func, typename... FuncArgs>
typename futurize<future<Args...>>::type futurize<future<Args...>>::apply(Func&& func, FuncArgs&&... args) noexcept {
    try {
        return func(std::forward<FuncArgs>(args)...);
    } catch (...) {
        return make_exception_future(std::current_exception());
    }
}

template<typename Func, typename... Args>
auto futurize_apply(Func&& func, Args&&... args) {
    using futurator = futurize<std::result_of_t<Func(Args&&...)>>;
    return futurator::apply(std::forward<Func>(func), std::forward<Args>(args)...);
}
/// Executes a callable in a seastar thread.
/// Runs a block of code in a threaded context,
/// which allows it to block (using \ref future::get()).  The
/// result of the callable is returned as a future.
/// \param func a callable to be executed in a thread
/// \param args a parameter pack to be forwarded to \c func.
/// \return whatever \c func returns, as a future.
/// Clock used for scheduling threads
using thread_clock = std::chrono::steady_clock;
struct thread_attributes {
        thread_scheduling_group* scheduling_group = nullptr;
};

class thread_scheduling_group {
    public:
        std::chrono::nanoseconds _period;
        std::chrono::nanoseconds _quota;
        std::chrono::time_point<thread_clock> _this_period_ends = {};
        std::chrono::time_point<thread_clock> _this_run_start = {};
        std::chrono::nanoseconds _this_period_remain = {};
        /// \brief Constructs a \c thread_scheduling_group object
        ///
        /// \param period a duration representing the period
        /// \param usage which fraction of the \c period to assign for the scheduling group. Expected between 0 and 1.
        thread_scheduling_group(std::chrono::nanoseconds period, float usage);
        /// \brief changes the current maximum usage per period
        ///
        /// \param new_usage The new fraction of the \c period (Expected between 0 and 1) during which to run
        void update_usage(float new_usage) {
            _quota = std::chrono::duration_cast<std::chrono::nanoseconds>(new_usage * _period);
        }
        void account_start();
        void account_stop();
        std::chrono::steady_clock::time_point* next_scheduling_point() const;
};

class thread_context;
struct jmp_buf_link {
    jmp_buf jmpbuf;
    jmp_buf_link* link;
    thread_context* thread;
    bool has_yield_at = false;
    std::chrono::time_point<thread_clock> yield_at_value;
    void initial_switch_in(ucontext_t* initial_context, const void* stack_bottom, size_t stack_size);
    void switch_in();
    void switch_out();
    void initial_switch_in_completed();
    void final_switch_out();
    std::chrono::time_point<thread_clock>* get_yield_at() {
        return has_yield_at ? &yield_at_value : nullptr;
    }
    void set_yield_at(const std::chrono::time_point<thread_clock>& value) {
        yield_at_value = value;
        has_yield_at = true;
    }    
    void clear_yield_at() {
        has_yield_at = false;
    }
};

thread_local jmp_buf_link g_unthreaded_context; //在jmp_buf_link init_switch_in的时候用来初始化g_current_context
thread_local jmp_buf_link* g_current_context;

struct thread_context {
    struct stack_deleter {
        void operator()(char *ptr) const noexcept;
    };
    using stack_holder = std::unique_ptr<char[], stack_deleter>;
    thread_attributes _attr;
    static constexpr size_t _stack_size = 128*1024;
    stack_holder _stack{make_stack()};
    std::function<void ()> _func;
    jmp_buf_link _context;
    promise<> _done;
    bool _joined = false;
    timer<> _sched_timer{[this] { reschedule(); }};
    promise<>* _sched_promise_ptr = nullptr;
    promise<> _sched_promise_value;
    std::list<thread_context*>::iterator _preempted_it;
    std::list<thread_context*>::iterator _all_it;
    // Replace boost::intrusive::list with std::list
    static thread_local std::list<thread_context*> _preempted_threads;
    static thread_local std::list<thread_context*> _all_threads;
    static void s_main(unsigned int lo, unsigned int hi);
    void setup();
    void main();
    static stack_holder make_stack();
    thread_context(thread_attributes attr, std::function<void ()> func);
    ~thread_context();
    void switch_in();
    void switch_out();
    bool should_yield() const;
    void reschedule();
    void yield();
    promise<>* get_sched_promise() {
        return _sched_promise_ptr;
    }
    void set_sched_promise() {
        _sched_promise_ptr = &_sched_promise_value;
    }
    void clear_sched_promise() {
        _sched_promise_ptr = nullptr;
    }
};
namespace thread_impl {
    inline thread_context* get() {
        return g_current_context->thread;
    }
    inline bool should_yield() {
        if (need_preempt()) {
            return true;
        } else if (g_current_context->get_yield_at()) {
            return std::chrono::steady_clock::now() >= *(g_current_context->get_yield_at());
        } else {
            return false;
        }
    }
    void yield(){
        g_current_context->thread->yield();
    }
    void switch_in(thread_context* to){
        to->switch_in();
    }
    void switch_out(thread_context* from){
        from->switch_out();
    }
    void init(){
        g_unthreaded_context.link = nullptr;
        g_unthreaded_context.thread = nullptr;
        g_current_context = &g_unthreaded_context;
    }
}


class thread {
    std::unique_ptr<thread_context> _context;
    static thread_local thread* _current;
public:
    /// \brief Constructs a \c thread object that does not represent a thread
    /// of execution.
    thread() = default;

    /// \brief Constructs a \c thread object that represents a thread of execution
    ///
    /// \param func Callable object to execute in thread.  The callable is
    ///             called immediately.
    template <typename Func>
    thread(Func func);

    /// \brief Constructs a \c thread object that represents a thread of execution
    /// \param attr Attributes describing the new thread.
    /// \param func Callable object to execute in thread.  The callable is
    ///             called immediately.
    template <typename Func>
    thread(thread_attributes attr, Func func);

    /// \brief Moves a thread object.
    thread(thread&& x) noexcept = default;

    /// \brief Move-assigns a thread object.
    thread& operator=(thread&& x) noexcept = default;

    /// \brief Destroys a \c thread object.
    /// The thread must not represent a running thread of execution (see join()).
    ~thread();

    future<> join();

    /// \brief Voluntarily defer execution of current thread.
    /// Gives other threads/fibers a chance to run on current CPU.
    /// The current thread will resume execution promptly.
    static void yield();

    /// \brief Checks whether this thread ought to call yield() now
    /// Useful where we cannot call yield() immediately because we
    /// Need to take some cleanup action first.
    static bool should_yield();

    static bool running_in_thread() {
        return thread_impl::get() != nullptr;
    }
    static bool try_run_one_yielded_thread();
};



class gate {
    size_t _count = 0;
    promise<>* _stopped_ptr = nullptr;
    promise<> _stopped_value;
public:
    void enter() {
        if (_stopped_ptr) {
            throw 1;
        }
        ++_count;
    }
    void leave() {
        --_count;
        if (!_count && _stopped_ptr) {
            _stopped_ptr->set_value();
        }
    }
    void check() {
        if (_stopped_ptr) {
            throw 1;
        }
    }
    future<> close() {
        assert(!_stopped_ptr && "gate::close() cannot be called more than once");
        _stopped_ptr = &_stopped_value;
        if (!_count) {
            _stopped_ptr->set_value();
        }
        return _stopped_ptr->get_future();
    }
    size_t get_count() const {
        return _count;
    }
};
template <typename Func>
inline
auto
with_gate(gate& g, Func&& func) {
    g.enter();
    return func().finally([&g] { g.leave(); });
}


// future<> later() {
//     promise<> p;
//     auto f = p.get_future();
//     engine().force_poll(); //把need_preempted改为true(这句是没有意义的)
//     ::schedule_normal(make_task([p = std::move(p)]() mutable {
//         p.set_value(); // 这段代码把一个p.set_value封装为一个task加到调度器中.
//     }));
//     return f;
// }


template<typename ExceptionFactory, typename Clock = typename timer<>::clock>
future<semaphore_units<ExceptionFactory, Clock>>
get_units(basic_semaphore<ExceptionFactory, Clock>& sem, size_t units) {
    return sem.wait(units).then([&sem, units] {
        return semaphore_units<ExceptionFactory, Clock>{ sem, units };
    });
}

template<typename ExceptionFactory, typename Clock = typename timer<>::clock>
future<semaphore_units<ExceptionFactory, Clock>>
get_units(basic_semaphore<ExceptionFactory, Clock>& sem, size_t units, typename basic_semaphore<ExceptionFactory, Clock>::time_point timeout) {
    return sem.wait(timeout, units).then([&sem, units] {
        return semaphore_units<ExceptionFactory, Clock>{ sem, units };
    });
}
template<typename ExceptionFactory, typename Clock = typename timer<>::clock>
semaphore_units<ExceptionFactory, Clock>
consume_units(basic_semaphore<ExceptionFactory, Clock>& sem, size_t units) {
    sem.consume(units);
    return semaphore_units<ExceptionFactory, Clock>{ sem, units };
}

template <typename ExceptionFactory, typename Func, typename Clock = typename timer<>::clock>
inline
futurize_t<std::result_of_t<Func()>>
with_semaphore(basic_semaphore<ExceptionFactory, Clock>& sem, size_t units, Func&& func) {
    return get_units(sem, units).then([func = std::forward<Func>(func)] (auto units) mutable {
        return futurize_apply(std::forward<Func>(func)).finally([units = std::move(units)] {});
    });
}



template <typename Func, typename... Args>
inline futurize_t<std::result_of_t<std::decay_t<Func>(std::decay_t<Args>...)>>
async(Func&& func, Args&&... args) {
    return async(thread_attributes{}, std::forward<Func>(func), std::forward<Args>(args)...);
}

template <typename Func, typename... Args>
inline
futurize_t<std::result_of_t<std::decay_t<Func>(std::decay_t<Args>...)>>
async(thread_attributes attr, Func&& func, Args&&... args) {
    using return_type = std::result_of_t<std::decay_t<Func>(std::decay_t<Args>...)>;
    struct work {
        thread_attributes attr;
        Func func;
        std::tuple<Args...> args;
        promise<return_type> pr;
        thread th;
    };
    return do_with(work{std::move(attr), std::forward<Func>(func), std::forward_as_tuple(std::forward<Args>(args)...)}, [] (work& w) mutable {
        auto ret = w.pr.get_future();
        w.th = thread(std::move(w.attr), [&w] {
            futurize<return_type>::apply(std::move(w.func), std::move(w.args)).forward_to(std::move(w.pr));
        });
        return w.th.join().then([ret = std::move(ret)] () mutable {
            return std::move(ret);
        });
    });
}


void report_failed_future(std::exception_ptr eptr) {
   std::cout<<"####"<<std::endl;
}


// Define the static members
thread_local std::list<thread_context*> thread_context::_preempted_threads;
thread_local std::list<thread_context*> thread_context::_all_threads;








#include "../util/shared_ptr.hh"
#include "../util/bool_class.hh"
#include <tuple>
#include <iterator>
#include <vector>
#include <experimental/optional>
#include "util/tuple_utils.hh"
extern __thread size_t task_quota;
struct parallel_for_each_state {
    // use optional<> to avoid out-of-line constructor
    std::optional<std::exception_ptr> ex;
    size_t waiting = 0;
    promise<> pr;
    void complete() {
        if (--waiting == 0) {
            if (ex) {
                pr.set_exception(std::move(*ex));
            } else {
                pr.set_value();
            }
        }
    }
};

//这里？
template <typename Iterator, typename Func>
GCC6_CONCEPT(requires requires (Func f, Iterator i) { { f(*i++) } -> std::same_as<future<>>; })
inline
future<>
parallel_for_each(Iterator begin, Iterator end, Func&& func) {
    if (begin == end) {
        return make_ready_future<>();
    }
    return do_with(parallel_for_each_state(), [&] (parallel_for_each_state& state) -> future<> {
        // increase ref count to ensure all functions run
        ++state.waiting;
        while (begin != end) {
            ++state.waiting;
            try {
                func(*begin++).then_wrapped([&] (future<> f) {
                    if (f.failed()) {
                        // We can only store one exception.  For more, use when_all().
                        if (!state.ex) {
                            state.ex = f.get_exception();
                        } else {
                            f.ignore_ready_future();
                        }
                    }
                    state.complete();
                });
            } catch (...) {
                if (!state.ex) {
                    state.ex = std::move(std::current_exception());
                }
                state.complete();
            }
        }
        // match increment on top
        state.complete();
        return state.pr.get_future();
    });
}


template <typename Range, typename Func>
GCC6_CONCEPT(requires requires (Func f, Range r) { { f(*r.begin()) } -> std::same_as<future<>>; })
inline
future<>
parallel_for_each(Range&& range, Func&& func) {
    return parallel_for_each(std::begin(range), std::end(range),
            std::forward<Func>(func));
}


template<typename AsyncAction, typename StopCondition>
static inline
void do_until_continued(StopCondition&& stop_cond, AsyncAction&& action, promise<> p) {
    while (!stop_cond()) {
        try {
            auto&& f = action();
            if (!f.available() || need_preempt()) {
                f.then_wrapped([action = std::forward<AsyncAction>(action),
                                stop_cond = std::forward<StopCondition>(stop_cond), 
                                p = std::move(p)]  // 修复：移动捕获p
                                (std::result_of_t<AsyncAction()> fut) mutable {
                    if (!fut.failed()) {
                        do_until_continued(std::forward<StopCondition>(stop_cond), 
                                          std::forward<AsyncAction>(action), 
                                          std::move(p));  // 修复：移动p
                    } else {
                        p.set_exception(fut.get_exception());  // 此时p已经被捕获
                    }
                });
                return;
            }
            if (f.failed()) {
                f.forward_to(std::move(p));
                return;
            }
        } catch (...) {
            p.set_exception(std::current_exception());
            return;
        }
    }
    p.set_value();
}


struct stop_iteration_tag { };
using stop_iteration = bool_class<stop_iteration_tag>;


template<typename AsyncAction>
GCC6_CONCEPT( requires ApplyReturns<AsyncAction, stop_iteration> || ApplyReturns<AsyncAction, future<stop_iteration>> )
static inline
future<> repeat(AsyncAction&& action) {
    using futurator = futurize<std::result_of_t<AsyncAction()>>;
    static_assert(std::is_same<future<stop_iteration>, typename futurator::type>::value, "bad AsyncAction signature");

    try {
        do {
            auto f = futurator::apply(action);

            if (!f.available()) {
                return f.then([action = std::forward<AsyncAction>(action)] (stop_iteration stop) mutable {
                    if (stop == stop_iteration::yes) {
                        return make_ready_future<>();
                    } else {
                        return repeat(std::forward<AsyncAction>(action));
                    }
                });
            }

            if (f.get0() == stop_iteration::yes) {
                return make_ready_future<>();
            }
        } while (!need_preempt());

        promise<> p;
        auto f = p.get_future();
        schedule(make_task([action = std::forward<AsyncAction>(action), p = std::move(p)]() mutable {
            repeat(std::forward<AsyncAction>(action)).forward_to(std::move(p));
        }));
        return f;
    } catch (...) {
        return make_exception_future(std::current_exception());
    }
}


template <typename T>
struct repeat_until_value_type_helper;


/// Type helper for repeat_until_value()
template <typename T>
struct repeat_until_value_type_helper<future<std::optional<T>>> {
    using value_type = T;
    using optional_type = std::optional<T>;
    using future_type = future<value_type>;
    using future_optional_type = future<optional_type>;
};

template <typename AsyncAction>
using repeat_until_value_return_type
        = typename repeat_until_value_type_helper<std::result_of_t<AsyncAction()>>::future_type;

template<typename AsyncAction>
GCC6_CONCEPT( requires requires (AsyncAction aa) {
    requires is_future<decltype(aa())>::value;
    bool(aa().get0());
    aa().get0().value();
} )
repeat_until_value_return_type<AsyncAction>
repeat_until_value(AsyncAction&& action) {
    using type_helper = repeat_until_value_type_helper<std::result_of_t<AsyncAction()>>;
    // the "T" in the documentation
    using value_type = typename type_helper::value_type;
    using optional_type = typename type_helper::optional_type;
    using futurator = futurize<typename type_helper::future_optional_type>;
    do {
        auto f = futurator::apply(action);

        if (!f.available()) {
            return f.then([action = std::forward<AsyncAction>(action)] (auto&& optional) mutable {
                if (optional) {
                    return make_ready_future<value_type>(std::move(optional.value()));
                } else {
                    return repeat_until_value(std::forward<AsyncAction>(action));
                }
            });
        }

        if (f.failed()) {
            return make_exception_future<value_type>(f.get_exception());
        }

        optional_type&& optional = std::move(f).get0();
        if (optional) {
            return make_ready_future<value_type>(std::move(optional.value()));
        }
    } while (!need_preempt());

    try {
        promise<value_type> p;
        auto f = p.get_future();
        schedule(make_task([action = std::forward<AsyncAction>(action), p = std::move(p)] () mutable {
            repeat_until_value(std::forward<AsyncAction>(action)).forward_to(std::move(p));
        }));
        return f;
    } catch (...) {
        return make_exception_future<value_type>(std::current_exception());
    }
}

template<typename AsyncAction, typename StopCondition>
GCC6_CONCEPT( requires ApplyReturns<StopCondition, bool> && ApplyReturns<AsyncAction, future<>> )
static inline
future<> do_until(StopCondition&& stop_cond, AsyncAction&& action) {
    promise<> p;
    auto f = p.get_future();
    do_until_continued(std::forward<StopCondition>(stop_cond),
        std::forward<AsyncAction>(action), std::move(p));
    return f;
}

template<typename AsyncAction>
GCC6_CONCEPT( requires ApplyReturns<AsyncAction, future<>> )
static inline
future<> keep_doing(AsyncAction&& action) {
    return repeat([action = std::forward<AsyncAction>(action)] () mutable {
        return action().then([] {
            return stop_iteration::no;
        });
    });
}


template<typename Iterator, typename AsyncAction>
GCC6_CONCEPT( requires requires (Iterator i, AsyncAction aa) { { aa(*i) } -> std::same_as<future<>>; } )
static inline
future<> do_for_each(Iterator begin, Iterator end, AsyncAction&& action) {
    if (begin == end) {
        return make_ready_future<>();
    }
    while (true) {
        auto f = action(*begin++);
        if (begin == end) {
            return f;
        }
        if (!f.available() || need_preempt()) {
            return std::move(f).then([action = std::forward<AsyncAction>(action),
                    begin = std::move(begin), end = std::move(end)] () mutable {
                return do_for_each(std::move(begin), std::move(end), std::forward<AsyncAction>(action));
            });
        }
        if (f.failed()) {
            return std::move(f);
        }
    }
}

template<typename Container, typename AsyncAction>
GCC6_CONCEPT( requires requires (Container c, AsyncAction aa) { { aa(*c.begin()) } -> std::same_as<future<>>; } )
static inline
future<> do_for_each(Container& c, AsyncAction&& action) {
    return do_for_each(std::begin(c), std::end(c), std::forward<AsyncAction>(action));
}

namespace internal {

template<typename... Futures>
struct identity_futures_tuple {
    using future_type = future<std::tuple<Futures...>>;
    using promise_type = typename future_type::promise_type;

    static void set_promise(promise_type& p, std::tuple<Futures...> futures) {
        p.set_value(std::move(futures));
    }
};

template<typename ResolvedTupleTransform, typename... Futures>
class when_all_state : public enable_lw_shared_from_this<when_all_state<ResolvedTupleTransform, Futures...>> {
    using type = std::tuple<Futures...>;
    type tuple;
public:
    typename ResolvedTupleTransform::promise_type p;
    when_all_state(Futures&&... t) : tuple(std::make_tuple(std::move(t)...)) {}
    ~when_all_state() {
        ResolvedTupleTransform::set_promise(p, std::move(tuple));
    }
private:
    template<size_t Idx>
    int wait() {
        auto& f = std::get<Idx>(tuple);
        static_assert(is_future<std::remove_reference_t<decltype(f)>>::value, "when_all parameter must be a future");
        if (!f.available()) {
            f = f.then_wrapped([s = this->shared_from_this()] (auto&& f) {
                return std::move(f);
            });
        }
        return 0;
    }
public:
    template <size_t... Idx>
    typename ResolvedTupleTransform::future_type wait_all(std::index_sequence<Idx...>) {
        [] (...) {} (this->template wait<Idx>()...);
        return p.get_future();
    }
};
}

// GCC6_CONCEPT(
// /// \cond internal
// namespace impl {
// // Want: folds
// template <typename T>
// struct is_tuple_of_futures : std::false_type {
// };
// template <>
// struct is_tuple_of_futures<std::tuple<>> : std::true_type {
// };
// template <typename... T, typename... Rest>
// struct is_tuple_of_futures<std::tuple<future<T...>, Rest...>> : is_tuple_of_futures<std::tuple<Rest...>> {
// };
// }

// template <typename... Futs>
// concept bool AllAreFutures = impl::is_tuple_of_futures<std::tuple<Futs...>>::value;
// )


GCC6_CONCEPT(
namespace impl {
// Want: folds
template <typename T>
struct is_tuple_of_futures : std::false_type {
};

template <>
struct is_tuple_of_futures<std::tuple<>> : std::true_type {
};

template <typename... T, typename... Rest>
struct is_tuple_of_futures<std::tuple<future<T...>, Rest...>> : is_tuple_of_futures<std::tuple<Rest...>> {
};
}

template <typename... Futs>
concept AllAreFutures = impl::is_tuple_of_futures<std::tuple<Futs...>>::value;



// template <typename Func, typename... T>
// concept ApplyReturnsAnyFuture = requires (Func f, T... args) {
//     requires is_future<decltype(f(std::forward<T>(args)...))>::value;
// };
)


template <typename... Futs>
GCC6_CONCEPT( requires AllAreFutures<Futs...> )
inline
future<std::tuple<Futs...>>
when_all(Futs&&... futs) {
    namespace si = internal;
    using state = si::when_all_state<si::identity_futures_tuple<Futs...>, Futs...>;
    auto s = make_lw_shared<state>(std::forward<Futs>(futs)...);
    return s->wait_all(std::make_index_sequence<sizeof...(Futs)>());
}

/// \cond internal
namespace internal {

template <typename Iterator, typename IteratorCategory>
inline
size_t
when_all_estimate_vector_capacity(Iterator begin, Iterator end, IteratorCategory category) {
    // For InputIterators we can't estimate needed capacity
    return 0;
}

template <typename Iterator>
inline
size_t
when_all_estimate_vector_capacity(Iterator begin, Iterator end, std::forward_iterator_tag category) {
    // May be linear time below random_access_iterator_tag, but still better than reallocation
    return std::distance(begin, end);
}

template<typename Future>
struct identity_futures_vector {
    using future_type = future<std::vector<Future>>;
    static future_type run(std::vector<Future> futures) {
        return make_ready_future<std::vector<Future>>(std::move(futures));
    }
};

// Internal function for when_all().
template <typename ResolvedVectorTransform, typename Future>
inline
typename ResolvedVectorTransform::future_type
complete_when_all(std::vector<Future>&& futures, typename std::vector<Future>::iterator pos) {
    // If any futures are already ready, skip them.
    while (pos != futures.end() && pos->available()) {
        ++pos;
    }
    // Done?
    if (pos == futures.end()) {
        return ResolvedVectorTransform::run(std::move(futures));
    }
    // Wait for unready future, store, and continue.
    return pos->then_wrapped([futures = std::move(futures), pos] (auto fut) mutable {
        *pos++ = std::move(fut);
        return complete_when_all<ResolvedVectorTransform>(std::move(futures), pos);
    });
}

template<typename ResolvedVectorTransform, typename FutureIterator>
inline auto
do_when_all(FutureIterator begin, FutureIterator end) {
    using itraits = std::iterator_traits<FutureIterator>;
    std::vector<typename itraits::value_type> ret;
    ret.reserve(when_all_estimate_vector_capacity(begin, end, typename itraits::iterator_category()));
    // Important to invoke the *begin here, in case it's a function iterator,
    // so we launch all computation in parallel.
    std::move(begin, end, std::back_inserter(ret));
    return complete_when_all<ResolvedVectorTransform>(std::move(ret), ret.begin());
}

}


template <typename FutureIterator>
GCC6_CONCEPT( requires requires (FutureIterator i) { { *i++ }; requires is_future<std::remove_reference_t<decltype(*i)>>::value; } )





inline
future<std::vector<typename std::iterator_traits<FutureIterator>::value_type>>
when_all(FutureIterator begin, FutureIterator end) {
    namespace si = internal;
    using itraits = std::iterator_traits<FutureIterator>;
    using result_transform = si::identity_futures_vector<typename itraits::value_type>;
    return si::do_when_all<result_transform>(std::move(begin), std::move(end));
}

template <typename T, bool IsFuture>
struct reducer_with_get_traits;

template <typename T>
struct reducer_with_get_traits<T, false> {
    using result_type = decltype(std::declval<T>().get());
    using future_type = future<result_type>;
    static future_type maybe_call_get(future<> f, lw_shared_ptr<T> r) {
        return f.then([r = std::move(r)] () mutable {
            return make_ready_future<result_type>(std::move(*r).get());
        });
    }
};

template <typename T>
struct reducer_with_get_traits<T, true> {
    using future_type = decltype(std::declval<T>().get());
    static future_type maybe_call_get(future<> f, lw_shared_ptr<T> r) {
        return f.then([r = std::move(r)] () mutable {
            return r->get();
        }).then_wrapped([r] (future_type f) {
            return f;
        });
    }
};

template <typename T, typename V = void>
struct reducer_traits {
    using future_type = future<>;
    static future_type maybe_call_get(future<> f, lw_shared_ptr<T> r) {
        return f.then([r = std::move(r)] {});
    }
};

template <typename T>
struct reducer_traits<T, decltype(std::declval<T>().get(), void())> : public reducer_with_get_traits<T, is_future<std::result_of_t<decltype(&T::get)(T)>>::value> {};


template <typename Iterator, typename Mapper, typename Reducer>
inline
auto
map_reduce(Iterator begin, Iterator end, Mapper&& mapper, Reducer&& r)
    -> typename reducer_traits<Reducer>::future_type
{
    auto r_ptr = make_lw_shared(std::forward<Reducer>(r));
    future<> ret = make_ready_future<>();
    using futurator = futurize<decltype(mapper(*begin))>;
    while (begin != end) {
        ret = futurator::apply(mapper, *begin++).then_wrapped([ret = std::move(ret), r_ptr] (auto f) mutable {
            return ret.then_wrapped([f = std::move(f), r_ptr] (auto rf) mutable {
                if (rf.failed()) {
                    f.ignore_ready_future();
                    return std::move(rf);
                } else {
                    return futurize<void>::apply(*r_ptr, std::move(f.get()));
                }
            });
        });
    }
    return reducer_traits<Reducer>::maybe_call_get(std::move(ret), r_ptr);
}


template <typename Iterator, typename Mapper, typename Initial, typename Reduce>
GCC6_CONCEPT( requires requires (Iterator i, Mapper mapper, Initial initial, Reduce reduce) {
    *i++;
    { i != i } -> std::same_as<bool>;//为什么?
    mapper(*i);
    requires is_future<decltype(mapper(*i))>::value;
    { reduce(std::move(initial), mapper(*i).get0()) } -> std::same_as<Initial>;
} )
inline
future<Initial>
map_reduce(Iterator begin, Iterator end, Mapper&& mapper, Initial initial, Reduce reduce) {
    struct state {
        Initial result;
        Reduce reduce;
    };
    auto s = make_lw_shared(state{std::move(initial), std::move(reduce)});
    future<> ret = make_ready_future<>();
    using futurator = futurize<decltype(mapper(*begin))>;
    while (begin != end) {
        ret = futurator::apply(mapper, *begin++).then_wrapped([s = s.get(), ret = std::move(ret)] (auto f) mutable {
            try {
                s->result = s->reduce(std::move(s->result), std::move(f.get0()));
                return std::move(ret);
            } catch (...) {
                return std::move(ret).then_wrapped([ex = std::current_exception()] (auto f) {
                    f.ignore_ready_future();
                    return make_exception_future<>(ex);
                });
            }
        });
    }
    return ret.then([s] {
        return make_ready_future<Initial>(std::move(s->result));
    });
}

template <typename Range, typename Mapper, typename Initial, typename Reduce>
GCC6_CONCEPT( requires requires (Range range, Mapper mapper, Initial initial, Reduce reduce) {
     std::begin(range);
     std::end(range);
     mapper(*std::begin(range));
     requires is_future<std::remove_reference_t<decltype(mapper(*std::begin(range)))>>::value;
    { reduce(std::move(initial), mapper(*std::begin(range)).get0()) } -> std::same_as<Initial>;
} )
inline
future<Initial>
map_reduce(Range&& range, Mapper&& mapper, Initial initial, Reduce reduce) {
    return map_reduce(std::begin(range), std::end(range), std::forward<Mapper>(mapper),
            std::move(initial), std::move(reduce));
}

template <typename Result, typename Addend = Result>
class adder {
private:
    Result _result;
public:
    future<> operator()(const Addend& value) {
        _result += value;
        return make_ready_future<>();
    }
    Result get() && {
        return std::move(_result);
    }
};

static inline future<> now() {
    return make_ready_future<>();
}

future<> later(){
    promise<> p;
    auto f = p.get_future();
    engine().force_poll(); //把need_preempted改为true(这句是没有意义的)
    ::schedule_normal(make_task([p = std::move(p)]() mutable {
        p.set_value(); // 这段代码把一个p.set_value封装为一个task加到调度器中.
    }));
    return f;
}



struct default_timeout_exception_factory {
    static auto timeout() {
        return timed_out_error();
    }
};

template<typename ExceptionFactory = default_timeout_exception_factory, typename Clock, typename Duration, typename... T>
future<T...> with_timeout(std::chrono::time_point<Clock, Duration> timeout, future<T...> f) {
    if (f.available()) {
        return f;
    }
    auto pr = std::make_unique<promise<T...>>();
    auto result = pr->get_future();
    timer<Clock> timer([&pr = *pr] {
        pr.set_exception(std::make_exception_ptr(ExceptionFactory::timeout()));
    });
    timer.arm(timeout);
    f.then_wrapped([pr = std::move(pr), timer = std::move(timer)] (auto&& f) mutable {
        if (timer.cancel()) {
            f.forward_to(std::move(*pr));
        } else {
            f.ignore_ready_future();
        }
    });
    return result;
}

namespace internal {
template<typename Future>
struct future_has_value {
    enum {
        value = !std::is_same<std::decay_t<Future>, future<>>::value
    };
};

template<typename Tuple>
struct tuple_to_future;
template<typename... Elements>
struct tuple_to_future<std::tuple<Elements...>> {
    using type = future<Elements...>;
    using promise_type = promise<Elements...>;
    static auto make_ready(std::tuple<Elements...> t) {
        auto create_future = [] (auto&&... args) {
            return make_ready_future<Elements...>(std::move(args)...);
        };
        return apply(create_future, std::move(t));
    }
    static auto make_failed(std::exception_ptr excp) {
        return make_exception_future<Elements...>(std::move(excp));
    }
};

template<typename... Futures>
class extract_values_from_futures_tuple {
    static auto transform(std::tuple<Futures...> futures) {
        auto prepare_result = [] (auto futures) {
            auto fs = tuple_filter_by_type<internal::future_has_value>(std::move(futures));
            return tuple_map(std::move(fs), [] (auto&& e) {
                return internal::untuple(e.get());
            });
        };
        using tuple_futurizer = internal::tuple_to_future<decltype(prepare_result(std::move(futures)))>;
        std::exception_ptr excp;
        tuple_for_each(futures, [&excp] (auto& f) {
            if (!excp) {
                if (f.failed()) {
                    excp = f.get_exception();
                }
            } else {
                f.ignore_ready_future();
            }
        });
        if (excp) {
            return tuple_futurizer::make_failed(std::move(excp));
        }
        return tuple_futurizer::make_ready(prepare_result(std::move(futures)));
    }
public:
    using future_type = decltype(transform(std::declval<std::tuple<Futures...>>()));
    using promise_type = typename future_type::promise_type;
    static void set_promise(promise_type& p, std::tuple<Futures...> tuple) {
        transform(std::move(tuple)).forward_to(std::move(p));
    }
};

template<typename Future>
struct extract_values_from_futures_vector {
    using value_type = decltype(untuple(std::declval<typename Future::value_type>()));
    using future_type = future<std::vector<value_type>>;
    static future_type run(std::vector<Future> futures) {
        std::vector<value_type> values;
        values.reserve(futures.size());

        std::exception_ptr excp;
        for (auto&& f : futures) {
            if (!excp) {
                if (f.failed()) {
                    excp = f.get_exception();
                } else {
                    values.emplace_back(untuple(f.get()));
                }
            } else {
                f.ignore_ready_future();
            }
        }
        if (excp) {
            return make_exception_future<std::vector<value_type>>(std::move(excp));
        }
        return make_ready_future<std::vector<value_type>>(std::move(values));
    }
};

template<>
struct extract_values_from_futures_vector<future<>> {
    using future_type = future<>;

    static future_type run(std::vector<future<>> futures) {
        std::exception_ptr excp;
        for (auto&& f : futures) {
            if (!excp) {
                if (f.failed()) {
                    excp = f.get_exception();
                }
            } else {
                f.ignore_ready_future();
            }
        }
        if (excp) {
            return make_exception_future<>(std::move(excp));
        }
        return make_ready_future<>();
    }
};

}
template<typename... Futures>
GCC6_CONCEPT( requires AllAreFutures<Futures...> )
inline auto when_all_succeed(Futures&&... futures) {
    using state = internal::when_all_state<internal::extract_values_from_futures_tuple<Futures...>, Futures...>;
    auto s = make_lw_shared<state>(std::forward<Futures>(futures)...);
    return s->wait_all(std::make_index_sequence<sizeof...(Futures)>());
}

template <typename FutureIterator, typename = typename std::iterator_traits<FutureIterator>::value_type>
GCC6_CONCEPT( requires requires (FutureIterator i) {
    *i++;
    {i!= i} ->std::same_as<bool>;
     requires is_future<std::remove_reference_t<decltype(*i)>>::value;
})

inline auto when_all_succeed(FutureIterator begin, FutureIterator end) {
    using itraits = std::iterator_traits<FutureIterator>;
    using result_transform = internal::extract_values_from_futures_vector<typename itraits::value_type>;
    return internal::do_when_all<result_transform>(std::move(begin), std::move(end));
}
// Define the static member
thread_local thread* thread::_current = nullptr;
// Implementation of global functions
void enable_timer(steady_clock_type::time_point when)
{
    engine().enable_timer(when);
}
#endif
















/*0000000000000000000000000000000000000000000000000000000000*/
/*0000000000000000000000000000000000000000000000000000000000*/



void reactor::enable_timer(steady_clock_type::time_point when) {
    itimerspec its;
    its.it_interval = {};
    its.it_value = to_timespec(when);
    auto ret = timer_settime(_steady_clock_timer, TIMER_ABSTIME, &its, NULL);
    // throw_system_error_on(ret == -1);
}


/*  信号触发​:默认情况下，定时器到期会发送一个信号（如 SIGALRM）到进程。进程可以通过信号处理函数来处理定时器到期事件. */

void reactor::add_timer(steady_timer *tmr) {
    std::cout<<"reactor add timer"<<std::endl;
    if (queue_timer(tmr)) {
        enable_timer(_timers.get_next_timeout());
    }
}
void reactor::add_timer(lowres_timer* tmr) {
    if (queue_timer(tmr)) {
        _lowres_next_timeout = _lowres_timers.get_next_timeout();
    }
}

void reactor::add_timer(manual_timer* tmr) {
    queue_timer(tmr);
}
bool reactor::queue_timer(lowres_timer* tmr) {
    return _lowres_timers.insert(*tmr);
}

bool reactor::queue_timer(manual_timer* tmr) {
    return _manual_timers.insert(*tmr);
}

bool reactor::queue_timer(steady_timer* tmr) {
    std::cout<<"reaactor queue timer"<<std::endl;
    return _timers.insert(*tmr);
}


/*
    del_timer什么时候调用?
*/
void reactor::del_timer(steady_timer* tmr) {
    if (tmr->_expired) {
        _expired_timers.erase(tmr->expired_it);  // 直接使用保存的迭代器
        tmr->_expired = false;
    } else {
        _timers.remove(*tmr);  // 通过 it 成员快速删除
    }
}

// 同理修改其他 del_timer 函数：
void reactor::del_timer(lowres_timer* tmr) {
    if (tmr->_expired) {
        _expired_lowres_timers.erase(tmr->expired_it);
        tmr->_expired = false;
    } else {
        _lowres_timers.remove(*tmr);
    }
}

void reactor::del_timer(manual_timer* tmr) {
    if (tmr->_expired) {
        _expired_manual_timers.erase(tmr->expired_it);
        tmr->_expired = false;
    } else {
        _manual_timers.remove(*tmr);
    }
}



template<typename Func>
future<> smp::invoke_on_all(Func&& func) {
        static_assert(std::is_same<future<>, typename futurize<std::result_of_t<Func()>>::type>::value, "bad Func signature");
        return parallel_for_each(all_cpus(), [&func] (unsigned id) {
            return smp::submit_to(id, Func(func));
        });
}




template <typename Func>
futurize_t<std::result_of_t<Func()>> smp::submit_to(unsigned t, Func&& func) {
        using ret_type = std::result_of_t<Func()>;
        if (t == engine().cpu_id()) {
            try {
                if (!is_future<ret_type>::value) {
                    // Non-deferring function, so don't worry about func lifetime
                    return futurize<ret_type>::apply(std::forward<Func>(func));
                } else if (std::is_lvalue_reference<Func>::value) {
                    // func is an lvalue, so caller worries about its lifetime
                    return futurize<ret_type>::apply(func);
                } else {
                    // Deferring call on rvalue function, make sure to preserve it across call
                    auto w = std::make_unique<std::decay_t<Func>>(std::move(func));
                    auto ret = futurize<ret_type>::apply(*w);
                    return ret.finally([w = std::move(w)] {});
                }
            } catch (...) {
                // Consistently return a failed future rather than throwing, to simplify callers
                return futurize<std::result_of_t<Func()>>::make_exception_future(std::current_exception());
            }
        } else {
            // 这里是修复的地方
            if (_qs != nullptr) {
                return _qs[t][engine().cpu_id()].submit(std::forward<Func>(func));
            } else {
                return futurize<std::result_of_t<Func()>>::make_exception_future(std::runtime_error("smp::_qs is null"));
            }
        }
}




template <typename... T>
void future_state<T...>::forward_to(promise<T...>& pr) noexcept{
    assert(_state != state::future);
    if (_state == state::exception) {
        pr.set_urgent_exception(std::move(_u.ex));
        _u.ex.~exception_ptr();
    } else {
        pr.set_urgent_value(std::move(_u.value));
        _u.value.~tuple();
    }
    _state = state::invalid;
}

void engine_exit(std::exception_ptr eptr) {
    if (!eptr) {
        engine().exit(0);
        return;
    }
    std::cout<<"Exception: "<< std::endl;
    engine().exit(1);
}




void future_state<>::forward_to(promise<>& pr) noexcept {
    assert(_u.st != state::future && _u.st != state::invalid);
    if (_u.st >= state::exception_min) {
        pr.set_urgent_exception(std::move(_u.ex));
        _u.ex.~exception_ptr();
    } else {
        pr.set_urgent_value(std::tuple<>());
    }
    _u.st = state::invalid;
}


bool thread::try_run_one_yielded_thread() {
    if (thread_context::_preempted_threads.empty()) {
        return false;
    }
    auto* t = thread_context::_preempted_threads.front();
    t->_sched_timer.cancel();
    t->_sched_promise_ptr->set_value();
    thread_context::_preempted_threads.pop_front();
    return true;
}
thread_scheduling_group::thread_scheduling_group(std::chrono::nanoseconds period, float usage)
        : _period(period), _quota(std::chrono::duration_cast<std::chrono::nanoseconds>(usage * period)) {
}

void thread_scheduling_group::account_start() {
    auto now = thread_clock::now();
    if (now >= _this_period_ends) {
        _this_period_ends = now + _period;
        _this_period_remain = _quota;
    }
    _this_run_start = now;
}

void thread_scheduling_group::account_stop() {
    _this_period_remain -= thread_clock::now() - _this_run_start;
}

std::chrono::steady_clock::time_point*
thread_scheduling_group::next_scheduling_point() const {
    auto now = thread_clock::now();
    auto current_remain = _this_period_remain - (now - _this_run_start);
    if (current_remain > std::chrono::nanoseconds(0)) {
        return nullptr;
    }
    static std::chrono::steady_clock::time_point result;
    result = _this_period_ends - current_remain;
    return &result;
}

// Constructor that takes a callable object
template <typename Func>
thread::thread(Func func) : thread(thread_attributes(), std::move(func)){}

// Constructor that takes thread attributes and a callable object
template <typename Func>
thread::thread(thread_attributes attr, Func func)
    : _context(std::make_unique<thread_context>(std::move(attr), std::move(func))) {}
    /*
        因为context是使用unique_ptr管理,所以当退出作用域时，unique会析构到，在析构时自动释放管理的内存.
    */


void thread::yield() {
    thread_impl::get()->yield();
}

bool thread::should_yield() {
    return thread_impl::get()->should_yield();
}


// Destructor
thread::~thread() {
    assert(!_context || _context->_joined);
}

void reactor::expire_manual_timers() {
    complete_timers(engine()._manual_timers, engine()._expired_manual_timers, []{
        std::cout<<"到期"<<std::endl;
    });
}

void manual_clock::expire_timers() {
    engine().expire_manual_timers();
}

inline void jmp_buf_link::initial_switch_in(ucontext_t* initial_context, const void*, size_t)
{
    if(g_current_context){
        std::cout<<"g_current_context非空"<<std::endl;
    }else{
        std::cout<<"g_current_context不空"<<std::endl;
    }
    auto prev = std::exchange(g_current_context, this);
    link = prev;
    if (setjmp(prev->jmpbuf) == 0) {
        std::cout<<"init setjmp"<<std::endl;
        //  如果第一次setjmp
        setcontext(initial_context);  
        //  这里会跳转到initial_context的入口函数中去执行.
        //  使用setcontext而不是longjmp，需要设置完整的初始上下文.
    }
    /*
    在这个过程中,已经执行完了绑定在线程上的回调函数。
    */
    std::cout<<"final long jmp"<<std::endl;
}


inline void jmp_buf_link::switch_in()
{
    auto prev = std::exchange(g_current_context, this);
    link = prev;
    if (setjmp(prev->jmpbuf) == 0) {
        longjmp(jmpbuf, 1);
    }
}

inline void jmp_buf_link::switch_out(){
    g_current_context = link;
    if (setjmp(jmpbuf) == 0) {
        longjmp(g_current_context->jmpbuf, 1);
    }
}

inline void jmp_buf_link::initial_switch_in_completed(){}

inline void jmp_buf_link::final_switch_out(){
    g_current_context = link;//link就是该context对应的上一个context(恢复).
    std::cout<<"final_switch_out"<<std::endl;
    longjmp(g_current_context->jmpbuf, 1);//使用longjmp跳转到当前context的jmpbuf
    //这个可能没用？
}

thread_context::~thread_context() {
    std::cout<<"开始析构thread_context"<<std::endl;
    _all_threads.erase(_all_it);//为什么？
}


void thread_context::yield() {
    if (!_attr.scheduling_group) {
        later().get();
    } 
    else
    {
        std::cout<<"yield 有scheduling group"<<std::endl;
        auto when = _attr.scheduling_group->next_scheduling_point();
        if (when) {
            _preempted_it = _preempted_threads.insert(_preempted_threads.end(), this);
            set_sched_promise();
            auto fut = get_sched_promise()->get_future();
            _sched_timer.arm(*when);
            fut.get();
            clear_sched_promise();
        } else if (need_preempt()) {
            later().get();
        }
    }
}

void thread_context::reschedule() {
    _preempted_threads.erase(_preempted_it);
    _sched_promise_ptr->set_value();
}

void thread_context::s_main(unsigned int lo, unsigned int hi) {
    uintptr_t q = lo | (uint64_t(hi) << 32);
    std::cout<<"执行s_main"<<std::endl;
    reinterpret_cast<thread_context*>(q)->main();
}

void
thread_context::main() {
    _context.initial_switch_in_completed();//这里什么都没有执行.
    if (_attr.scheduling_group) {
        std::cout<<"attr有scheduling group"<<std::endl;
        _attr.scheduling_group->account_start();
        //没有执行到这里.
    }
    try {
        std::cout<<"开始执行回调函数"<<std::endl;
        _func();            //执行线程绑定在context的函数.
        _done.set_value(); // done的类型是promise<>，set_value把done对应的future_state<>状态设置为result.
    } catch (...) {
        _done.set_exception(std::current_exception());
    }
    if (_attr.scheduling_group) {
        _attr.scheduling_group->account_stop();
    }
    _context.final_switch_out();
}

smp_message_queue::smp_message_queue(reactor* from, reactor* to) : _pending(to),_completed(from){ }

void smp_message_queue::stop() {
    // _metrics.clear();
}
void smp_message_queue::move_pending() {
    auto begin = _tx.a.pending_fifo.cbegin();
    auto end = _tx.a.pending_fifo.cend();
    end = _pending.push(begin, end);
    if (begin == end) {
        return;
    }
    auto nr = end - begin;
    _pending.maybe_wakeup();
    _tx.a.pending_fifo.erase(begin, end);
    _current_queue_length += nr;
    _last_snt_batch = nr;
    _sent += nr;
}

bool smp_message_queue::pure_poll_tx() const {
    // can't use read_available(), not available on older boost
    // empty() is not const, so need const_cast.
    return !const_cast<lf_queue&>(_completed).empty();
}

void smp_message_queue::submit_item(std::unique_ptr<smp_message_queue::work_item> item) {
    _tx.a.pending_fifo.push_back(item.get());
    item.release();
    if (_tx.a.pending_fifo.size() >= batch_size) {
        move_pending();
    }
}

void smp_message_queue::respond(work_item* item) {
    _completed_fifo.push_back(item);
    if (_completed_fifo.size() >= batch_size || engine()._stopped) {
        flush_response_batch();
    }
}

void smp_message_queue::flush_response_batch() {
    if (!_completed_fifo.empty()) {
        auto begin = _completed_fifo.cbegin();
        auto end = _completed_fifo.cend();
        end = _completed.push(begin, end);
        if (begin == end) {
            return;
        }
        _completed.maybe_wakeup();
        _completed_fifo.erase(begin, end);
    }
}

bool smp_message_queue::has_unflushed_responses() const {
    return !_completed_fifo.empty();
}

bool smp_message_queue::pure_poll_rx() const {
    // can't use read_available(), not available on older boost
    // empty() is not const, so need const_cast.
    return !const_cast<lf_queue&>(_pending).empty();
}

void
smp_message_queue::lf_queue::maybe_wakeup() {
    // Called after lf_queue_base::push().
    //
    // This is read-after-write, which wants memory_order_seq_cst,
    // but we insert that barrier using systemwide_memory_barrier()
    // because seq_cst is so expensive.
    //
    // However, we do need a compiler barrier:
    std::atomic_signal_fence(std::memory_order_seq_cst);
    if (remote->_sleeping.load(std::memory_order_relaxed)) {
        // We are free to clear it, because we're sending a signal now
        remote->_sleeping.store(false, std::memory_order_relaxed);
        remote->wakeup();
    }
}

template<size_t PrefetchCnt, typename Func>
size_t smp_message_queue::process_queue(lf_queue& q, Func process) {
    // copy batch to local memory in order to minimize
    // time in which cross-cpu data is accessed
    work_item* items[queue_length + PrefetchCnt];
    work_item* wi;
    if (!q.pop(wi))
        return 0;
    // start prefecthing first item before popping the rest to overlap memory
    // access with potential cache miss the second pop may cause
    prefetch<2>(wi);
    auto nr = q.pop(items);
    std::fill(std::begin(items) + nr, std::begin(items) + nr + PrefetchCnt, nr ? items[nr - 1] : wi);
    unsigned i = 0;
    do {
        prefetch_n<2>(std::begin(items) + i, std::begin(items) + i + PrefetchCnt);
        process(wi);
        wi = items[i++];
    } while(i <= nr);
    return nr + 1;
}

size_t smp_message_queue::process_completions() {
    auto nr = process_queue<prefetch_cnt*2>(_completed, [] (work_item* wi) {
        wi->complete();
        delete wi;
    });
    _current_queue_length -= nr;
    _compl += nr;
    _last_cmpl_batch = nr;
    return nr;
}

void smp_message_queue::flush_request_batch() {
    if (!_tx.a.pending_fifo.empty()) {
        move_pending();
    }
}

size_t smp_message_queue::process_incoming() {
    auto nr = process_queue<prefetch_cnt>(_pending, [this] (work_item* wi) {
        wi->process().then([this, wi] {
            respond(wi);
        });
    });
    _received += nr;
    _last_rcv_batch = nr;
    return nr;
}

void smp_message_queue::start(unsigned cpuid) {
    _tx.init();
    char instance[10];
    std::snprintf(instance, sizeof(instance), "%u-%u", engine().cpu_id(), cpuid);
}




// // 实现maybe_wakeup方法
// void smp_message_queue::lf_queue::maybe_wakeup() {
//     // 在调用lf_queue_base::push()之后调用
    
//     // 这是读后写操作，通常需要memory_order_seq_cst，
//     // 但我们使用systemwide_memory_barrier()插入该屏障，
//     // 因为seq_cst成本很高。
    
//     // 然而，我们确实需要一个编译器屏障：
//     std::atomic_signal_fence(std::memory_order_seq_cst);
//     if (remote->_sleeping.load(std::memory_order_relaxed)) {
//         // 我们可以自由地清除它，因为我们现在正在发送信号
//         remote->_sleeping.store(false, std::memory_order_relaxed);
//         remote->wakeup();
//     }
// }

// // 实现pure_poll_tx方法
// bool smp_message_queue::pure_poll_tx() const {
//     // 检查完成队列是否为空
//     return !_completed.empty();
// }

// // 实现pure_poll_rx方法 
// bool smp_message_queue::pure_poll_rx() const {
//     // 检查挂起队列是否为空
//     return !_pending.empty();
// }



// bool reactor::do_expire_lowres_timers() {
//     if (engine()._lowres_next_timeout == lowres_clock::time_point()) {
//         return false;
//     }
//     auto now = lowres_clock::now();
//     if (now > engine()._lowres_next_timeout) {
//         complete_timers(engine()._lowres_timers, engine()._expired_lowres_timers, [] {
//             if (!engine()._lowres_timers.empty()) {
//                 engine()._lowres_next_timeout = engine()._lowres_timers.get_next_timeout();
//             } else {
//                 engine()._lowres_next_timeout = lowres_clock::time_point();
//             }
//         });
//         return true;
//     }
//     return false;
// }

bool reactor::do_check_lowres_timers() const{
    if (engine()._lowres_next_timeout == lowres_clock::time_point()) {
        return false;
    }
    return lowres_clock::now() > engine()._lowres_next_timeout;
}

thread_context::stack_holder
thread_context::make_stack() {
    auto stack = stack_holder(new char[_stack_size]);
    return stack;
}

thread_context::thread_context(thread_attributes attr, std::function<void ()> func)
        : _attr(std::move(attr))
        , _func(std::move(func)) {
    setup();
    std::cout<<"添加this到all_threads"<<std::endl;
    _all_threads.push_front(this);
    _all_it = _all_threads.begin();
    //为什么这里是this,而不是*this，而不是_all_it?思考
    //因为_all_threads存放的就是thread_context*，所以添加的也是指针。
}

template <typename T>
template <typename Arg>
inline
future<T>
futurize<T>::make_exception_future(Arg&& arg) {
    return ::make_exception_future<T>(std::forward<Arg>(arg));
}

template <typename... T>
template <typename Arg>
inline
future<T...>
futurize<future<T...>>::make_exception_future(Arg&& arg) {
    return ::make_exception_future<T...>(std::forward<Arg>(arg));
}

template <typename Arg>
inline
future<>
futurize<void>::make_exception_future(Arg&& arg) {
    return ::make_exception_future<>(std::forward<Arg>(arg));
}

template <typename T>
inline
future<T>
futurize<T>::from_tuple(std::tuple<T>&& value) {
    return make_ready_future<T>(std::move(value));
}

template <typename T>
inline
future<T>
futurize<T>::from_tuple(const std::tuple<T>& value) {
    return make_ready_future<T>(value);
}
inline future<> futurize<void>::from_tuple(std::tuple<>&& value) {
    return make_ready_future<>();
}

inline future<> futurize<void>::from_tuple(const std::tuple<>& value) {
    return make_ready_future<>();
}



void thread_context::stack_deleter::operator()(char* ptr) const noexcept {
    delete[] ptr;
}

void
thread_context::setup() {
    // use setcontext() for the initial jump, as it allows us
    // to set up a stack, but continue with longjmp() as it's much faster.
    ucontext_t initial_context;
    auto q = uint64_t(reinterpret_cast<uintptr_t>(this));//将thread_
    auto main = reinterpret_cast<void (*)()>(&thread_context::s_main);
    auto r = getcontext(&initial_context);//保存当前上下文到initial_context中.
    // throw_system_error_on(r == -1);
    initial_context.uc_stack.ss_sp = _stack.get(); //设置栈空间
    initial_context.uc_stack.ss_size = _stack_size;
    initial_context.uc_link = nullptr;
    makecontext(&initial_context, main, 2, int(q), int(q >> 32));  //makecontext前32位，后32位.
    _context.thread = this;//_context是jmp_buf_link类型.(绑定父类型)
    _context.initial_switch_in(&initial_context, _stack.get(), _stack_size);//进入这个函数准备执行了s_main
    std::cout<<"执行完了回调函数"<<std::endl;
}

void thread_context::switch_in() {
    if (_attr.scheduling_group) {
        _attr.scheduling_group->account_start();
        _context.set_yield_at(_attr.scheduling_group->_this_run_start + _attr.scheduling_group->_this_period_remain);
    } else {
        _context.clear_yield_at();//设置_context的yield_为false
    }
    _context.switch_in();
}

void thread_context::switch_out() {
    if (_attr.scheduling_group) {
        _attr.scheduling_group->account_stop();
    }
    _context.switch_out();
}

bool thread_context::should_yield() const {
    if (!_attr.scheduling_group) {
        return need_preempt();
    }
    return need_preempt() || bool(_attr.scheduling_group->next_scheduling_point());
}














signals::signals() : _pending_signals(0) {
}

signals::~signals() {
    sigset_t mask;
    sigfillset(&mask);
    ::pthread_sigmask(SIG_BLOCK, &mask, NULL);
}

signals::signal_handler::signal_handler(int signo, std::function<void ()>&& handler)
        : _handler(std::move(handler)) {
            std::cout<<"调用signal_handler"<<std::endl;
    struct sigaction sa;
    sa.sa_sigaction = action;//这个是信号处理函数.
    sa.sa_mask = make_empty_sigset_mask();
    sa.sa_flags = SA_SIGINFO | SA_RESTART;
    engine()._signals._pending_signals.fetch_or(1ull << signo, std::memory_order_relaxed);
    std::cout<<"engine().signals"<<engine()._signals._pending_signals<<std::endl;
    auto r = ::sigaction(signo, &sa, nullptr);
    // throw_system_error_on(r == -1);
    auto mask = make_sigset_mask(signo);
    r = ::pthread_sigmask(SIG_UNBLOCK, &mask, NULL);
    throw_pthread_error(r);
}

void signals::handle_signal(int signo, std::function<void ()>&& handler) {
    std::cout<<"handle signal"<<std::endl;
    _signal_handlers.emplace(std::piecewise_construct,std::make_tuple(signo), std::make_tuple(signo, std::move(handler)));
    //插入singo,和对应的handler.
}

void signals::handle_signal_once(int signo, std::function<void ()>&& handler) {
    return handle_signal(signo, [fired = false, handler = std::move(handler)] () mutable {
        if (!fired) {
            fired = true;
            handler();
        }
    });
}

bool signals::poll_signal() {
    auto signals = _pending_signals.load(std::memory_order_relaxed);
    //为什么一直是0.

    if (signals) {
            std::cout<<"signals "<<signals<<std::endl;
        _pending_signals.fetch_and(~signals, std::memory_order_relaxed);
        for (size_t i = 0; i < sizeof(signals)*8; i++) {
            // std::cout<<"遍历"<<std::endl;
            if (signals & (1ull << i)) {
                //执行handler
                std::cout<<"执行handler"<<std::endl;
               _signal_handlers.at(i)._handler();
            }
        }
    }
    return signals;
}
bool signals::pure_poll_signal() const {
    return _pending_signals.load(std::memory_order_relaxed);
}

void signals::action(int signo, siginfo_t* siginfo, void* ignore) {
    std::cout<<"action########"<<std::endl;
    engine()._signals._pending_signals.fetch_or(1ull << signo, std::memory_order_relaxed);
}




/// \brief Waits for the future to become available
/// This method blocks the current thread until the future becomes available.
template <typename... T>
void future<T...>::wait() {
    std::cout<<"future wait"<<std::endl;
    auto thread = thread_impl::get();
    assert(thread);//这里报错.

    schedule([this, thread] (future_state<T...>&& new_state) {
        *state() = std::move(new_state);
        thread_impl::switch_in(thread);
    });
    thread_impl::switch_out(thread);
}

template <typename... T>
[[gnu::always_inline]]
std::tuple<T...> future<T...>::get() {
    if (!state()->available()) {
        std::cout<<"future.get  调用这里的wait"<<std::endl;
        wait();
    } else if (thread_impl::get() && thread_impl::should_yield()) {
        std::cout<<"future.get  should_yield"<<std::endl;
        thread_impl::yield();
    }
    return get_available_state().get();
}


// Join function
future<> thread::join() {
    _context->_joined = true;
    return _context->_done.get_future();
}



template <typename T, typename E, typename EnableFunc>
void reactor::complete_timers(T& timers, E& expired_timers, EnableFunc&& enable_fn) {
    expired_timers = timers.expire(timers.now()); // 获取过期的定时器
    std::cout << "Expired " << expired_timers.size() << " timers" << std::endl;
    // 处理所有过期定时器
    for (auto* timer_ptr : expired_timers) {
        if (timer_ptr) {
            std::cout << "Marking timer " << " as expired" << std::endl;
            timer_ptr->_expired = true;
        }
    }
    // arm表示定时器是否有一个过期时间.
    while (!expired_timers.empty()) {
        auto* timer_ptr = expired_timers.front();
        expired_timers.pop_front();
        if (timer_ptr) {
            std::cout << "Processing timer "<<std::endl;
            timer_ptr->_queued = false;
            if (timer_ptr->_armed) {
                timer_ptr->_armed = false;
                if (timer_ptr->_period) {
                    // std::cout << "Re-adding periodic timer " << timer_ptr->_timerid << std::endl;
                    timer_ptr->readd_periodic();//周期定时器

                }
                try {
                    std::cout << "Executing timer callback for " << std::endl;
                    timer_ptr->_callback();
                } catch (const std::exception& e) {
                    // std::cerr << "Timer " << timer_ptr->_timerid << " callback failed: " << e.what() << std::endl;
                } catch (...) {
                    // std::cerr << "Timer " << timer_ptr->_timerid << " callback failed with unknown error" << std::endl;
                }
            }
        }
    }
    
    enable_fn();
}

static decltype(auto) install_signal_handler_stack() {
    size_t size = SIGSTKSZ;
    auto mem = std::make_unique<char[]>(size);
    stack_t stack;
    stack_t prev_stack;
    stack.ss_sp = mem.get();
    stack.ss_flags = 0;
    stack.ss_size = size;
    auto r = sigaltstack(&stack, &prev_stack);
    assert(r == 0);
    // throw_system_error_on(r == -1);
    return defer([mem = std::move(mem), prev_stack] () mutable {
        try {
            auto r = sigaltstack(&prev_stack, NULL);
            // throw_system_error_on(r == -1);
            assert(r == 0);
        } catch (...) {
            mem.release(); // We failed to restore previous stack, must leak it.
            std::cout<<< "Failed to restore previous signal stack" << std::endl;
        }
    });
}


bool
reactor::poll_once() {
    bool work = false;
    for (auto c : _pollers) {
        work |= c->poll();
    }

    return work;
}


int reactor::run(){
    auto signal_stack = install_signal_handler_stack();
    poller io_poller(std::make_unique<io_pollfn>(*this));
    poller sig_poller(std::make_unique<signal_pollfn>(*this));
    poller aio_poller(std::make_unique<aio_batch_submit_pollfn>(*this));
    poller batch_flush_poller(std::make_unique<batch_flush_pollfn>(*this));
    poller execution_stage_poller(std::make_unique<execution_stage_pollfn>());
    start_aio_eventfd_loop();
    if (_id == 0) {
       if (_handle_sigint) {
          _signals.handle_signal_once(SIGINT, [this] { stop(); });
       }
       _signals.handle_signal_once(SIGTERM, [this] { stop(); });
    }
    _signals.handle_signal(alarm_signal(), [this] {
        complete_timers(_timers, _expired_timers, [this] {
        if (!_timers.empty()) {
                // std::cout << "Enabling timer for " << _timers.get_next_timeout()<<std::endl;//这行是有执行的.
                enable_timer(_timers.get_next_timeout());
            }
        });
    });
    _cpu_started.wait(smp::count).then([this] {
        _network_stack->initialize().then([this] {
            _start_promise.set_value();
        });
    });
    _network_stack_ready_promise.get_future().then([this] (std::unique_ptr<network_stack> stack) {
        _network_stack = std::move(stack);
        for (unsigned c = 0; c < smp::count; c++) {
            smp::submit_to(c, [] {
                    engine()._cpu_started.signal();
            });
        }
    });
    // Register smp queues poller
    std::optional<poller> smp_poller;
    if (smp::count > 1) {
        smp_poller = poller(std::make_unique<smp_pollfn>(*this));
    }
    poller syscall_poller(std::make_unique<syscall_pollfn>(*this));
    _signals.handle_signal(alarm_signal(), [this] {
        complete_timers(_timers, _expired_timers, [this] {
            if (!_timers.empty()) {
                enable_timer(_timers.get_next_timeout());
            }
        });
    });
    poller drain_cross_cpu_freelist(std::make_unique<drain_cross_cpu_freelist_pollfn>());
    poller expire_lowres_timers(std::make_unique<lowres_timer_pollfn>(*this));
    using namespace std::chrono_literals;
    timer<lowres_clock> load_timer;
    auto last_idle = _total_idle;
    auto idle_start = steady_clock_type::now(), idle_end = idle_start;
    load_timer.set_callback([this, &last_idle, &idle_start, &idle_end] () mutable {
        _total_idle += idle_end - idle_start;
        auto load = double((_total_idle - last_idle).count()) / double(std::chrono::duration_cast<steady_clock_type::duration>(1s).count());
        last_idle = _total_idle;
        load = std::min(load, 1.0);
        idle_start = idle_end;
        _loads.push_front(load);
        if (_loads.size() > 5) {
            auto drop = _loads.back();
            _loads.pop_back();
            _load -= (drop/5);
        }
        _load += (load/5);
    });
    load_timer.arm_periodic(1s);
    itimerspec its = posix::to_relative_itimerspec(_task_quota, _task_quota);
    _task_quota_timer.timerfd_settime(0, its);
    auto& task_quote_itimerspec = its;
    struct sigaction sa_block_notifier = {};
    sa_block_notifier.sa_handler = &reactor::block_notifier;
    sa_block_notifier.sa_flags = SA_RESTART;
    auto r = sigaction(block_notifier_signal(), &sa_block_notifier, nullptr);
    assert(r == 0);
    bool idle = false;
    std::function<bool()> check_for_work = [this] () {
        return poll_once() || !_pending_tasks.empty() || thread::try_run_one_yielded_thread();
    };
    std::function<bool()> pure_check_for_work = [this] () {
        return pure_poll_once() || !_pending_tasks.empty() || thread::try_run_one_yielded_thread();
    };

    while(true){
        run_tasks(_pending_tasks);
         _signals.poll_signal();
        // do_check_lowres_timers();
        // do_expire_lowres_timers();
        // std::this_thread::sleep_for(100ms);
    }
    return 0;
}


boost::program_options::options_description
reactor::get_options_description() {
    namespace bpo = boost::program_options;
    bpo::options_description opts("Core options");
    // auto net_stack_names = network_stack_registry::list();
    opts.add_options()
        // ("network-stack", bpo::value<std::string>(),
        //         sprint("select network stack (valid values: %s)",
        //                 format_separated(net_stack_names.begin(), net_stack_names.end(), ", ")).c_str())
        ("no-handle-interrupt", "ignore SIGINT (for gdb)")
        ("poll-mode", "poll continuously (100% cpu use)")
        ("idle-poll-time-us", bpo::value<unsigned>()->default_value(200us / 1us),
                "idle polling time in microseconds (reduce for overprovisioned environments or laptops)")
        ("poll-aio", bpo::value<bool>()->default_value(true),
                "busy-poll for disk I/O (reduces latency and increases throughput)")
        ("task-quota-ms", bpo::value<double>()->default_value(2.0), "Max time (ms) between polls")
        ("max-task-backlog", bpo::value<unsigned>()->default_value(1000), "Maximum number of task backlog to allow; above this we ignore I/O")
        ("blocked-reactor-notify-ms", bpo::value<unsigned>()->default_value(2000), "threshold in miliseconds over which the reactor is considered blocked if no progress is made")
        ("relaxed-dma", "allow using buffered I/O if DMA is not available (reduces performance)")
        ("overprovisioned", "run in an overprovisioned environment (such as docker or a laptop); equivalent to --idle-poll-time-us 0 --thread-affinity 0 --poll-aio 0")
        ("abort-on-seastar-bad-alloc", "abort when seastar allocator cannot allocate memory");
    // opts.add(network_stack_registry::options_description());
    return opts;
}


void reactor::configure(boost::program_options::variables_map vm) {
    // auto network_stack_ready = vm.count("network-stack")
    //  ? network_stack_registry::create(sstring(vm["network-stack"].as<std::string>()), vm)
    //     : network_stack_registry::create(vm);
    // network_stack_ready.then([this] (std::unique_ptr<network_stack> stack) {
    //     _network_stack_ready_promise.set_value(std::move(stack));
    // });
    std::cout<<"reactor::configure"<<std::endl;
    _handle_sigint = !vm.count("no-handle-interrupt");
    std::cout<<"reactor::configure  _handle_sigint:"<<_handle_sigint<<std::endl;
    _task_quota = vm["task-quota-ms"].as<double>()*1ms;
    std::cout<<"reactor::configure  _task_quota:"<<std::endl;
    auto blocked_time = vm["blocked-reactor-notify-ms"].as<unsigned>()*1ms;
    std::cout<<"reactor::configure  blocked_time:"<<std::endl;
    _tasks_processed_report_threshold = unsigned(blocked_time / _task_quota);
    std::cout<<"reactor::configure  _tasks_processed_report_threshold:"<<std::endl;
    _max_task_backlog = vm["max-task-backlog"].as<unsigned>();
    std::cout<<"reactor::configure  _max_task_backlog:"<<std::endl;
    // _max_poll_time = vm["idle-poll-time-us"].as<unsigned>() * 1us;
    _max_poll_time = 200us;
    std::cout<<"reactor::configure  _max_poll_time:"<<std::endl;
    if (vm.count("poll-mode")) {
        std::cout<<"reactor::configure  poll_mode"<<std::endl;
        _max_poll_time = std::chrono::nanoseconds::max();
    }
    if (vm.count("overprovisioned")
           && vm["idle-poll-time-us"].defaulted()
           && !vm.count("poll-mode")) {
            std::cout<<"reactor::configure  overprovisioned"<<std::endl;
        _max_poll_time = 0us;
    }
    set_strict_dma(!vm.count("relaxed-dma"));
    std::cout<<"reactor::configure  _strict_dma"<<std::endl;

    // if (!vm["poll-aio"].as<bool>()
    //         || (vm["poll-aio"].defaulted() && vm.count("overprovisioned"))) {
    //     _aio_eventfd = pollable_fd(file_desc::eventfd(0, 0));
    // }
}
future<> reactor::run_exit_tasks() {
    _stop_requested.broadcast();
    _stopping = true;
    // // stop_aio_eventfd_loop();
    // return do_for_each(_exit_funcs.rbegin(), _exit_funcs.rend(), [] (auto& func) {
    //     return func();
    // });
}



reactor::reactor(unsigned id)
    : _id(id)
    , _cpu_started(0)
    , _io_context(0)
    , _io_context_available(max_aio){
    thread_impl::init();
    auto r = ::io_setup(max_aio, &_io_context);
    assert(r >= 0);
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, alarm_signal());
    r = ::pthread_sigmask(SIG_BLOCK, &mask, NULL);
    assert(r == 0);
    struct sigevent sev;
    sev.sigev_notify = SIGEV_THREAD_ID;
    sev._sigev_un._tid = syscall(SYS_gettid);
    sev.sigev_signo = alarm_signal();
    r = timer_create(CLOCK_MONOTONIC, &sev, &_steady_clock_timer);
    assert(r >= 0);
    sigemptyset(&mask);
    sigaddset(&mask, block_notifier_signal());
    r = ::pthread_sigmask(SIG_UNBLOCK, &mask, NULL);
    assert(r == 0);
    memory::set_reclaim_hook([this] (std::function<void ()> reclaim_fn) {
        add_high_priority_task(make_task([fn = std::move(reclaim_fn)] {
            fn();
        }));
    });
}
reactor::~reactor() {
    if (_steady_clock_timer) {
        timer_delete(_steady_clock_timer);
    }
}

void reactor::at_exit(std::function<future<> ()> func) {
    assert(!_stopping);
    _exit_funcs.push_back(std::move(func));
}


void reactor::run_tasks(std::deque<std::unique_ptr<task>>& tasks) {
    while (!tasks.empty()) {
        std::cout<<"run_task开始执行"<<std::endl;
        auto tsk = std::move(tasks.front());
        tasks.pop_front();
        tsk->run();
        std::cout<<"run_task结束执行"<<std::endl;
        tsk.reset();
    }
}

void reactor::exit(int ret) {
    smp::submit_to(0, [this, ret] { _return = ret; stop(); });
}

void reactor::stop() {
    assert(engine()._id == 0);
    smp::cleanup_cpu();
    if (!_stopping) {
        
    }
}



void smp::pin(unsigned cpu_id) {
    pin_this_thread(cpu_id);
}

void smp::arrive_at_event_loop_end() {
    if (_all_event_loops_done) {
        _all_event_loops_done->wait();
    }
}

void smp::allocate_reactor(unsigned id) {
    std::cout<<"开始执行smp::allocate_reactor"<<std::endl;
    assert(!reactor_holder);
    // we cannot just write "local_engin = new reactor" since reactor's constructor
    // uses local_engine
    void *buf;
    int r = posix_memalign(&buf, 64, sizeof(reactor));
    assert(r == 0);
    local_engine = reinterpret_cast<reactor*>(buf);
    new (buf) reactor(id); // 为什么要这样new?
    reactor_holder.reset(local_engine);
    std::cout<<"smp::allocate_reactor结束"<<std::endl;

}

void smp::cleanup() {
    smp::_threads = std::vector<posix_thread>();
    _thread_loops.clear();
}

void smp::cleanup_cpu() {
    size_t cpuid = engine().cpu_id();

    if (_qs) {
        for(unsigned i = 0; i < smp::count; i++) {
            _qs[i][cpuid].stop();
        }
    }
}

void smp::create_thread(std::function<void ()> thread_loop) {
    _threads.emplace_back(std::move(thread_loop));
}


static inline std::vector<char> string2vector(std::string str) {
    auto v = std::vector<char>(str.begin(), str.end());
    v.push_back('\0');
    return v;
}


#include <boost/lexical_cast.hpp>

size_t parse_memory_size(std::string s) {
    size_t factor = 1;
    if (s.size()) {
        auto c = s[s.size() - 1];
        static std::string suffixes = "kMGT";
        auto pos = suffixes.find(c);
        if (pos == suffixes.npos) {
            throw std::runtime_error("Cannot parse memory size");
        }
        factor <<= (pos + 1) * 10;
        s = s.substr(0, s.size() - 1);
    }
    return boost::lexical_cast<size_t>(s) * factor;
}


template <typename... A>
std::string format(const char* fmt, A&&... a) {
    return "";
}

void smp::configure(boost::program_options::variables_map configuration)
{
    // 初始化信号集，屏蔽所有信号
    sigset_t sigs;
    sigfillset(&sigs);
    for (auto sig : {SIGHUP, SIGQUIT, SIGILL, SIGABRT, SIGFPE, SIGSEGV,
            SIGALRM, SIGCONT, SIGSTOP, SIGTSTP, SIGTTIN, SIGTTOU}) {
        sigdelset(&sigs, sig);  // 从信号集中移除特定信号
    }
    pthread_sigmask(SIG_BLOCK, &sigs, nullptr);  // 设置线程信号掩码


    // 安装一次性信号处理器
    install_oneshot_signal_handler<SIGSEGV, sigsegv_action>();
    install_oneshot_signal_handler<SIGABRT, sigabrt_action>();
    std::cout<<"设置thread affinity"<<std::endl;
    // 获取配置中的线程亲和性设置
    auto thread_affinity = configuration["thread-affinity"].as<bool>(); //这里出错
    std::cout<<"thread affinity end"<<std::endl;
    if (configuration.count("overprovisioned")
           && configuration["thread-affinity"].defaulted()) {
        thread_affinity = false;  // 如果过载且未显式设置，则关闭线程亲和性
    }
    if (!thread_affinity && _using_dpdk) {
        printf("警告: 在 DPDK 模式下忽略 --thread-affinity 0\n");
    }

    // 初始化 SMP（对称多处理）相关参数
    smp::count = 1;  // 默认 CPU 数量为 1
    smp::_tmain = std::this_thread::get_id();  // 主线程 ID
    auto nr_cpus = resource::nr_processing_units();  // 获取可用的 CPU 数量
    resource::cpuset cpu_set;  // CPU 集合
    for (unsigned i = 0; i < nr_cpus; ++i) {
        cpu_set.insert(i);  // 将所有 CPU 添加到集合中
    }

    // 根据配置覆盖 CPU 集合和数量
    if (configuration.count("cpuset")) {
        cpu_set = configuration["cpuset"].as<cpuset_bpo_wrapper>().value;
    }
    if (configuration.count("smp")) {
        nr_cpus = configuration["smp"].as<unsigned>();
    } else {
        nr_cpus = cpu_set.size();
    }
    smp::count = nr_cpus;  // 更新 CPU 数量
    _reactors.resize(nr_cpus);  // 调整反应器数组大小
    // 配置资源分配
    resource::configuration rc;
    if (configuration.count("memory")) {
        //没有走到这行
        std::cout<<"配置中含有memory"<<std::endl;
        rc.total_memory = parse_memory_size(configuration["memory"].as<std::string>());
    }
    if (configuration.count("reserve-memory")) {
        std::cout<<"配置中含有reserve memory"<<std::endl;
        rc.reserve_memory = parse_memory_size(configuration["reserve-memory"].as<std::string>());
    }
    // 处理大页内存路径和内存锁定
    std::optional<std::string> hugepages_path;
    if (configuration.count("hugepages")) {
        std::cout<<"配置中含有hugepages"<<std::endl;
        hugepages_path = configuration["hugepages"].as<std::string>();
    }
    auto mlock = false;
    if (configuration.count("lock-memory")) {
        std::cout<<"配置中含有lock memory"<<std::endl;
        mlock = configuration["lock-memory"].as<bool>();
    }
    if (mlock) {
        std::cout<<"lock memory"<<std::endl;
        auto r = mlockall(MCL_CURRENT | MCL_FUTURE);  // 锁定内存
        if (r) {
            printf("警告: mlockall 失败\n");
        }
    }
    // 配置资源分配参数
    rc.cpus = smp::count;//12
    rc.cpu_set = std::move(cpu_set);
    if (configuration.count("max-io-requests")) {
        std::cout<<"配置中含有max io requests"<<std::endl;
        rc.max_io_requests = configuration["max-io-requests"].as<unsigned>();
    }
    if (configuration.count("num-io-queues")) {
        std::cout<<"配置中含有num io queues"<<std::endl;
        rc.io_queues = configuration["num-io-queues"].as<unsigned>();
    }
    // 分配资源并初始化 CPU 和内存
    auto resources = resource::allocate(rc); 
    std::vector<resource::cpu> allocations = std::move(resources.cpus);//allocations是CPU的vector.
/*
allocations format:
[cpu][cpu]...[cpu]
or
[cpuid(0~11),bytes,nodeid(0)]
*/

    if (thread_affinity) {
        std::cout<<"thread pind to 0"<<std::endl;
        smp::pin(allocations[0].cpu_id);  // 绑定主线程到CPU 0
    }
    std::cout<<"mem config begin "<<std::endl;
    // std::cout<<"hugepages_path is "<<hugepages_path<<std::endl;
    //这里报错
    memory::configure(allocations[0].mem, hugepages_path);//hugepages_path是空的.

    std::cout<<"mem config end "<<std::endl;
    // 启用或禁用内存分配失败时的终止行为
    if (configuration.count("abort-on-seastar-bad-alloc")) {
        memory::enable_abort_on_allocation_failure();
    }
    // 启用堆内存分析
    bool heapprof_enabled = configuration.count("heapprof");
    memory::set_heap_profiling_enabled(heapprof_enabled);
    // 创建同步屏障
    static boost::barrier reactors_registered(smp::count);
    static boost::barrier smp_queues_constructed(smp::count);
    static boost::barrier inited(smp::count);

    // 初始化 IO 队列信息
    auto io_info = std::move(resources.io_queues);
    std::vector<io_queue*> all_io_queues;
    all_io_queues.resize(io_info.coordinators.size());
    io_queue::fill_shares_array();

    // 分配 IO 队列
    auto alloc_io_queue = [io_info, &all_io_queues] (unsigned shard) {
        auto cid = io_info.shard_to_coordinator[shard];
        int vec_idx = 0;
        for (auto& coordinator: io_info.coordinators) {
            if (coordinator.id != cid) {
                vec_idx++;
                continue;
            }
            if (shard == cid) {
                all_io_queues[vec_idx] = new io_queue(coordinator.id, coordinator.capacity, io_info.shard_to_coordinator);
            }
            return vec_idx;
        }
        assert(0); // 不可能到达这里
    };
    // 分配 IO 队列给线程
    auto assign_io_queue = [&all_io_queues] (shard_id id, int queue_idx) {
        if (all_io_queues[queue_idx]->coordinator() == id) {
            engine().my_io_queue.reset(all_io_queues[queue_idx]);
        }
        engine()._io_queue = all_io_queues[queue_idx];
        engine()._io_coordinator = all_io_queues[queue_idx]->coordinator();
    };

    _all_event_loops_done.emplace(smp::count);

    // 创建额外的线程来运行反应器
    unsigned i;
    for (i = 1; i < smp::count; i++) {
        auto allocation = allocations[i];
        create_thread([configuration, hugepages_path, i, allocation, assign_io_queue, alloc_io_queue, thread_affinity, heapprof_enabled] {
            std::cout<<"create thread "<<i<<std::endl;
            auto thread_name = format("reactor-{}", i);
            pthread_setname_np(pthread_self(), thread_name.c_str());  // 设置线程名称
            if (thread_affinity) {
                smp::pin(allocation.cpu_id);  // 绑定线程到指定 CPU
            }
            memory::configure(allocation.mem, hugepages_path);
            memory::set_heap_profiling_enabled(heapprof_enabled);
            sigset_t mask;
            sigfillset(&mask);
            for (auto sig : { SIGSEGV }) {
                sigdelset(&mask, sig);  // 移除特定信号
            }
            auto r = ::pthread_sigmask(SIG_BLOCK, &mask, NULL);
            throw_pthread_error(r);
            allocate_reactor(i);
            _reactors[i] = &engine();
            auto queue_idx = alloc_io_queue(i);
            reactors_registered.wait();
            smp_queues_constructed.wait();
            start_all_queues();
            assign_io_queue(i, queue_idx);
            inited.wait();
            engine().configure(configuration);
            engine().run();
        });
    }
    // 主线程分配反应器
    allocate_reactor(0);
    _reactors[0] = &engine();
    auto queue_idx = alloc_io_queue(0);
    // 等待所有反应器注册完成
    reactors_registered.wait();
    std::cout<<"reactors registered done"<<std::endl;
    /*----------------------------上面代码成功执行-------------------------------------------------- */
    smp::_qs = new smp_message_queue* [smp::count];

    std::cout<<"smp qs begin"<<std::endl;
    for(unsigned i = 0; i < smp::count; i++) {
        smp::_qs[i] = reinterpret_cast<smp_message_queue*>(operator new[] (sizeof(smp_message_queue) * smp::count));
        for (unsigned j = 0; j < smp::count; ++j) {
            new (&smp::_qs[i][j]) smp_message_queue(_reactors[j], _reactors[i]);
        }
    }
    std::cout<<"smp qs end"<<std::endl;
    smp_queues_constructed.wait();
    std::cout<<"start all queues"<<std::endl;
    start_all_queues();
    std::cout<<"start all queues done"<<std::endl;
    assign_io_queue(0, queue_idx);
    std::cout<<"assign io queues done"<<std::endl;
    inited.wait();
    std::cout<<"inited done"<<std::endl;
    // 配置引擎并启动低分辨率时钟
    engine().configure(configuration);//这里出错(为什么之前的没有报错?)
    std::cout<<"engine configure done"<<std::endl;
    engine()._lowres_clock = std::make_unique<lowres_clock>();
}

bool smp::poll_queues() {
    size_t got = 0;
    for (unsigned i = 0; i < count; i++) {
        if (engine().cpu_id() != i) {
            auto& rxq = _qs[engine().cpu_id()][i];
            rxq.flush_response_batch();
            got += rxq.has_unflushed_responses();
            got += rxq.process_incoming();
            auto& txq = _qs[i][engine()._id];
            txq.flush_request_batch();
            got += txq.process_completions();
        }
    }
    return got != 0;
}

bool smp::pure_poll_queues() {
    for (unsigned i = 0; i < count; i++) {
        if (engine().cpu_id() != i) {
            auto& rxq = _qs[engine().cpu_id()][i];
            rxq.flush_response_batch();
            auto& txq = _qs[i][engine()._id];
            txq.flush_request_batch();
            if (rxq.pure_poll_rx() || txq.pure_poll_tx() || rxq.has_unflushed_responses()) {
                return true;
            }
        }
    }
    return false;
}

boost::program_options::options_description
smp::get_options_description()
{
    namespace bpo = boost::program_options;
    bpo::options_description opts("SMP options");
    opts.add_options()
        ("smp,c", bpo::value<unsigned>(), "number of threads (default: one per CPU)")
        ("cpuset", bpo::value<cpuset_bpo_wrapper>(), "CPUs to use (in cpuset(7) format; default: all))")
        ("memory,m", bpo::value<std::string>(), "memory to use, in bytes (ex: 4G) (default: all)")
        ("reserve-memory", bpo::value<std::string>(), "memory reserved to OS (if --memory not specified)")
        ("hugepages", bpo::value<std::string>(), "path to accessible hugetlbfs mount (typically /dev/hugepages/something)")
        ("lock-memory", bpo::value<bool>(), "lock all memory (prevents swapping)")
        ("thread-affinity", bpo::value<bool>()->default_value(true), "pin threads to their cpus (disable for overprovisioning)")
        ("num-io-queues", bpo::value<unsigned>(), "Number of IO queues. Each IO unit will be responsible for a fraction of the IO requests. Defaults to the number of threads")
        ("max-io-requests", bpo::value<unsigned>(), "Maximum amount of concurrent requests to be sent to the disk. Defaults to 128 times the number of IO queues")
        ;
    return opts;
}


unsigned smp::count = 1;
bool smp::_using_dpdk;

void smp::start_all_queues()
{
    for (unsigned c = 0; c < count; c++) {
        if (c != engine().cpu_id()) {
            _qs[c][engine().cpu_id()].start(c);
        }
    }
}


void smp::join_all()
{

    for (auto&& t: smp::_threads) {
        t.join();
    }
}


void* posix_thread::start_routine(void* arg) noexcept {
    auto pfunc = reinterpret_cast<std::function<void ()>*>(arg);
    (*pfunc)();
    return nullptr;
}

posix_thread::posix_thread(std::function<void ()> func)
    : posix_thread(attr{}, std::move(func)) {
}

posix_thread::posix_thread(attr a, std::function<void ()> func)
    : _func(std::make_unique<std::function<void ()>>(std::move(func))) {
    pthread_attr_t pa;
    auto r = pthread_attr_init(&pa);
    if (r) {
        throw std::system_error(r, std::system_category());
    }
    auto stack_size = a._stack_size.size;
    if (!stack_size) {
        stack_size = 2 << 20;
    }
    // allocate guard area as well
    _stack = mmap_anonymous(nullptr, stack_size + (4 << 20),
        PROT_NONE, MAP_PRIVATE | MAP_NORESERVE);
    auto stack_start = align_up(_stack.get() + 1, 2 << 20);
    mmap_area real_stack = mmap_anonymous(stack_start, stack_size,
        PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_FIXED | MAP_STACK);
    real_stack.release(); // protected by @_stack
    ::madvise(stack_start, stack_size, MADV_HUGEPAGE);
    r = pthread_attr_setstack(&pa, stack_start, stack_size);
    if (r) {
        throw std::system_error(r, std::system_category());
    }
    r = pthread_create(&_pthread, &pa,
                &posix_thread::start_routine, _func.get());
    if (r) {
        throw std::system_error(r, std::system_category());
    }
}

posix_thread::posix_thread(posix_thread&& x)
    : _func(std::move(x._func)), _pthread(x._pthread), _valid(x._valid)
    , _stack(std::move(x._stack)) {
    x._valid = false;
}

posix_thread::~posix_thread() {
    assert(!_valid);
}

void posix_thread::join() {
    assert(_valid);
    pthread_join(_pthread, NULL);
    _valid = false;
}



#include <iostream>


namespace memory {

static thread_local int abort_on_alloc_failure_suppressed = 0;

disable_abort_on_alloc_failure_temporarily::disable_abort_on_alloc_failure_temporarily() {
    ++abort_on_alloc_failure_suppressed;
}

disable_abort_on_alloc_failure_temporarily::~disable_abort_on_alloc_failure_temporarily() noexcept {
    --abort_on_alloc_failure_suppressed;
}

}

#ifndef DEFAULT_ALLOCATOR

#include "../util/bitops.hh"
#include "../util/align.hh"
#include "../fd/posix.hh"
#include "../util/shared_ptr.hh"
#include <new>
#include <cstdint>
#include <algorithm>
#include <limits>
#include <cassert>
#include <atomic>
#include <mutex>
#include <experimental/optional>
#include <functional>
#include <cstring>
#include <sys/uio.h>  // For writev
#include <boost/intrusive/list.hpp>
#include <sys/mman.h>
#include "../util/defer.hh"
#include "../util/backtrace.hh"
#include <unordered_set>
#ifdef HAVE_NUMA
#include <numaif.h>
#endif

struct allocation_site {
    mutable size_t count = 0; // number of live objects allocated at backtrace.
    mutable size_t size = 0; // amount of bytes in live objects allocated at backtrace.
    mutable const allocation_site* next = nullptr;
    saved_backtrace backtrace;

    bool operator==(const allocation_site& o) const {
        return backtrace == o.backtrace;
    }

    bool operator!=(const allocation_site& o) const {
        return !(*this == o);
    }
};

namespace std {

template<>
struct hash<::allocation_site> {
    size_t operator()(const ::allocation_site& bi) const {
        return std::hash<saved_backtrace>()(bi.backtrace);
    }
};

}

using allocation_site_ptr = const allocation_site*;

namespace memory {

static allocation_site_ptr get_allocation_site() __attribute__((unused));

static std::atomic<bool> abort_on_allocation_failure{false};

void enable_abort_on_allocation_failure() {
    abort_on_allocation_failure.store(true, std::memory_order_seq_cst);
}

static void on_allocation_failure(size_t size) {
    if (!abort_on_alloc_failure_suppressed
            && abort_on_allocation_failure.load(std::memory_order_relaxed)) {
        abort();
    }
}

static constexpr unsigned cpu_id_shift = 36; // FIXME: make dynamic
static constexpr unsigned max_cpus = 256;
static constexpr size_t cache_line_size = 64;
using pageidx = uint32_t;
struct page;
class page_list;
static std::atomic<bool> live_cpus[max_cpus];
static thread_local uint64_t g_allocs;
static thread_local uint64_t g_frees;
static thread_local uint64_t g_cross_cpu_frees;
static thread_local uint64_t g_reclaims;
#include <functional>  // for std::function
#include <optional>    // for std::optional
#include <memory>      // for std::unique_ptr
using allocate_system_memory_fn = std::function<mmap_area(std::optional<void*> where, size_t how_much)>;

namespace bi = boost::intrusive;

inline
unsigned object_cpu_id(const void* ptr) {
    return (reinterpret_cast<uintptr_t>(ptr) >> cpu_id_shift) & 0xff;
}

class page_list_link {
    uint32_t _prev;
    uint32_t _next;
    friend class page_list;
};

static char* mem_base() {
    static char* known;
    static std::once_flag flag;
    std::call_once(flag, [] {
        size_t alloc = size_t(1) << 44;
        auto r = ::mmap(NULL, 2 * alloc,
                    PROT_NONE,
                    MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE,
                    -1, 0);
        if (r == MAP_FAILED) {
            abort();
        }
        ::madvise(r, 2 * alloc, MADV_DONTDUMP);
        auto cr = reinterpret_cast<char*>(r);
        known = align_up(cr, alloc);
        ::munmap(cr, known - cr);
        ::munmap(known + alloc, cr + 2 * alloc - (known + alloc));
    });
    return known;
}

constexpr bool is_page_aligned(size_t size) {
    return (size & (page_size - 1)) == 0;
}

constexpr size_t next_page_aligned(size_t size) {
    return (size + (page_size - 1)) & ~(page_size - 1);
}

class small_pool;

struct free_object {
    free_object* next;
};

struct page {
    bool free;
    uint8_t offset_in_span;
    uint16_t nr_small_alloc;
    uint32_t span_size; // in pages, if we're the head or the tail
    page_list_link link;
    small_pool* pool;  // if used in a small_pool
    free_object* freelist;
};

class page_list {
    uint32_t _front = 0;
    uint32_t _back = 0;
public:
    page& front(page* ary) { return ary[_front]; }
    page& back(page* ary) { return ary[_back]; }
    bool empty() const { return !_front; }
    void erase(page* ary, page& span) {
        if (span.link._next) {
            ary[span.link._next].link._prev = span.link._prev;
        } else {
            _back = span.link._prev;
        }
        if (span.link._prev) {
            ary[span.link._prev].link._next = span.link._next;
        } else {
            _front = span.link._next;
        }
    }
    void push_front(page* ary, page& span) {
        auto idx = &span - ary;
        if (_front) {
            ary[_front].link._prev = idx;
        } else {
            _back = idx;
        }
        span.link._next = _front;
        span.link._prev = 0;
        _front = idx;
    }
    void pop_front(page* ary) {
        if (ary[_front].link._next) {
            ary[ary[_front].link._next].link._prev = 0;
        } else {
            _back = 0;
        }
        _front = ary[_front].link._next;
    }
    page* find(uint32_t n_pages, page* ary) {
        auto n = _front;
        while (n && ary[n].span_size < n_pages) {
            n = ary[n].link._next;
        }
        if (!n) {
            return nullptr;
        }
        return &ary[n];
    }
};

class small_pool {
    unsigned _object_size;
    unsigned _span_size;
    free_object* _free = nullptr;
    size_t _free_count = 0;
    unsigned _min_free;
    unsigned _max_free;
    unsigned _spans_in_use = 0;
    page_list _span_list;
    static constexpr unsigned idx_frac_bits = 2;
private:
    size_t span_bytes() const { return _span_size * page_size; }
public:
    explicit small_pool(unsigned object_size) noexcept;
    ~small_pool();
    void* allocate();
    void deallocate(void* object);
    unsigned object_size() const { return _object_size; }
    bool objects_page_aligned() const { return is_page_aligned(_object_size); }
    static constexpr unsigned size_to_idx(unsigned size);
    static constexpr unsigned idx_to_size(unsigned idx);
    allocation_site_ptr& alloc_site_holder(void* ptr);
private:
    void add_more_objects();
    void trim_free_list();
    float waste();
};

// index 0b0001'1100 -> size (1 << 4) + 0b11 << (4 - 2)

constexpr unsigned
small_pool::idx_to_size(unsigned idx) {
    return (((1 << idx_frac_bits) | (idx & ((1 << idx_frac_bits) - 1)))
              << (idx >> idx_frac_bits))
                  >> idx_frac_bits;
}

constexpr unsigned
small_pool::size_to_idx(unsigned size) {
    return ((log2floor(size) << idx_frac_bits) - ((1 << idx_frac_bits) - 1))
            + ((size - 1) >> (log2floor(size) - idx_frac_bits));
}

class small_pool_array {
public:
    static constexpr unsigned nr_small_pools = small_pool::size_to_idx(4 * page_size) + 1;
private:
    union u {
        small_pool a[nr_small_pools];
        u() {
            for (unsigned i = 0; i < nr_small_pools; ++i) {
                new (&a[i]) small_pool(small_pool::idx_to_size(i));
            }
        }
        ~u() {
            // cannot really call destructor, since other
            // objects may be freed after we are gone.
        }
    } _u;
public:
    small_pool& operator[](unsigned idx) { return _u.a[idx]; }
};

static constexpr size_t max_small_allocation
    = small_pool::idx_to_size(small_pool_array::nr_small_pools - 1);

constexpr size_t object_size_with_alloc_site(size_t size) {

    return size;
}

struct cross_cpu_free_item {
    cross_cpu_free_item* next;
};

struct cpu_pages {
    uint32_t min_free_pages = 20000000 / page_size;
    char* memory;
    page* pages;
    uint32_t nr_pages;
    uint32_t nr_free_pages;
    uint32_t current_min_free_pages = 0;
    unsigned cpu_id = -1U;
    std::function<void (std::function<void ()>)> reclaim_hook;
    std::vector<reclaimer*> reclaimers;
    static constexpr unsigned nr_span_lists = 32;
    union pla {
        pla() {
            for (auto&& e : free_spans) {
                std::cout<<"开始初始化free_spans"<<std::endl;
                new (&e) page_list;
            }
        }
        ~pla() {
            // no destructor -- might be freeing after we die
        }
        page_list free_spans[nr_span_lists];  // contains spans with span_size >= 2^idx
    } fsu;
    small_pool_array small_pools;
    alignas(cache_line_size) std::atomic<cross_cpu_free_item*> xcpu_freelist;
    alignas(cache_line_size) std::vector<physical_address> virt_to_phys_map;
    static std::atomic<unsigned> cpu_id_gen;
    static cpu_pages* all_cpus[max_cpus];
    union asu {
        using alloc_sites_type = std::unordered_set<allocation_site>;
        asu() {
            new (&alloc_sites) alloc_sites_type();
        }
        ~asu() {} // alloc_sites live forever
        alloc_sites_type alloc_sites;
    } asu;
    allocation_site_ptr alloc_site_list_head = nullptr; // For easy traversal of asu.alloc_sites from scylla-gdb.py
    bool collect_backtrace = false;
    char* mem() { return memory; }

    void link(page_list& list, page* span);
    void unlink(page_list& list, page* span);
    struct trim {
        unsigned offset;
        unsigned nr_pages; // 这个是什么意思?
    };
    void maybe_reclaim();
    template <typename Trimmer>
    void* allocate_large_and_trim(unsigned nr_pages, Trimmer trimmer);
    void* allocate_large(unsigned nr_pages);
    void* allocate_large_aligned(unsigned align_pages, unsigned nr_pages);
    page* find_and_unlink_span(unsigned nr_pages);
    page* find_and_unlink_span_reclaiming(unsigned n_pages);
    void free_large(void* ptr);
    void free_span(pageidx start, uint32_t nr_pages);
    void free_span_no_merge(pageidx start, uint32_t nr_pages);
    void* allocate_small(unsigned size);
    void free(void* ptr);
    void free(void* ptr, size_t size);
    bool try_cross_cpu_free(void* ptr);
    void shrink(void* ptr, size_t new_size);
    void free_cross_cpu(unsigned cpu_id, void* ptr);
    bool drain_cross_cpu_freelist();
    size_t object_size(void* ptr);
    page* to_page(void* p) {
        return &pages[(reinterpret_cast<char*>(p) - mem()) / page_size];
    }

    bool is_initialized() const;
    bool initialize();
    reclaiming_result run_reclaimers(reclaimer_scope);
    void schedule_reclaim();
    void set_reclaim_hook(std::function<void (std::function<void ()>)> hook);
    void set_min_free_pages(size_t pages);
    void resize(size_t new_size, allocate_system_memory_fn alloc_sys_mem);
    void do_resize(size_t new_size, allocate_system_memory_fn alloc_sys_mem);
    void replace_memory_backing(allocate_system_memory_fn alloc_sys_mem);
    void init_virt_to_phys_map();
    memory::memory_layout memory_layout();
    translation translate(const void* addr, size_t size);
    ~cpu_pages();
};

static thread_local cpu_pages cpu_mem;
std::atomic<unsigned> cpu_pages::cpu_id_gen;
cpu_pages* cpu_pages::all_cpus[max_cpus];

void set_heap_profiling_enabled(bool enable) {
    bool is_enabled = cpu_mem.collect_backtrace;
    if (enable) {
        if (!is_enabled) {
            abort();
        }
    } else {
        if (is_enabled) {
            abort();
        }
    }
    cpu_mem.collect_backtrace = enable;
}

// Free spans are store in the largest index i such that nr_pages >= 1 << i.
static inline
unsigned index_of(unsigned pages) {
    return std::numeric_limits<unsigned>::digits - count_leading_zeros(pages) - 1;
}

// Smallest index i such that all spans stored in the index are >= pages.
static inline
unsigned index_of_conservative(unsigned pages) {
    if (pages == 1) {
        return 0;
    }
    return std::numeric_limits<unsigned>::digits - count_leading_zeros(pages - 1);
}

void
cpu_pages::unlink(page_list& list, page* span) {
    list.erase(pages, *span);
}

void
cpu_pages::link(page_list& list, page* span) {
    list.push_front(pages, *span);
}

void cpu_pages::free_span_no_merge(uint32_t span_start, uint32_t nr_pages) {
    assert(nr_pages);
    nr_free_pages += nr_pages;
    auto span = &pages[span_start];
    auto span_end = &pages[span_start + nr_pages - 1];
    span->free = span_end->free = true;
    span->span_size = span_end->span_size = nr_pages;
    auto idx = index_of(nr_pages);
    link(fsu.free_spans[idx], span);
}

void cpu_pages::free_span(uint32_t span_start, uint32_t nr_pages) {
    page* before = &pages[span_start - 1];
    if (before->free) {
        auto b_size = before->span_size;
        assert(b_size);
        span_start -= b_size;
        nr_pages += b_size;
        nr_free_pages -= b_size;
        unlink(fsu.free_spans[index_of(b_size)], before - (b_size - 1));
    }
    page* after = &pages[span_start + nr_pages];
    if (after->free) {
        auto a_size = after->span_size;
        assert(a_size);
        nr_pages += a_size;
        nr_free_pages -= a_size;
        unlink(fsu.free_spans[index_of(a_size)], after);
    }
    free_span_no_merge(span_start, nr_pages);
}

page*
cpu_pages::find_and_unlink_span(unsigned n_pages) {
    auto idx = index_of_conservative(n_pages);
    auto orig_idx = idx;
    if (n_pages >= (2u << idx)) {
        throw std::bad_alloc();
    }
    while (idx < nr_span_lists && fsu.free_spans[idx].empty()) {
        ++idx;
    }
    if (idx == nr_span_lists) {
        if (initialize()) {
            return find_and_unlink_span(n_pages);
        }
        // Can smaller list possibly hold object?
        idx = index_of(n_pages);
        if (idx == orig_idx) {   // was exact power of two
            return nullptr;
        }
    }
    auto& list = fsu.free_spans[idx];
    page* span = list.find(n_pages, pages);
    if (!span) {
        return nullptr;
    }
    unlink(list, span);
    return span;
}

page*
cpu_pages::find_and_unlink_span_reclaiming(unsigned n_pages) {
    while (true) {
        auto span = find_and_unlink_span(n_pages);
        if (span) {
            return span;
        }
        if (run_reclaimers(reclaimer_scope::sync) == reclaiming_result::reclaimed_nothing) {
            return nullptr;
        }
    }
}

void cpu_pages::maybe_reclaim() {
    if (nr_free_pages < current_min_free_pages) {
        drain_cross_cpu_freelist();
        run_reclaimers(reclaimer_scope::sync);
        if (nr_free_pages < current_min_free_pages) {
            schedule_reclaim();
        }
    }
}

template <typename Trimmer>
void*
cpu_pages::allocate_large_and_trim(unsigned n_pages, Trimmer trimmer) {
    // Avoid exercising the reclaimers for requests we'll not be able to satisfy
    // nr_pages might be zero during startup, so check for that too
    if (nr_pages && n_pages >= nr_pages) {
        return nullptr;
    }
    page* span = find_and_unlink_span_reclaiming(n_pages);
    if (!span) {
        return nullptr;
    }
    auto span_size = span->span_size;
    auto span_idx = span - pages;
    nr_free_pages -= span->span_size;
    trim t = trimmer(span_idx, nr_pages);
    if (t.offset) {
        free_span_no_merge(span_idx, t.offset);
        span_idx += t.offset;
        span_size -= t.offset;
        span = &pages[span_idx];

    }
    if (t.nr_pages < span_size) {
        free_span_no_merge(span_idx + t.nr_pages, span_size - t.nr_pages);
    }
    auto span_end = &pages[span_idx + t.nr_pages - 1];
    span->free = span_end->free = false;
    span->span_size = span_end->span_size = t.nr_pages;
    span->pool = nullptr;
    maybe_reclaim();
    return mem() + span_idx * page_size;
}

void*
cpu_pages::allocate_large(unsigned n_pages) {
    return allocate_large_and_trim(n_pages, [n_pages] (unsigned idx, unsigned n) {
        return trim{0, std::min(n, n_pages)};
    });
}

void*
cpu_pages::allocate_large_aligned(unsigned align_pages, unsigned n_pages) {
    return allocate_large_and_trim(n_pages + align_pages - 1, [=] (unsigned idx, unsigned n) {
        return trim{align_up(idx, align_pages) - idx, n_pages};
    });
}

#ifdef SEASTAR_HEAPPROF

class disable_backtrace_temporarily {
    bool _old;
public:
    disable_backtrace_temporarily() {
        _old = cpu_mem.collect_backtrace;
        cpu_mem.collect_backtrace = false;
    }
    ~disable_backtrace_temporarily() {
        cpu_mem.collect_backtrace = _old;
    }
};

#else

struct disable_backtrace_temporarily {
    ~disable_backtrace_temporarily() {}
};

#endif

static
saved_backtrace get_backtrace() noexcept {
    disable_backtrace_temporarily dbt;
    return current_backtrace();
}

static
allocation_site_ptr get_allocation_site() {
    if (!cpu_mem.is_initialized() || !cpu_mem.collect_backtrace) {
        return nullptr;
    }
    disable_backtrace_temporarily dbt;
    allocation_site new_alloc_site;
    new_alloc_site.backtrace = get_backtrace();
    auto insert_result = cpu_mem.asu.alloc_sites.insert(std::move(new_alloc_site));
    allocation_site_ptr alloc_site = &*insert_result.first;
    if (insert_result.second) {
        alloc_site->next = cpu_mem.alloc_site_list_head;
        cpu_mem.alloc_site_list_head = alloc_site;
    }
    return alloc_site;
}

#ifdef SEASTAR_HEAPPROF

allocation_site_ptr&
small_pool::alloc_site_holder(void* ptr) {
    if (objects_page_aligned()) {
        return cpu_mem.to_page(ptr)->alloc_site;
    } else {
        return *reinterpret_cast<allocation_site_ptr*>(reinterpret_cast<char*>(ptr) + _object_size - sizeof(allocation_site_ptr));
    }
}

#endif

void*
cpu_pages::allocate_small(unsigned size) {
    auto idx = small_pool::size_to_idx(size);
    auto& pool = small_pools[idx];
    assert(size <= pool.object_size());
    auto ptr = pool.allocate();
#ifdef SEASTAR_HEAPPROF
    if (!ptr) {
        return nullptr;
    }
    allocation_site_ptr alloc_site = get_allocation_site();
    if (alloc_site) {
        ++alloc_site->count;
        alloc_site->size += pool.object_size();
    }
    new (&pool.alloc_site_holder(ptr)) allocation_site_ptr{alloc_site};
#endif
    return ptr;
}

void cpu_pages::free_large(void* ptr) {
    pageidx idx = (reinterpret_cast<char*>(ptr) - mem()) / page_size;
    page* span = &pages[idx];
#ifdef SEASTAR_HEAPPROF
    auto alloc_site = span->alloc_site;
    if (alloc_site) {
        --alloc_site->count;
        alloc_site->size -= span->span_size * page_size;
    }
#endif
    free_span(idx, span->span_size);
}

size_t cpu_pages::object_size(void* ptr) {
    pageidx idx = (reinterpret_cast<char*>(ptr) - mem()) / page_size;
    page* span = &pages[idx];
    if (span->pool) {
        auto s = span->pool->object_size();
#ifdef SEASTAR_HEAPPROF
        // We must not allow the object to be extended onto the allocation_site_ptr field.
        if (!span->pool->objects_page_aligned()) {
            s -= sizeof(allocation_site_ptr);
        }
#endif
        return s;
    } else {
        return size_t(span->span_size) * page_size;
    }
}

void cpu_pages::free_cross_cpu(unsigned cpu_id, void* ptr) {
    if (!live_cpus[cpu_id].load(std::memory_order_relaxed)) {
        // Thread was destroyed; leak object
        // should only happen for boost unit-tests.
        return;
    }
    auto p = reinterpret_cast<cross_cpu_free_item*>(ptr);
    auto& list = all_cpus[cpu_id]->xcpu_freelist;
    auto old = list.load(std::memory_order_relaxed);
    do {
        p->next = old;
    } while (!list.compare_exchange_weak(old, p, std::memory_order_release, std::memory_order_relaxed));
    ++g_cross_cpu_frees;
}

bool cpu_pages::drain_cross_cpu_freelist() {
    if (!xcpu_freelist.load(std::memory_order_relaxed)) {
        return false;
    }
    auto p = xcpu_freelist.exchange(nullptr, std::memory_order_acquire);
    while (p) {
        auto n = p->next;
        ++g_frees;
        free(p);
        p = n;
    }
    return true;
}

void cpu_pages::free(void* ptr) {
    page* span = to_page(ptr);
    if (span->pool) {
        small_pool& pool = *span->pool;
#ifdef SEASTAR_HEAPPROF
        allocation_site_ptr alloc_site = pool.alloc_site_holder(ptr);
        if (alloc_site) {
            --alloc_site->count;
            alloc_site->size -= pool.object_size();
        }
#endif
        pool.deallocate(ptr);
    } else {
        free_large(ptr);
    }
}

void cpu_pages::free(void* ptr, size_t size) {
    // match action on allocate() so hit the right pool
    if (size <= sizeof(free_object)) {
        size = sizeof(free_object);
    }
    if (size <= max_small_allocation) {
        size = object_size_with_alloc_site(size);
        auto pool = &small_pools[small_pool::size_to_idx(size)];
#ifdef SEASTAR_HEAPPROF
        allocation_site_ptr alloc_site = pool->alloc_site_holder(ptr);
        if (alloc_site) {
            --alloc_site->count;
            alloc_site->size -= pool->object_size();
        }
#endif
        pool->deallocate(ptr);
    } else {
        free_large(ptr);
    }
}

bool
cpu_pages::try_cross_cpu_free(void* ptr) {
    auto obj_cpu = object_cpu_id(ptr);
    if (obj_cpu != cpu_id) {
        free_cross_cpu(obj_cpu, ptr);
        return true;
    }
    return false;
}

void cpu_pages::shrink(void* ptr, size_t new_size) {
    auto obj_cpu = object_cpu_id(ptr);
    assert(obj_cpu == cpu_id);
    page* span = to_page(ptr);
    if (span->pool) {
        return;
    }
    size_t new_size_pages = align_up(new_size, page_size) / page_size;
    auto old_size_pages = span->span_size;
    assert(old_size_pages >= new_size_pages);
    if (new_size_pages == old_size_pages) {
        return;
    }
#ifdef SEASTAR_HEAPPROF
    auto alloc_site = span->alloc_site;
    if (alloc_site) {
        alloc_site->size -= span->span_size * page_size;
        alloc_site->size += new_size_pages * page_size;
    }
#endif
    span->span_size = new_size_pages;
    span[new_size_pages - 1].free = false;
    span[new_size_pages - 1].span_size = new_size_pages;
    pageidx idx = span - pages;
    free_span(idx + new_size_pages, old_size_pages - new_size_pages);
}

cpu_pages::~cpu_pages() {
    live_cpus[cpu_id].store(false, std::memory_order_relaxed);
}

bool cpu_pages::is_initialized() const {
    return bool(nr_pages);
}

bool cpu_pages::initialize() {
    std::cout<<"调用初始化cpu_pages"<<std::endl;
    if (is_initialized()) {
        return false;
    }
    cpu_id = cpu_id_gen.fetch_add(1, std::memory_order_relaxed);
    assert(cpu_id < max_cpus);
    all_cpus[cpu_id] = this;
    auto base = mem_base() + (size_t(cpu_id) << cpu_id_shift);
    auto size = 32 << 20;  // Small size for bootstrap
    auto r = ::mmap(base, size,
            PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED,
            -1, 0);
    if (r == MAP_FAILED) {
        abort();
    }
    ::madvise(base, size, MADV_HUGEPAGE);
    pages = reinterpret_cast<page*>(base);
    memory = base;
    nr_pages = size / page_size;
    // we reserve the end page so we don't have to special case
    // the last span.
    auto reserved = align_up(sizeof(page) * (nr_pages + 1), page_size) / page_size;
    for (pageidx i = 0; i < reserved; ++i) {
        pages[i].free = false;
    }
    pages[nr_pages].free = false;
    free_span_no_merge(reserved, nr_pages - reserved);
    live_cpus[cpu_id].store(true, std::memory_order_relaxed);
    return true;
}


mmap_area
allocate_anonymous_memory(std::optional<void*> where, size_t how_much) {
    return mmap_anonymous(where.value_or(nullptr),
            how_much, PROT_READ|PROT_WRITE, MAP_PRIVATE|(where?MAP_FIXED :0));
}








/*
    映射匿名area.
*/

mmap_area
allocate_hugetlbfs_memory(file_desc& fd, std::optional<void*> where, size_t how_much) {
    auto pos = fd.size();
    fd.truncate(pos + how_much);
    auto ret = fd.map(
            how_much,
            PROT_READ | PROT_WRITE,
            MAP_SHARED | MAP_POPULATE | (where ? MAP_FIXED : 0),
            pos,
            where.value_or(nullptr));
    return ret;
}

void cpu_pages::replace_memory_backing(allocate_system_memory_fn alloc_sys_mem) {
    // We would like to use ::mremap() to atomically replace the old anonymous
    // memory with hugetlbfs backed memory, but mremap() does not support hugetlbfs
    // (for no reason at all).  So we must copy the anonymous memory to some other
    // place, map hugetlbfs in place, and copy it back, without modifying it during
    // the operation.
    auto bytes = nr_pages * page_size;
    auto old_mem = mem();
    auto relocated_old_mem = mmap_anonymous(nullptr, bytes, PROT_READ|PROT_WRITE, MAP_PRIVATE);
    std::memcpy(relocated_old_mem.get(), old_mem, bytes);
    alloc_sys_mem({old_mem}, bytes).release();
    std::memcpy(old_mem, relocated_old_mem.get(), bytes);
}

void cpu_pages::init_virt_to_phys_map() {
    auto nr_entries = nr_pages / (huge_page_size / page_size);
    virt_to_phys_map.resize(nr_entries);
    auto fd = file_desc::open("/proc/self/pagemap", O_RDONLY | O_CLOEXEC);
    for (size_t i = 0; i != nr_entries; ++i) {
        uint64_t entry = 0;
        auto phys = std::numeric_limits<physical_address>::max();
        auto pfn = reinterpret_cast<uintptr_t>(mem() + i * huge_page_size) / page_size;
        fd.pread(&entry, 8, pfn * 8);
        assert(entry & 0x8000'0000'0000'0000);
        phys = (entry & 0x007f'ffff'ffff'ffff) << page_bits;
        virt_to_phys_map[i] = phys;
    }
}

translation cpu_pages::translate(const void* addr, size_t size) {
    auto a = reinterpret_cast<uintptr_t>(addr) - reinterpret_cast<uintptr_t>(mem());
    auto pfn = a / huge_page_size;
    if (pfn >= virt_to_phys_map.size()) {
        return {};
    }
    auto phys = virt_to_phys_map[pfn];
    if (phys == std::numeric_limits<physical_address>::max()) {
        return {};
    }
    auto translation_size = align_up(a + 1, huge_page_size) - a;
    size = std::min(size, translation_size);
    phys += a & (huge_page_size - 1);
    return translation{phys, size};
}


void cpu_pages::do_resize(size_t new_size, allocate_system_memory_fn alloc_sys_mem) {
    std::cout<<"调用resize"<<std::endl;
    auto new_pages = new_size / page_size;
    if (new_pages <= nr_pages) {
        return;
    }
    auto old_size = nr_pages * page_size;
    auto mmap_start = memory + old_size;
    auto mmap_size = new_size - old_size;
    auto mem = alloc_sys_mem({mmap_start}, mmap_size);
    mem.release();
    ::madvise(mmap_start, mmap_size, MADV_HUGEPAGE);
    // one past last page structure is a sentinel
    auto new_page_array_pages = align_up(sizeof(page[new_pages + 1]), page_size) / page_size;
    auto new_page_array
        = reinterpret_cast<page*>(allocate_large(new_page_array_pages));
    if (!new_page_array) {
        throw std::bad_alloc();
    }
    std::copy(pages, pages + nr_pages, new_page_array);
    // mark new one-past-last page as taken to avoid boundary conditions
    new_page_array[new_pages].free = false;
    auto old_pages = reinterpret_cast<char*>(pages);
    auto old_nr_pages = nr_pages;
    auto old_pages_size = align_up(sizeof(page[nr_pages + 1]), page_size);
    pages = new_page_array;
    nr_pages = new_pages;
    auto old_pages_start = (old_pages - memory) / page_size;
    if (old_pages_start == 0) {
        // keep page 0 allocated
        old_pages_start = 1;
        old_pages_size -= page_size;
    }
    free_span(old_pages_start, old_pages_size / page_size);
    free_span(old_nr_pages, new_pages - old_nr_pages);
}

void cpu_pages::resize(size_t new_size, allocate_system_memory_fn alloc_memory) {
    new_size = align_down(new_size, huge_page_size);
    if(!cpu_pages::is_initialized()){
        std::cout<<"第一次执行初始化"<<std::endl;
        cpu_pages::initialize();
    }
    while (nr_pages * page_size < new_size) {
        // don't reallocate all at once, since there might not
        // be enough free memory available to relocate the pages array
        auto tmp_size = std::min(new_size, 4 * nr_pages * page_size);
        do_resize(tmp_size, alloc_memory);
    }
}

reclaiming_result cpu_pages::run_reclaimers(reclaimer_scope scope) {
    auto target = std::max(nr_free_pages + 1, min_free_pages);
    reclaiming_result result = reclaiming_result::reclaimed_nothing;
    while (nr_free_pages < target) {
        bool made_progress = false;
        ++g_reclaims;
        for (auto&& r : reclaimers) {
            if (r->scope() >= scope) {
                made_progress |= r->do_reclaim() == reclaiming_result::reclaimed_something;
            }
        }
        if (!made_progress) {
            return result;
        }
        result = reclaiming_result::reclaimed_something;
    }
    return result;
}

void cpu_pages::schedule_reclaim() {
    current_min_free_pages = 0;
    reclaim_hook([this] {
        if (nr_free_pages < min_free_pages) {
            try {
                run_reclaimers(reclaimer_scope::async);
            } catch (...) {
                current_min_free_pages = min_free_pages;
                throw;
            }
        }
        current_min_free_pages = min_free_pages;
    });
}

memory::memory_layout cpu_pages::memory_layout() {
    assert(is_initialized());
    return {
        reinterpret_cast<uintptr_t>(memory),
        reinterpret_cast<uintptr_t>(memory) + nr_pages * page_size
    };
}

void cpu_pages::set_reclaim_hook(std::function<void (std::function<void ()>)> hook) {
    reclaim_hook = hook;
    current_min_free_pages = min_free_pages;
}

void cpu_pages::set_min_free_pages(size_t pages) {
    if (pages > std::numeric_limits<decltype(min_free_pages)>::max()) {
        throw std::runtime_error("Number of pages too large");
    }
    min_free_pages = pages;
    maybe_reclaim();
}

small_pool::small_pool(unsigned object_size) noexcept
    : _object_size(object_size), _span_size(1) {
    while (_object_size > span_bytes()
            || (_span_size < 32 && waste() > 0.05)
            || (span_bytes() / object_size < 32)) {
        _span_size *= 2;
    }
    _max_free = std::max<unsigned>(100, span_bytes() * 2 / _object_size);
    _min_free = _max_free / 2;
}

small_pool::~small_pool() {
    _min_free = _max_free = 0;
    trim_free_list();
}

// Should not throw in case of running out of memory to avoid infinite recursion,
// becaue throwing std::bad_alloc requires allocation. __cxa_allocate_exception
// falls back to the emergency pool in case malloc() returns nullptr.
void*
small_pool::allocate() {
    if (!_free) {
        add_more_objects();
    }
    if (!_free) {
        return nullptr;
    }
    auto* obj = _free;
    _free = _free->next;
    --_free_count;
    return obj;
}

void
small_pool::deallocate(void* object) {
    auto o = reinterpret_cast<free_object*>(object);
    o->next = _free;
    _free = o;
    ++_free_count;
    if (_free_count >= _max_free) {
        trim_free_list();
    }
}

void
small_pool::add_more_objects() {
    auto goal = (_min_free + _max_free) / 2;
    while (!_span_list.empty() && _free_count < goal) {
        page& span = _span_list.front(cpu_mem.pages);
        _span_list.pop_front(cpu_mem.pages);
        while (span.freelist) {
            auto obj = span.freelist;
            span.freelist = span.freelist->next;
            obj->next = _free;
            _free = obj;
            ++_free_count;
            ++span.nr_small_alloc;
        }
    }
    while (_free_count < goal) {
        disable_backtrace_temporarily dbt;
        auto data = reinterpret_cast<char*>(cpu_mem.allocate_large(_span_size));
        if (!data) {
            return;
        }
        ++_spans_in_use;
        auto span = cpu_mem.to_page(data);
        for (unsigned i = 0; i < _span_size; ++i) {
            span[i].offset_in_span = i;
            span[i].pool = this;
        }
        span->nr_small_alloc = 0;
        span->freelist = nullptr;
        for (unsigned offset = 0; offset <= span_bytes() - _object_size; offset += _object_size) {
            auto h = reinterpret_cast<free_object*>(data + offset);
            h->next = _free;
            _free = h;
            ++_free_count;
            ++span->nr_small_alloc;
        }
    }
}

void
small_pool::trim_free_list() {
    auto goal = (_min_free + _max_free) / 2;
    while (_free && _free_count > goal) {
        auto obj = _free;
        _free = _free->next;
        --_free_count;
        page* span = cpu_mem.to_page(obj);
        span -= span->offset_in_span;
        if (!span->freelist) {
            new (&span->link) page_list_link();
            _span_list.push_front(cpu_mem.pages, *span);
        }
        obj->next = span->freelist;
        span->freelist = obj;
        if (--span->nr_small_alloc == 0) {
            _span_list.erase(cpu_mem.pages, *span);
            cpu_mem.free_span(span - cpu_mem.pages, span->span_size);
            --_spans_in_use;
        }
    }
}

float small_pool::waste() {
    return (span_bytes() % _object_size) / (1.0 * span_bytes());
}

void
abort_on_underflow(size_t size) {
    if (std::make_signed_t<size_t>(size) < 0) {
        // probably a logic error, stop hard
        abort();
    }
}

void* allocate_large(size_t size) {
    abort_on_underflow(size);
    unsigned size_in_pages = (size + page_size - 1) >> page_bits;
    std::cout<<"size:   "<<size<<"  size_in pages:  "<<size_in_pages<<std::endl;
    if ((size_t(size_in_pages) << page_bits) < size) {
        std::cout<<"allocate_large overflow"<<std::endl;
        throw std::bad_alloc();
    }
    return cpu_mem.allocate_large(size_in_pages);

}

void* allocate_large_aligned(size_t align, size_t size) {
    abort_on_underflow(size);
    unsigned size_in_pages = (size + page_size - 1) >> page_bits;
    unsigned align_in_pages = std::max(align, page_size) >> page_bits;
    return cpu_mem.allocate_large_aligned(align_in_pages, size_in_pages);
}

void free_large(void* ptr) {
    return cpu_mem.free_large(ptr);
}

size_t object_size(void* ptr) {
    return cpu_pages::all_cpus[object_cpu_id(ptr)]->object_size(ptr);
}

void* allocate(size_t size) {
    if (size <= sizeof(free_object)) {
        size = sizeof(free_object);
    }
    void* ptr;
    if (size <= max_small_allocation) {
        size = object_size_with_alloc_site(size);
        ptr = cpu_mem.allocate_small(size);
    } else {
        std::cout<<"huge alloc"<<std::endl;
        ptr = allocate_large(size);
    }
    if (!ptr) {
        on_allocation_failure(size);
    }
    ++g_allocs;
    return ptr;
}

void* allocate_aligned(size_t align, size_t size) {
    size = std::max(size, align);
    if (size <= sizeof(free_object)) {
        size = sizeof(free_object);
    }
    void* ptr;
    if (size <= max_small_allocation && align <= page_size) {
        // Our small allocator only guarantees alignment for power-of-two
        // allocations which are not larger than a page.
        size = 1 << log2ceil(object_size_with_alloc_site(size));
        ptr = cpu_mem.allocate_small(size);
    } else {
        ptr = allocate_large_aligned(align, size);
    }
    if (!ptr) {
        on_allocation_failure(size);
    }
    ++g_allocs;
    return ptr;
}

void free(void* obj) {
    if (cpu_mem.try_cross_cpu_free(obj)) {
        return;
    }
    ++g_frees;
    cpu_mem.free(obj);
}

void free(void* obj, size_t size) {
    if (cpu_mem.try_cross_cpu_free(obj)) {
        return;
    }
    ++g_frees;
    cpu_mem.free(obj, size);
}

void shrink(void* obj, size_t new_size) {
    ++g_frees;
    ++g_allocs; // keep them balanced
    cpu_mem.shrink(obj, new_size);
}

void set_reclaim_hook(std::function<void (std::function<void ()>)> hook) {
    cpu_mem.set_reclaim_hook(hook);
}

reclaimer::reclaimer(reclaim_fn reclaim, reclaimer_scope scope)
    : _reclaim(std::move(reclaim))
    , _scope(scope) {
    cpu_mem.reclaimers.push_back(this);
}

reclaimer::~reclaimer() {
    auto& r = cpu_mem.reclaimers;
    r.erase(std::find(r.begin(), r.end(), this));
}

void configure(std::vector<resource::memory> m, std::optional<std::string> hugetlbfs_path) {
    size_t total = 0;
    // 调试：显示每个内存块的详细信息
    std::cout << "=== 开始配置内存 ===" << std::endl;
    std::cout << "计算总内存:" << std::endl;
    for (size_t i = 0; i < m.size(); ++i) {
        auto&& x = m[i];
        std::cout << "  内存块[" << i 
                  << "] 大小: " << x.bytes << " 字节"
                  << (x.nodeid != static_cast<unsigned long>(-1) ? 
                     " NUMA节点: " + std::to_string(x.nodeid) : "")
                  << std::endl;
        total += x.bytes;
    }
    std::cout << "总内存量: " << total << " 字节" << std::endl;
    allocate_system_memory_fn sys_alloc = allocate_anonymous_memory; 
    //绑定实现函数.
    if (hugetlbfs_path) {
        std::cout << "检测到HugeTLBFS路径: " << *hugetlbfs_path << std::endl;
        auto fdp = make_lw_shared<file_desc>(file_desc::temporary(*hugetlbfs_path));
        sys_alloc = [fdp](std::optional<void*> where, size_t how_much) {
            return allocate_hugetlbfs_memory(*fdp, where, how_much);
        };   
        std::cout << "切换内存分配函数到HugeTLBFS专用分配器" << std::endl;
        cpu_mem.replace_memory_backing(sys_alloc);
    } else {
        std::cout << "使用默认匿名内存分配" << std::endl;
    }
    // 调试：显示内存调整操作
    std::cout << "调整内存池大小至: " << total << " 字节" << std::endl;
    cpu_mem.resize(total, sys_alloc);
    //这句代码卡住. (传入的函数都是sys_alloc)
    std::cout << "调整内存池end"<<std::endl;

    size_t pos = 0;
    for (auto&& x : m) {        
        // 调试：显示每个内存块的分配进度
        std::cout << "已分配内存块 [" << pos << " -> " << (pos + x.bytes)
                  << ") 大小: " << x.bytes << " 字节" << std::endl;
        pos += x.bytes;
    }

    if (hugetlbfs_path) {
        std::cout << "初始化HugeTLBFS虚拟地址到物理地址映射" << std::endl;
        cpu_mem.init_virt_to_phys_map();
    }
    
    std::cout << "=== 内存配置完成 ===" << std::endl;
}



statistics stats() {
    return statistics{g_allocs, g_frees, g_cross_cpu_frees,
        cpu_mem.nr_pages * page_size, cpu_mem.nr_free_pages * page_size, g_reclaims};
}

bool drain_cross_cpu_freelist() {
    return cpu_mem.drain_cross_cpu_freelist();
}

translation
translate(const void* addr, size_t size) {
    auto cpu_id = object_cpu_id(addr);
    if (cpu_id >= max_cpus) {
        return {};
    }
    auto cp = cpu_pages::all_cpus[cpu_id];
    if (!cp) {
        return {};
    }
    return cp->translate(addr, size);
}

memory_layout get_memory_layout() {
    return cpu_mem.memory_layout();
}

size_t min_free_memory() {
    return cpu_mem.min_free_pages * page_size;
}

void set_min_free_pages(size_t pages) {
    cpu_mem.set_min_free_pages(pages);
}

} // namespace memory



















inline bool engine_is_ready() {
    return local_engine != nullptr;
}


#endif // DEFAULT_ALLOCATOR

template <typename... T>
inline
void promise<T...>::abandoned() noexcept {
    if (_future) {
        assert(_state);
        assert(_state->available() || !_task);
        _future->_local_state = std::move(*_state);
        _future->_promise = nullptr;
    } else if (_state && _state->failed()) {
        report_failed_future(_state->get_exception());
    }
}

template <typename Clock>
inline
timer<Clock>::timer(callback_t&& callback) : _callback(std::move(callback)) {
}


template <typename Clock>
inline
typename timer<Clock>::time_point timer<Clock>::get_timeout() {
    return _expiry;
}


template <typename Clock>
inline
bool timer<Clock>::cancel() {
    if (!_armed) {
        return false;
    }
    _armed = false;
    if (_queued) {
        engine().del_timer(this);
        _queued = false;
    }
    return true;
}


template <typename... T>
inline
void promise<T...>::migrated() noexcept {
    if (_future) {
        _future->_promise = this;
    }
}

template <typename Clock>
inline
timer<Clock>::~timer() {
    if (_queued) {
        engine().del_timer(this);
    }
}

template <typename Clock>
inline
void timer<Clock>::readd_periodic() {
    arm_state(Clock::now() + _period.value(), {_period.value()});
    engine().queue_timer(this);
}

template <typename Clock>
inline
void timer<Clock>::arm_state(time_point until, std::optional<duration> period) {
    assert(!_armed);
    _period = period;
    _armed = true;
    _expired = false;
    _expiry = until;
    _queued = true;
}

lowres_clock::lowres_clock() {
    update();
    _timer.set_callback(&lowres_clock::update);
    _timer.arm_periodic(_granularity);
}

void lowres_clock::update() {
    using namespace std::chrono;
    auto now = steady_clock_type::now();
    auto ticks = duration_cast<milliseconds>(now.time_since_epoch()).count();
    _now.store(ticks, std::memory_order_relaxed);
}


template <typename Func>
future<io_event> io_queue::queue_request(shard_id coordinator, const io_priority_class& pc, size_t len, Func prepare_io) {
    auto start = std::chrono::steady_clock::now();
    return smp::submit_to(coordinator, [start, &pc, len, prepare_io = std::move(prepare_io), owner = engine().cpu_id()] {
        auto& queue = *(engine()._io_queue);
        unsigned weight = 1 + len/(16 << 10);
        // First time will hit here, and then we create the class. It is important
        // that we create the shared pointer in the same shard it will be used at later.
        auto& pclass = queue.find_or_create_class(pc, owner);
        pclass.bytes += len;
        pclass.ops++;
        pclass.nr_queued++;
        return queue._fq.queue(pclass.ptr, weight, [&pclass, start, prepare_io = std::move(prepare_io)] {
            pclass.nr_queued--;
            pclass.queue_time = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - start);
            return engine().submit_io(std::move(prepare_io));
        });
    });
}


template <typename Func>
future<io_event>
reactor::submit_io(Func prepare_io) {
    // return _io_context_available.wait(1).then([this, prepare_io = std::move(prepare_io)] () mutable {
    //     auto pr = std::make_unique<promise<io_event>>();
    //     iocb io;
    //     prepare_io(io);
    //     if (_aio_eventfd) {
    //         io_set_eventfd(&io, _aio_eventfd->get_fd());
    //     }
    //     auto f = pr->get_future();
    //     io.data = pr.get();
    //     _pending_aio.push_back(io);
    //     pr.release();
    //     if ((_io_queue->queued_requests() > 0) ||
    //         (_pending_aio.size() >= std::min(max_aio / 4, _io_queue->_capacity / 2))) {
    //         flush_pending_aio();
    //     }
    //     return f;
    // });
}

template <typename... T>
inline
future<T...>
promise<T...>::get_future() noexcept {
    assert(!_future && _state && !_task);
    return future<T...>(this);
}


// file_desc
// file_desc::temporary(sstring directory) {
//     // FIXME: add O_TMPFILE support one day
//     directory += "/XXXXXX";
//     std::vector<char> templat(directory.c_str(), directory.c_str() + directory.size() + 1);
//     int fd = ::mkstemp(templat.data());
//     throw_system_error_on(fd == -1);
//     int r = ::unlink(templat.data());
//     throw_system_error_on(r == -1); // leaks created file, but what can we do?
//     return file_desc(fd);
// }

// void mmap_deleter::operator()(void* ptr) const {
//     ::munmap(ptr, _size);
// }



template <typename Clock>
inline
void timer<Clock>::arm(time_point until, std::optional<duration> period) {
    arm_state(until, period);
    engine().add_timer(this);
}

template <typename Clock>
inline
void timer<Clock>::rearm(time_point until, std::optional<duration> period) {
    if (_armed) {
        cancel();
    }
    arm(until, period);
}

template <typename Clock>
inline
void timer<Clock>::arm(duration delta) {
    return arm(Clock::now() + delta);
}

template <typename Clock>
inline
void timer<Clock>::arm_periodic(duration delta) {
    arm(Clock::now() + delta, {delta});
}

template <typename Clock>
inline
void timer<Clock>::set_callback(callback_t&& callback) {
    _callback = std::move(callback);
}




template <typename... T>
template<typename promise<T...>::urgent Urgent>
inline
void promise<T...>::make_ready() noexcept {
    if (_task) {
        _state = nullptr;
        if (Urgent == urgent::yes && !need_preempt()) {
            ::schedule_urgent(std::move(_task));
        } else {
            ::schedule_normal(std::move(_task));
        }
    }
}

// #include <boost/range/algorithm.hpp>
// #include <boost/algorithm/string.hpp>
// #include <boost/algorithm/string/replace.hpp>
// #include <boost/range/algorithm_ext/erase.hpp>

// namespace metrics {

// metric_groups::metric_groups() noexcept : _impl(impl::create_metric_groups()) {
// }

// void metric_groups::clear() {
//     _impl = impl::create_metric_groups();
// }

// metric_groups::metric_groups(std::initializer_list<metric_group_definition> mg) : _impl(impl::create_metric_groups()) {
//     for (auto&& i : mg) {
//         add_group(i.name, i.metrics);
//     }
// }
// metric_groups& metric_groups::add_group(const group_name_type& name, const std::initializer_list<metric_definition>& l) {
//     _impl->add_group(name, l);
//     return *this;
// }
// metric_group::metric_group() noexcept = default;
// metric_group::~metric_group() = default;
// metric_group::metric_group(const group_name_type& name, std::initializer_list<metric_definition> l) {
//     add_group(name, l);
// }

// metric_group_definition::metric_group_definition(const group_name_type& name, std::initializer_list<metric_definition> l) : name(name), metrics(l) {
// }

// metric_group_definition::~metric_group_definition() = default;

// metric_groups::~metric_groups() = default;
// metric_definition::metric_definition(metric_definition&& m) noexcept : _impl(std::move(m._impl)) {
// }

// metric_definition::~metric_definition()  = default;

// metric_definition::metric_definition(impl::metric_definition_impl const& m) noexcept :
//     _impl(std::make_unique<impl::metric_definition_impl>(m)) {
// }

// bool label_instance::operator<(const label_instance& id2) const {
//     auto& id1 = *this;
//     return std::tie(id1.key(), id1.value())
//                 < std::tie(id2.key(), id2.value());
// }

// bool label_instance::operator==(const label_instance& id2) const {
//     auto& id1 = *this;
//     return std::tie(id1.key(), id1.value())
//                     == std::tie(id2.key(), id2.value());
// }


// static std::string get_hostname() {
//     char hostname[PATH_MAX];
//     gethostname(hostname, sizeof(hostname));
//     hostname[PATH_MAX-1] = '\0';
//     return hostname;
// }


// boost::program_options::options_description get_options_description() {
//     namespace bpo = boost::program_options;
//     bpo::options_description opts("Metrics options");
//     opts.add_options()(
//             "metrics-hostname",
//             bpo::value<std::string>()->default_value(get_hostname()),
//             "set the hostname used by the metrics, if not set, the local hostname will be used");
//     return opts;
// }

// future<> configure(const boost::program_options::variables_map & opts) {
//     impl::config c;
//     c.hostname = opts["metrics-hostname"].as<std::string>();
//     return smp::invoke_on_all([c] {
//         impl::get_local_impl()->set_config(c);
//     });
// }


// bool label_instance::operator!=(const label_instance& id2) const {
//     auto& id1 = *this;
//     return !(id1 == id2);
// }

// label shard_label("shard");
// label type_label("type");
// namespace impl {

// registered_metric::registered_metric(metric_id id, data_type type, metric_function f, description d, bool enabled) :
//         _type(type), _d(d), _enabled(enabled), _f(f), _impl(get_local_impl()), _id(id) {
// }

// metric_value metric_value::operator+(const metric_value& c) {
//     metric_value res(*this);
//     switch (_type) {
//     case data_type::HISTOGRAM:
//         boost::get<histogram>(res.u) += boost::get<histogram>(c.u);
//     default:
//         boost::get<double>(res.u) += boost::get<double>(c.u);
//         break;
//     }
//     return res;
// }

// metric_definition_impl::metric_definition_impl(
//         metric_name_type name,
//         metric_type type,
//         metric_function f,
//         description d,
//         std::vector<label_instance> _labels)
//         : name(name), type(type), f(f)
//         , d(d), enabled(true) {
//     for (auto i: _labels) {
//         labels[i.key()] = i.value();
//     }
//     if (labels.find(shard_label.name()) == labels.end()) {
//         labels[shard_label.name()] = shard();
//     }
//     if (labels.find(type_label.name()) == labels.end()) {
//         labels[type_label.name()] = type.type_name;
//     }
// }

// metric_definition_impl& metric_definition_impl::operator ()(bool _enabled) {
//     enabled = _enabled;
//     return *this;
// }

// metric_definition_impl& metric_definition_impl::operator ()(const label_instance& label) {
//     labels[label.key()] = label.value();
//     return *this;
// }

// std::unique_ptr<metric_groups_def> create_metric_groups() {
//     return  std::make_unique<metric_groups_impl>();
// }

// metric_groups_impl::~metric_groups_impl() {
//     for (auto i : _registration) {
//         unregister_metric(i);
//     }
// }

// metric_groups_impl& metric_groups_impl::add_metric(group_name_type name, const metric_definition& md)  {

//     metric_id id(name, md._impl->name, md._impl->labels);

//     shared_ptr<registered_metric> rm =
//             ::make_shared<registered_metric>(id, md._impl->type.base_type, md._impl->f, md._impl->d, md._impl->enabled);

//     get_local_impl()->add_registration(id, rm);

//     _registration.push_back(id);
//     return *this;
// }

// metric_groups_impl& metric_groups_impl::add_group(group_name_type name, const std::vector<metric_definition>& l) {
//     for (auto i = l.begin(); i != l.end(); ++i) {
//         add_metric(name, *(i->_impl.get()));
//     }
//     return *this;
// }

// metric_groups_impl& metric_groups_impl::add_group(group_name_type name, const std::initializer_list<metric_definition>& l) {
//     for (auto i = l.begin(); i != l.end(); ++i) {
//         add_metric(name, *i);
//     }
//     return *this;
// }

// bool metric_id::operator<(
//         const metric_id& id2) const {
//     return as_tuple() < id2.as_tuple();
// }

// static std::string safe_name(const std::string& name) {
//     auto rep = boost::replace_all_copy(boost::replace_all_copy(name, "-", "_"), " ", "_");
//     boost::remove_erase_if(rep, boost::is_any_of("+()"));
//     return rep;
// }

// std::string metric_id::full_name() const {
//     return safe_name(_group + "_" + _name);
// }

// bool metric_id::operator==(
//         const metric_id & id2) const {
//     return as_tuple() == id2.as_tuple();
// }

// // Unfortunately, metrics_impl can not be shared because it
// // need to be available before the first users (reactor) will call it

// shared_ptr<impl>  get_local_impl() {
//     static thread_local auto the_impl = make_shared<impl>();
//     return the_impl;
// }

// void unregister_metric(const metric_id & id) {
//     shared_ptr<impl> map = get_local_impl();
//     auto i = map->get_value_map().find(id.full_name());
//     if (i != map->get_value_map().end()) {
//         auto j = i->second.find(id.labels());
//         if (j != i->second.end()) {
//             j->second = nullptr;
//             i->second.erase(j);
//         }
//         if (i->second.empty()) {
//             map->get_value_map().erase(i);
//         }
//     }
// }

// const value_map& get_value_map() {
//     return get_local_impl()->get_value_map();
// }

// values_copy get_values() {
//     values_copy res;

//     for (auto i : get_local_impl()->get_value_map()) {
//         std::vector<std::tuple<shared_ptr<registered_metric>, metric_value>> values;
//         for (auto&& v : i.second) {
//             if (v.second.get() && v.second->is_enabled()) {
//                 values.emplace_back(v.second, (*(v.second))());
//             }
//         }
//         if (values.size() > 0) {
//             res[i.first] = std::move(values);
//         }
//     }
//     return std::move(res);
// }


// instance_id_type shard() {
//     if (engine_is_ready()) {
//         return std::to_string(engine().cpu_id());
//     }
//     return std::string("0");
// }

// void impl::add_registration(const metric_id& id, shared_ptr<registered_metric> rm) {
//     std::string name = id.full_name();
//     if (_value_map.find(name) != _value_map.end()) {
//         auto& metric = _value_map[name];
//         if (metric.find(id.labels()) != metric.end()) {
//             throw std::runtime_error("registering metrics twice for metrics: " + name);
//         }
//         if (metric.begin()->second->get_type() != rm->get_type()) {
//             throw std::runtime_error("registering metrics " + name + " registered with different type.");
//         }
//         metric[id.labels()] = rm;
//     } else {
//         _value_map[name].info().type = rm->get_type();
//         _value_map[name][id.labels()] = rm;
//     }
// }

// }

// const bool metric_disabled = false;

// histogram& histogram::operator+=(const histogram& c) {
//     for (size_t i = 0; i < c.buckets.size(); i++) {
//         if (buckets.size() <= i) {
//             buckets.push_back(c.buckets[i]);
//         } else {
//             if (buckets[i].upper_bound != c.buckets[i].upper_bound) {
//                 throw std::out_of_range("Trying to add histogram with different bucket limits");
//             }
//             buckets[i].count += c.buckets[i].count;
//         }
//     }
//     return *this;
// }

// histogram histogram::operator+(const histogram& c) const {
//     histogram res = *this;
//     res += c;
//     return res;
// }

// histogram histogram::operator+(histogram&& c) const {
//     c += *this;
//     return std::move(c);
// }

// }