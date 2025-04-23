// #ifndef FUTURE_ALL_HH
// #define FUTURE_ALL_HH

// #include "../task/task.hh"
// #include <stdexcept>
// #include <atomic>
// #include <memory>
// #include <utility>
// #include <tuple>
// #include <type_traits>
// #include "../util/shared_ptr.hh"
// #include <assert.h>
// #include <cstdlib>
// #include <chrono>
// #include <functional>
// #include <type_traits>
// #include <setjmp.h>
// #include <optional>
// #include "do_with.hh"
// #include "../fd/posix.hh"
// #include <chrono>
// #include <boost/intrusive/list.hpp>
// #include <setjmp.h>
// #include <ucontext.h>
// #include <list>
// #include "../resource/resource.hh"
// #include <chrono>
// #include <limits>
// #include <bitset>
// #include <array>
// #include <atomic>
// #include <list>
// #include <deque>
// #include <unordered_map>
// #include <boost/program_options.hpp>
// #include <boost/filesystem.hpp>
// #include <experimental/optional>
// #include <iostream>
// #include <time.h>
// #include <signal.h>
// #include <thread>
// #include <iomanip>
// #include "../mem/memory.hh"
// #include <mutex>  // 为std::lock_guard添加
// #include <stdexcept>
// #include <exception>
// #include <deque>
// #include <unordered_set>
// #include <queue>
// #include <libaio.h>




// #ifdef __cpp_concepts
// #define GCC6_CONCEPT(x...) x
// #define GCC6_NO_CONCEPT(x...)
// #else
// #define GCC6_CONCEPT(x...)
// #define GCC6_NO_CONCEPT(x...) x
// #endif


// extern __thread bool g_need_preempt;
// inline bool need_preempt() {
//     return true;
//     // prevent compiler from eliminating loads in a loop
//     std::atomic_signal_fence(std::memory_order_seq_cst);
//     return g_need_preempt;
// }
// using shard_id = unsigned;

// using namespace std::chrono_literals;
// std::ostream& operator<<(std::ostream& os, const std::chrono::steady_clock::time_point& tp) {
//     auto duration = tp.time_since_epoch();
//     auto hours = std::chrono::duration_cast<std::chrono::hours>(duration);
//     duration -= hours;
//     auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration);
//     duration -= minutes;
//     auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
//     os << hours.count() << "h " << minutes.count() << "m " << seconds.count() << "s";
//     return os;
// }

// #include "../util/align.hh"
// class posix_thread {
// public:
//     class attr;
// private:
//     // must allocate, since this class is moveable
//     std::unique_ptr<std::function<void ()>> _func;
//     pthread_t _pthread;
//     bool _valid = true;
//     mmap_area _stack;
// private:
//     static void* start_routine(void* arg) noexcept;
// public:
//     posix_thread(std::function<void ()> func);
//     posix_thread(attr a, std::function<void ()> func);
//     posix_thread(posix_thread&& x);
//     ~posix_thread();
//     void join();
// public:
//     class attr {
//     public:
//         struct stack_size { size_t size = 0; };
//         attr() = default;
//         template <typename... A>
//         attr(A... a) {
//             set(std::forward<A>(a)...);
//         }
//         void set() {}
//         template <typename A, typename... Rest>
//         void set(A a, Rest... rest) {
//             set(std::forward<A>(a));
//             set(std::forward<Rest>(rest)...);
//         }
//         void set(stack_size ss) { _stack_size = ss; }
//     private:
//         stack_size _stack_size;
//         friend class posix_thread;
//     };
// };


// #include <atomic>
// #include <boost/mpl/range_c.hpp>
// #include <boost/mpl/for_each.hpp>
// #include "../util/align.hh"
// #include "../util/spinlock.hh"

// static constexpr size_t  cacheline_size = 64;
// template <size_t N, int RW, int LOC>
// struct prefetcher;

// template<int RW, int LOC>
// struct prefetcher<0, RW, LOC> {
//     prefetcher(uintptr_t ptr) {}
// };

// template <size_t N, int RW, int LOC>
// struct prefetcher {
//     prefetcher(uintptr_t ptr) {
//         __builtin_prefetch(reinterpret_cast<void*>(ptr), RW, LOC);
//         std::atomic_signal_fence(std::memory_order_seq_cst);
//         prefetcher<N-64, RW, LOC>(ptr + 64);
//     }
// };
// template<typename T, int LOC = 3>
// void prefetch(T* ptr) {
//     prefetcher<align_up(sizeof(T), cacheline_size), 0, LOC>(reinterpret_cast<uintptr_t>(ptr));
// }

// template<typename Iterator, int LOC = 3>
// void prefetch(Iterator begin, Iterator end) {
//     std::for_each(begin, end, [] (auto v) { prefetch<decltype(*v), LOC>(v); });
// }

// template<size_t C, typename T, int LOC = 3>
// void prefetch_n(T** pptr) {
//     boost::mpl::for_each< boost::mpl::range_c<size_t,0,C> >( [pptr] (size_t x) { prefetch<T, LOC>(*(pptr + x)); } );
// }

// template<size_t L, int LOC = 3>
// void prefetch(void* ptr) {
//     prefetcher<L*cacheline_size, 0, LOC>(reinterpret_cast<uintptr_t>(ptr));
// }

// template<size_t L, typename Iterator, int LOC = 3>
// void prefetch_n(Iterator begin, Iterator end) {
//     std::for_each(begin, end, [] (auto v) { prefetch<L, LOC>(v); });
// }

// template<size_t L, size_t C, typename T, int LOC = 3>
// void prefetch_n(T** pptr) {
//     boost::mpl::for_each< boost::mpl::range_c<size_t,0,C> >( [pptr] (size_t x) { prefetch<L, LOC>(*(pptr + x)); } );
// }

// template<typename T, int LOC = 3>
// void prefetchw(T* ptr) {
//     prefetcher<align_up(sizeof(T), cacheline_size), 1, LOC>(reinterpret_cast<uintptr_t>(ptr));
// }
// template<typename Iterator, int LOC = 3>
// void prefetchw_n(Iterator begin, Iterator end) {
//     std::for_each(begin, end, [] (auto v) { prefetchw<decltype(*v), LOC>(v); });
// }
// template<size_t C, typename T, int LOC = 3>
// void prefetchw_n(T** pptr) {
//     boost::mpl::for_each< boost::mpl::range_c<size_t,0,C> >( [pptr] (size_t x) { prefetchw<T, LOC>(*(pptr + x)); } );
// }
// template<size_t L, int LOC = 3>
// void prefetchw(void* ptr) {
//     prefetcher<L*cacheline_size, 1, LOC>(reinterpret_cast<uintptr_t>(ptr));
// }
// template<size_t L, typename Iterator, int LOC = 3>
// void prefetchw_n(Iterator begin, Iterator end) {
//    std::for_each(begin, end, [] (auto v) { prefetchw<L, LOC>(v); });
// }

// template<size_t L, size_t C, typename T, int LOC = 3>
// void prefetchw_n(T** pptr) {
//     boost::mpl::for_each< boost::mpl::range_c<size_t,0,C> >( [pptr] (size_t x) { prefetchw<L, LOC>(*(pptr + x)); } );
// }

// #include <bitset>
// #include <limits>

// namespace bitsets {
// static constexpr int ulong_bits = std::numeric_limits<unsigned long>::digits;
// template<typename T>
// inline size_t count_leading_zeros(T value);
// template<typename T>
// static inline size_t count_trailing_zeros(T value);
// template<>
// inline size_t count_leading_zeros<unsigned long>(unsigned long value)
// {
//     return __builtin_clzl(value);
// }
// template<>
// inline size_t count_leading_zeros<long>(long value)
// {
//     return __builtin_clzl((unsigned long)value) - 1;
// }
// template<>
// inline size_t count_leading_zeros<long long>(long long value)
// {
//     return __builtin_clzll((unsigned long long)value) - 1;
// }
// template<>
// inline
// size_t count_trailing_zeros<unsigned long>(unsigned long value)
// {
//     return __builtin_ctzl(value);
// }

// template<>
// inline
// size_t count_trailing_zeros<long>(long value)
// {
//     return __builtin_ctzl((unsigned long)value);
// }
// template<size_t N>
// static inline size_t get_first_set(const std::bitset<N>& bitset)
// {
//     static_assert(N <= ulong_bits, "bitset too large");
//     return count_trailing_zeros(bitset.to_ulong());
// }

// template<size_t N>
// static inline size_t get_last_set(const std::bitset<N>& bitset)
// {
//     static_assert(N <= ulong_bits, "bitset too large");
//     return ulong_bits - 1 - count_leading_zeros(bitset.to_ulong());
// }

// template<size_t N>
// class set_iterator : public std::iterator<std::input_iterator_tag, int>
// {
// private:
//     void advance()
//     {
//         if (_bitset.none()) {
//             _index = -1;
//         } else {
//             auto shift = get_first_set(_bitset) + 1;
//             _index += shift;
//             _bitset >>= shift;
//         }
//     }
// public:
//     set_iterator(std::bitset<N> bitset, int offset = 0)
//         : _bitset(bitset)
//         , _index(offset - 1)
//     {
//         static_assert(N <= ulong_bits, "This implementation is inefficient for large bitsets");
//         _bitset >>= offset;
//         advance();
//     }

//     void operator++()
//     {
//         advance();
//     }

//     int operator*() const
//     {
//         return _index;
//     }

//     bool operator==(const set_iterator& other) const
//     {
//         return _index == other._index;
//     }

//     bool operator!=(const set_iterator& other) const
//     {
//         return !(*this == other);
//     }
// private:
//     std::bitset<N> _bitset;
//     int _index;
// };

// template<size_t N>
// class set_range
// {
// public:
//     using iterator = set_iterator<N>;
//     using value_type = int;

//     set_range(std::bitset<N> bitset, int offset = 0)
//         : _bitset(bitset)
//         , _offset(offset)
//     {
//     }

//     iterator begin() const { return iterator(_bitset, _offset); }
//     iterator end() const { return iterator(0); }
// private:
//     std::bitset<N> _bitset;
//     int _offset;
// };

// template<size_t N>
// static inline set_range<N> for_each_set(std::bitset<N> bitset, int offset = 0)
// {
//     return set_range<N>(bitset, offset);
// }

// }




// inline
// sigset_t make_empty_sigset_mask() {
//     sigset_t set;
//     sigemptyset(&set);
//     return set;
// }

// inline int alarm_signal() {
//     // We don't want to use SIGALRM, because the boost unit test library
//     // also plays with it.
//     return SIGALRM;
//     return SIGRTMIN;
// }

// inline
// sigset_t make_sigset_mask(int signo) {
//     sigset_t set;
//     sigemptyset(&set);
//     sigaddset(&set, signo);
//     return set;
// }

// template <typename T>
// inline
// void throw_pthread_error(T r) {
//     if (r != 0) {
//         throw std::system_error(r, std::system_category());
//     }
// }



// struct signals {
//         signals();
//         ~signals();
//         bool poll_signal();
//         bool pure_poll_signal() const;
//         void handle_signal(int signo, std::function<void ()>&& handler);
//         void handle_signal_once(int signo, std::function<void ()>&& handler);
//         static void action(int signo, siginfo_t* siginfo, void* ignore);
//         struct signal_handler {
//             signal_handler(int signo, std::function<void ()>&& handler);
//             std::function<void ()> _handler;
//         };
//         std::atomic<uint64_t> _pending_signals;
//         std::unordered_map<int, signal_handler> _signal_handlers;
        
// };









// class reactor;
// reactor& engine();
// template <typename Clock> class timer;
// using steady_clock_type = std::chrono::steady_clock;


// void schedule_normal(std::unique_ptr<task> t);
// void schedule_urgent(std::unique_ptr<task> t);


// class manual_clock {
//     public:
//         using rep = int64_t;
//         using period = std::chrono::nanoseconds::period;
//         using duration = std::chrono::duration<rep, period>;
//         using time_point = std::chrono::time_point<manual_clock, duration>;
//     private:
//         static std::atomic<rep> _now;
//     public:
//         manual_clock();
//         static time_point now() {
//             return time_point(duration(_now.load(std::memory_order_relaxed)));
//         }
//         static void advance(duration d);
//         static void expire_timers();
// };



// class lowres_clock {
// public:
//     typedef int64_t rep;
//     // The lowres_clock's resolution is 10ms. However, to make it is easier to
//     // do calcuations with std::chrono::milliseconds, we make the clock's
//     // period to 1ms instead of 10ms.
//     typedef std::ratio<1, 1000> period;
//     typedef std::chrono::duration<rep, period> duration;
//     typedef std::chrono::time_point<lowres_clock, duration> time_point;
//     lowres_clock();
//     ~lowres_clock();
//     static time_point now() {
//         auto nr = _now.load(std::memory_order_relaxed);
//         return time_point(duration(nr));
//     }
// private:
//     static void update();
//     // _now is updated by cpu0 and read by other cpus. Make _now on its own
//     // cache line to avoid false sharing.
//     static std::atomic<rep> _now [[gnu::aligned(64)]];
//     // High resolution timer to drive this low resolution clock
//     struct timer_deleter {
//         void operator()(void*) const;
//     };
//     std::unique_ptr<void, timer_deleter> _timer;
//     // High resolution timer expires every 10 milliseconds
//     static constexpr std::chrono::milliseconds _granularity{10};
// };




// // Utility function for error handling
// inline void throw_system_error_on(bool condition) {
//     if (condition) {
//         throw std::system_error(errno, std::system_category());
//     }
// }

// using steady_clock_type = std::chrono::steady_clock;
// // timer 有默认参数,所以timer<>表示一个steady_clock_type
// template <typename Clock = steady_clock_type>
// class timer {
// public:
//     timer();
//     using time_point = typename Clock::time_point;
//     using duration = typename Clock::duration;
//     typedef Clock clock;
//     using callback_t = std::function<void()>;
//     using iterator = typename std::list<timer*>::iterator;
//     iterator it; // 新增的迭代器成员
//     iterator expired_it;  // 过期链表中的位置
//     // boost::intrusive::list_member_hook<> _link;
//     callback_t _callback;
//     time_point _expiry; //到期时间点,每个timer都有一个到期时间点.
//     std::experimental::optional<duration> _period;
//     bool _armed = false;
//     bool _queued = false;
//     bool _expired = false;
//     void readd_periodic();
//     void arm_state(time_point until, std::experimental::optional<duration> period);
//     timer(timer&& t) noexcept;
//     explicit timer(callback_t&& callback);
//     ~timer();
//     // future<> expired();
//     void set_callback(callback_t&& callback);
//     void arm(time_point until, std::experimental::optional<duration> period = {});
//     void rearm(time_point until, std::experimental::optional<duration> period = {});
//     void rearm(duration delta) { rearm(Clock::now() + delta); }
//     void arm(duration delta);
//     void arm_periodic(duration delta);
//     bool armed() const { return _armed; }
//     bool cancel();
//     time_point get_timeout();
// };


// // Forward declarations for timer-related functions
// void enable_timer(steady_clock_type::time_point when);

// // Function declarations
// bool queue_timer(timer<steady_clock_type>* tmr);
// void add_timer(timer<steady_clock_type>* tmr);
// void del_timer(timer<steady_clock_type>* tmr);

// bool queue_timer(timer<lowres_clock>* tmr);
// void add_timer(timer<lowres_clock>* tmr);
// void del_timer(timer<lowres_clock>* tmr);

// bool queue_timer(timer<manual_clock>* tmr);
// void add_timer(timer<manual_clock>* tmr);
// void del_timer(timer<manual_clock>* tmr);

// template<typename Timer>
// class timer_set {
// public:
//     using time_point = typename Timer::time_point;
//     using timer_list_t = std::list<Timer*>;
//     using duration = typename Timer::duration;
//     using timestamp_t = typename duration::rep;
//     static constexpr timestamp_t max_timestamp = std::numeric_limits<timestamp_t>::max();
//     static constexpr int timestamp_bits = std::numeric_limits<timestamp_t>::digits;//63
//     static constexpr int n_buckets = timestamp_bits + 1;//64
//     std::array<timer_list_t, n_buckets> _buckets;
//     timestamp_t _last;
//     timestamp_t _next; 
//     std::bitset<n_buckets> _non_empty_buckets;
//     /// \brief 获取时间点对应的时间戳（计数）
//     /// \param tp 时间点对象
//     /// \return 时间点自纪元以来的计数值
//     static timestamp_t get_timestamp(time_point tp) {
//         return tp.time_since_epoch().count();
//     }
//     /// \brief 获取定时器的超时时间戳
//     /// \param timer 定时器对象
//     /// \return 定时器超时时间的时间戳
//     static timestamp_t get_timestamp(Timer& timer) {
//         return get_timestamp(timer.get_timeout());
//     }
//     /// \brief 根据时间戳计算对应的桶索引
//     /// \param timestamp 要计算的定时器时间戳
//     /// \return 对应的桶索引
//     int get_index(timestamp_t timestamp) const {
//         if (timestamp <= _last) {
//             return n_buckets - 1;
//         }
//         auto index = bitsets::count_leading_zeros(timestamp ^ _last);
//         assert(index < n_buckets - 1);
//         return index;
//     }
//     /*
//     这个需要手动推导一下
//     */

//     /// \brief 获取定时器对应的桶索引
//     /// \param timer 定时器对象
//     /// \return 对应的桶索引
//     int get_index(Timer& timer) const {
//         return get_index(get_timestamp(timer));
//     }
//     /*
//     一个timer有唯一的一个过期时间。
//     */

//     /// \brief 获取最后一个非空桶的索引
//     /// \return 最后一个非空桶的索引
//     int get_last_non_empty_bucket() const {
//         return bitsets::get_last_set(_non_empty_buckets);
//     }

// public:
//     /// \brief 构造函数初始化成员变量
//     timer_set() : _last(0), _next(max_timestamp), _non_empty_buckets(0) {}

//     ~timer_set() {
//         // 清理所有定时器资源
//         for (auto& list : _buckets) {
//             while (!list.empty()) {
//                 auto* timer = list.front();
//                 list.pop_front();
//                 timer->cancel();
//             }
//         }
//     }

//     /// \brief 将定时器插入到对应的桶中
//     /// \param timer 要插入的定时器对象
//     /// \return true 如果插入后_next被更新为更小的值，否则false
//     bool insert(Timer& timer) {
//         auto timestamp = get_timestamp(timer);
//         auto index = get_index(timestamp);
//         auto& list = _buckets[index];
//         list.push_back(&timer);
//         timer.it = --list.end();//timer.it是timer在list中的迭代器(使用尾插法,所以end前一个位置就是最后一个元素前开后闭)
//         _non_empty_buckets[index] = true;
//         if (timestamp < _next) {
//             _next = timestamp;
//             return true;
//         }
//         return false;
//     }
//     /*
//         next是边插入边维护的一个变量.表示下一次过期的时间.
//     */

//     /// \brief 从集合中移除定时器
//     /// \param timer 要移除的定时器对象
//     void remove(Timer& timer) {
//         auto index = get_index(timer);
//         auto& list = _buckets[index];
//         list.erase(timer.it);//erase一个节点会造成内存泄漏吗? 不会:见 STL源码, 解析
//         if (list.empty()) {
//             _non_empty_buckets[index] = false;
//         }
//     }
//     /** 
//      * 
//      * 这个地方像是RTOS优先级位图
//      * 
//     */
//     /// \brief 获取已到期的定时器列表
//     /// \param now 当前时间点
//     /// \return 包含所有已到期定时器的列表
//     timer_list_t expire(time_point now) {
//         timer_list_t exp;
//         auto timestamp = get_timestamp(now);
//         if (timestamp < _last) {
//             abort();
//         }
//         //当前时间一定>=_last
//         auto index = get_index(timestamp);
//         // 处理所有在当前时间之前的非空桶
//         for (int i : bitsets::for_each_set(_non_empty_buckets, index + 1)) {
//             exp.splice(exp.end(), _buckets[i]);
//             _non_empty_buckets[i] = false;
//         }
//         /*
//             把所有过期的链表添加到exp后面(exp是一个临时的链表，操作时间间复杂度O(1)).
//         */
//         _last = timestamp;//所以_last就是最后一次处理过期定时器的时间.
//         _next = max_timestamp;//_next设置为无穷.
//         auto& list = _buckets[index];
//         // 处理当前索引的桶中的定时器
//         while (!list.empty()) {
//             auto* timer = list.front();
//             list.pop_front();
//             if (timer->get_timeout() <= now) {
//                 exp.push_back(timer);
//             } else {
//                 insert(*timer);
//             }
//         }
//         _non_empty_buckets[index] = !list.empty();
//         if (_next == max_timestamp && _non_empty_buckets.any()) {
//             // 更新_next为最后一个非空桶中的最小时间戳
//             for (auto* timer : _buckets[get_last_non_empty_bucket()]) {
//                 _next = std::min(_next, get_timestamp(*timer));
//             }
//         }
//         return exp;//返回这个链表
//     }

//     time_point get_next_timeout() const {
//         return time_point(duration(std::max(_last, _next)));
//     }
//     void clear() {
//         for (auto& list : _buckets) {
//             list.clear();
//         }
//         _non_empty_buckets.reset();
//     }
//     size_t size() const {
//         size_t res = 0;
//         for (const auto& list : _buckets) {
//             res += list.size();
//         }
//         return res;
//     }
//     bool empty() const {
//         return _non_empty_buckets.none();
//     }
//     time_point now() {
//         return Timer::clock::now();
//     }
// };


// template<typename T>
// struct function_traits;

// template<typename Ret, typename... Args>
// struct function_traits<Ret(Args...)>
// {
//     using return_type = Ret;
//     using args_as_tuple = std::tuple<Args...>;
//     using signature = Ret (Args...);
 
//     static constexpr std::size_t arity = sizeof...(Args);
 
//     template <std::size_t N>
//     struct arg
//     {
//         static_assert(N < arity, "no such parameter index.");
//         using type = typename std::tuple_element<N, std::tuple<Args...>>::type;
//     };
// };

// template<typename Ret, typename... Args>
// struct function_traits<Ret(*)(Args...)> : public function_traits<Ret(Args...)>
// {};

// template <typename T, typename Ret, typename... Args>
// struct function_traits<Ret(T::*)(Args...)> : public function_traits<Ret(Args...)>
// {};

// template <typename T, typename Ret, typename... Args>
// struct function_traits<Ret(T::*)(Args...) const> : public function_traits<Ret(Args...)>
// {};

// template <typename T>
// struct function_traits : public function_traits<decltype(&T::operator())>
// {};

// template<typename T>
// struct function_traits<T&> : public function_traits<std::remove_reference_t<T>>
// {};
// template <typename... T> class future;
// template <typename... T> class promise;
// template <typename... T> struct future_state;


// /// \cond internal
// template <typename... T>
// struct future_state {
//     static constexpr bool copy_noexcept = std::is_nothrow_copy_constructible<std::tuple<T...>>::value;
//     static_assert(std::is_nothrow_move_constructible<std::tuple<T...>>::value,
//                   "Types must be no-throw move constructible");
//     static_assert(std::is_nothrow_destructible<std::tuple<T...>>::value,
//                   "Types must be no-throw destructible");
//     static_assert(std::is_nothrow_copy_constructible<std::exception_ptr>::value,
//                   "std::exception_ptr's copy constructor must not throw");
//     static_assert(std::is_nothrow_move_constructible<std::exception_ptr>::value,
//                   "std::exception_ptr's move constructor must not throw");
//     enum class state {
//          invalid,
//          future,
//          result,
//          exception,
//     } _state = state::future;
//     union any {
//         any() {}
//         ~any() {}
//         std::tuple<T...> value;
//         std::exception_ptr ex;
//     } _u;
//     future_state() noexcept {}
//     [[gnu::always_inline]]
//     future_state(future_state&& x) noexcept
//             : _state(x._state) {
//         switch (_state) {
//         case state::future:
//             break;
//         case state::result:
//             new (&_u.value) std::tuple<T...>(std::move(x._u.value));
//             x._u.value.~tuple();
//             break;
//         case state::exception:
//             new (&_u.ex) std::exception_ptr(std::move(x._u.ex));
//             x._u.ex.~exception_ptr();
//             break;
//         case state::invalid:
//             break;
//         default:
//             abort();
//         }
//         x._state = state::invalid;
//     }
//     __attribute__((always_inline))
//     ~future_state() noexcept {
//         switch (_state) {
//         case state::invalid:
//             break;
//         case state::future:
//             break;
//         case state::result:
//             _u.value.~tuple();
//             break;
//         case state::exception:
//             _u.ex.~exception_ptr();
//             break;
//         default:
//             abort();
//         }
//     }
//     future_state& operator=(future_state&& x) noexcept {
//         if (this != &x) {
//             this->~future_state();
//             new (this) future_state(std::move(x));
//         }
//         return *this;
//     }
//     bool available() const noexcept { return _state == state::result || _state == state::exception; }
//     bool failed() const noexcept { return _state == state::exception; }
//     void wait();
//     void set(const std::tuple<T...>& value) noexcept {
//         assert(_state == state::future);
//         new (&_u.value) std::tuple<T...>(value);
//         _state = state::result;
//     }
//     void set(std::tuple<T...>&& value) noexcept {
//         assert(_state == state::future);
//         new (&_u.value) std::tuple<T...>(std::move(value));
//         _state = state::result;
//     }
//     template <typename... A>
//     void set(A&&... a) {
//         assert(_state == state::future);
//         new (&_u.value) std::tuple<T...>(std::forward<A>(a)...);
//         _state = state::result;
//     }
//     void set_exception(std::exception_ptr ex) noexcept {
//         assert(_state == state::future);
//         new (&_u.ex) std::exception_ptr(ex);
//         _state = state::exception;
//     }
//     std::exception_ptr get_exception() && noexcept {
//         assert(_state == state::exception);
//         // Move ex out so future::~future() knows we've handled it
//         _state = state::invalid;
//         auto ex = std::move(_u.ex);
//         _u.ex.~exception_ptr();
//         return ex;
//     }
//     std::exception_ptr get_exception() const& noexcept {
//         assert(_state == state::exception);
//         return _u.ex;
//     }
//     std::tuple<T...> get_value() && noexcept {
//         assert(_state == state::result);
//         return std::move(_u.value);
//     }
//     template<typename U = std::tuple<T...>>
//     std::enable_if_t<std::is_copy_constructible<U>::value, U> get_value() const& noexcept(copy_noexcept) {
//         assert(_state == state::result);
//         return _u.value;
//     }
//     std::tuple<T...> get() && {
//         assert(_state != state::future);
//         if (_state == state::exception) {
//             _state = state::invalid;
//             auto ex = std::move(_u.ex);
//             _u.ex.~exception_ptr();
//             // Move ex out so future::~future() knows we've handled it
//             std::rethrow_exception(std::move(ex));
//         }
//         return std::move(_u.value);
//     }
//     std::tuple<T...> get() const& {
//         assert(_state != state::future);
//         if (_state == state::exception) {
//             std::rethrow_exception(_u.ex);
//         }
//         return _u.value;
//     }
//     void ignore() noexcept {
//         assert(_state != state::future);
//         this->~future_state();
//         _state = state::invalid;
//     }
//     using get0_return_type = std::tuple_element_t<0, std::tuple<T...>>;
//     static get0_return_type get0(std::tuple<T...>&& x) {
//         return std::get<0>(std::move(x));
//     }
//     void forward_to(promise<T...>& pr) noexcept;
//         // assert(_state != state::future);
//         // if (_state == state::exception) {
//         //     pr.set_urgent_exception(std::move(_u.ex));
//         //     _u.ex.~exception_ptr();
//         // } else {
//         //     pr.set_urgent_value(std::move(_u.value));
//         //     _u.value.~tuple();
//         // }
//         // _state = state::invalid;
//     //}
// };

// template <>
// struct future_state<> {
//     static_assert(sizeof(std::exception_ptr) == sizeof(void*), "exception_ptr not a pointer");
//     static_assert(std::is_nothrow_copy_constructible<std::exception_ptr>::value,
//                   "std::exception_ptr's copy constructor must not throw");
//     static_assert(std::is_nothrow_move_constructible<std::exception_ptr>::value,
//                   "std::exception_ptr's move constructor must not throw");
//     static constexpr bool copy_noexcept = true;
//     enum class state : uintptr_t {
//          invalid = 0,
//          future = 1,
//          result = 2,
//          exception_min = 3,  // or anything greater
//     };
//     union any {
//         any() { st = state::future; }
//         ~any() {}
//         state st;
//         std::exception_ptr ex;
//     } _u;
//     future_state() noexcept {}
//     [[gnu::always_inline]]
//     future_state(future_state&& x) noexcept {
//         if (x._u.st < state::exception_min) {
//             _u.st = x._u.st;
//         } else {
//             // Move ex out so future::~future() knows we've handled it
//             // Moving it will reset us to invalid state
//             new (&_u.ex) std::exception_ptr(std::move(x._u.ex));
//             x._u.ex.~exception_ptr();
//         }
//         x._u.st = state::invalid;
//     }
//     [[gnu::always_inline]]
//     ~future_state() noexcept {
//         if (_u.st >= state::exception_min) {
//             _u.ex.~exception_ptr();
//         }
//     }
//     future_state& operator=(future_state&& x) noexcept {
//         if (this != &x) {
//             this->~future_state();
//             new (this) future_state(std::move(x));
//         }
//         return *this;
//     }
//     bool available() const noexcept { return _u.st == state::result || _u.st >= state::exception_min; }
//     bool failed() const noexcept { return _u.st >= state::exception_min; }
//     void set(const std::tuple<>& value) noexcept {
//         assert(_u.st == state::future);
//         _u.st = state::result;
//     }
//     void set(std::tuple<>&& value) noexcept {
//         assert(_u.st == state::future);
//         _u.st = state::result;
//     }
//     void set() {
//         assert(_u.st == state::future);
//         _u.st = state::result;//
//     }
//     void set_exception(std::exception_ptr ex) noexcept {
//         assert(_u.st == state::future);
//         new (&_u.ex) std::exception_ptr(ex);
//         assert(_u.st >= state::exception_min);
//     }
//     std::tuple<> get() && {
//         assert(_u.st != state::future);
//         if (_u.st >= state::exception_min) {
//             // Move ex out so future::~future() knows we've handled it
//             // Moving it will reset us to invalid state
//             std::rethrow_exception(std::move(_u.ex));
//         }
//         return {};
//     }
//     std::tuple<> get() const& {
//         assert(_u.st != state::future);
//         if (_u.st >= state::exception_min) {
//             std::rethrow_exception(_u.ex);
//         }
//         return {};
//     }
//     void ignore() noexcept {
//         assert(_u.st != state::future);
//         this->~future_state();
//         _u.st = state::invalid;
//     }
//     using get0_return_type = void;
//     static get0_return_type get0(std::tuple<>&&) {
//         return;
//     }
//     std::exception_ptr get_exception() && noexcept {
//         assert(_u.st >= state::exception_min);
//         // Move ex out so future::~future() knows we've handled it
//         // Moving it will reset us to invalid state
//         return std::move(_u.ex);
//     }
//     std::exception_ptr get_exception() const& noexcept {
//         assert(_u.st >= state::exception_min);
//         return _u.ex;
//     }
//     std::tuple<> get_value() const noexcept {
//         assert(_u.st == state::result);
//         return {};
//     }
//     void forward_to(promise<>& pr) noexcept;
//     //     assert(_u.st != state::future && _u.st != state::invalid);
//     //     if (_u.st >= state::exception_min) {
//     //         pr.set_urgent_exception(std::move(_u.ex));
//     //         _u.ex.~exception_ptr();
//     //     }else{
//     //         pr.set_urgent_value(std::tuple<>());
//     //     }
//     //     _u.st = state::invalid;
//     // }
// };

// template <typename Func, typename... T>
// struct continuation final : task {
//     continuation(Func&& func, future_state<T...>&& state) : _state(std::move(state)), _func(std::move(func)) {}
//     continuation(Func&& func) : _func(std::move(func)) {}
//     virtual void run() noexcept override {
//         _func(std::move(_state));
//     }
//     future_state<T...> _state;
//     Func _func;
// };

// template <typename... T>
// class future;
// template <typename... T> struct is_future : std::false_type {};
// template <typename... T> struct is_future<future<T...>> : std::true_type {};
// struct ready_future_marker {};
// struct ready_future_from_tuple_marker {};
// struct exception_future_marker {};
// template <typename T>
// struct futurize;

// template <typename T>
// using futurize_t = typename futurize<T>::type;


// template <typename T>
// struct futurize {
//     /// If \c T is a future, \c T; otherwise \c future<T>
//     using type = future<T>;
//     /// The promise type associated with \c type.
//     using promise_type = promise<T>;
//     /// The value tuple type associated with \c type
//     using value_type = std::tuple<T>;

//     /// Apply a function to an argument list (expressed as a tuple)
//     /// and return the result, as a future (if it wasn't already).
//     template<typename Func, typename... FuncArgs>
//     static inline type apply(Func&& func, std::tuple<FuncArgs...>&& args) noexcept;

//     /// Apply a function to an argument list
//     /// and return the result, as a future (if it wasn't already).
//     template<typename Func, typename... FuncArgs>
//     static inline type apply(Func&& func, FuncArgs&&... args) noexcept;
//     /// Convert a value or a future to a future
//     static inline type convert(T&& value) {  
//         return make_ready_future<T>(std::move(value)); 
//     }
//     // 如果convert传入的是值, 使用 make_ready_future 转为future类型

//     static inline type convert(type&& value){ 
//         return std::move(value); 
//     }
//     // 如果convert传入的是future，直接把future使用std::move变为右值.

//     /// Convert the tuple representation into a future
//     static type from_tuple(value_type&& value);
//     /// Convert the tuple representation into a future
//     static type from_tuple(const value_type& value);

//     /// Makes an exceptional future of type \ref type.
//     template <typename Arg>
//     static type make_exception_future(Arg&& arg);
// };

// /// \cond internal
// template <>
// struct futurize<void> {
//     using type = future<>;
//     using promise_type = promise<>;
//     using value_type = std::tuple<>;

//     template<typename Func, typename... FuncArgs>
//     static inline type apply(Func&& func, std::tuple<FuncArgs...>&& args) noexcept;

//     template<typename Func, typename... FuncArgs>
//     static inline type apply(Func&& func, FuncArgs&&... args) noexcept;

//     static inline type from_tuple(value_type&& value);
//     static inline type from_tuple(const value_type& value);

//     template <typename Arg>
//     static type make_exception_future(Arg&& arg);
// };

// template <typename... Args>
// struct futurize<future<Args...>> {
//     using type = future<Args...>;
//     using promise_type = promise<Args...>;

//     template<typename Func, typename... FuncArgs>
//     static inline type apply(Func&& func, std::tuple<FuncArgs...>&& args) noexcept;

//     template<typename Func, typename... FuncArgs>
//     static inline type apply(Func&& func, FuncArgs&&... args) noexcept;

//     static inline type convert(Args&&... values) { return make_ready_future<Args...>(std::move(values)...); }
//     static inline type convert(type&& value) { return std::move(value); }

//     template <typename Arg>
//     static type make_exception_future(Arg&& arg);
// };


// GCC6_CONCEPT(
// template <typename T>
// concept Future = is_future<T>::value;

// template <typename Func, typename... T>
// concept CanApply = requires (Func f, T... args) {
//     f(std::forward<T>(args)...);
// };

// template <typename Func, typename Return, typename... T>
// concept ApplyReturns = requires (Func f, T... args) {
//     { f(std::forward<T>(args)...) } -> std::convertible_to<Return>;
// };

// template <typename Func, typename... T>
// concept ApplyReturnsAnyFuture = requires (Func f, T... args) {
//     requires is_future<decltype(f(std::forward<T>(args)...))>::value;
// };
// )

// void engine_exit(std::exception_ptr eptr = {});
// void report_failed_future(std::exception_ptr ex);

// template <typename... T>
// class future {
//     promise<T...>* _promise;
//     future_state<T...> _local_state;  // valid if !_promise
//     static constexpr bool copy_noexcept = future_state<T...>::copy_noexcept;
// private:
//     future(promise<T...>* pr) noexcept : _promise(pr) {
//         _promise->_future = this;
//     }
//     template <typename... A>
//     future(ready_future_marker, A&&... a) : _promise(nullptr) {
//         _local_state.set(std::forward<A>(a)...);
//     }
//     template <typename... A>
//     future(ready_future_from_tuple_marker, std::tuple<A...>&& data) : _promise(nullptr) {
//         _local_state.set(std::move(data));
//     }
//     future(exception_future_marker, std::exception_ptr ex) noexcept : _promise(nullptr) {
//         _local_state.set_exception(std::move(ex));
//     }
//     [[gnu::always_inline]]
//     explicit future(future_state<T...>&& state) noexcept
//             : _promise(nullptr), _local_state(std::move(state)) {
//     }
//     [[gnu::always_inline]]
//     future_state<T...>* state() noexcept {
//         return _promise ? _promise->_state : &_local_state;
//     }

//     template <typename Func>
//     void schedule(Func&& func) {
//         if (state()->available()) {
//             std::cout<<"schedule state available."<<std::endl;
//             ::schedule_normal(std::make_unique<continuation<Func, T...>>(std::move(func), std::move(*state())));
//         } else {
//             //走这一条
//             assert(_promise);
//             std::cout<<"schedule state unavailable."<<std::endl;
//             _promise->schedule(std::move(func));
//             _promise->_future = nullptr;
//             _promise = nullptr;
//         }
//     }

//     [[gnu::always_inline]]
//     future_state<T...> get_available_state() noexcept {
//         auto st = state();
//         if (_promise) {
//             _promise->_future = nullptr;
//             _promise = nullptr;
//         }
//         return std::move(*st);
//     }

//     [[gnu::noinline]]
//     future<T...> rethrow_with_nested() {
//         if (!failed()) {
//             return make_exception_future<T...>(std::current_exception());
//         } else {
//             std::nested_exception f_ex;
//             try {
//                 get();
//             } catch (...) {
//                 std::throw_with_nested(f_ex);
//             }
//         }
//         assert(0 && "we should not be here");
//     }

//     template<typename... U>
//     friend class shared_future;
// public:
//     /// \brief The data type carried by the future.
//     using value_type = std::tuple<T...>;
//     /// \brief The data type carried by the future.
//     using promise_type = promise<T...>;
//     /// \brief Moves the future into a new object.
//     [[gnu::always_inline]]
//     future(future&& x) noexcept : _promise(x._promise) {
//         if (!_promise) {
//             _local_state = std::move(x._local_state);
//         }
//         x._promise = nullptr;
//         if (_promise) {
//             _promise->_future = this;
//         }
//     }
//     future(const future&) = delete;
//     future& operator=(future&& x) noexcept {
//         if (this != &x) {
//             this->~future();
//             new (this) future(std::move(x));
//         }
//         return *this;
//     }
//     void operator=(const future&) = delete;
//     __attribute__((always_inline))
//     ~future() {
//         if (_promise) {
//             _promise->_future = nullptr;
//         }
//         if (failed()) {
//             report_failed_future(state()->get_exception());
//         }
//     }
//     std::tuple<T...> get();

//      std::exception_ptr get_exception() {
//         return get_available_state().get_exception();
//     }
//     typename future_state<T...>::get0_return_type get0() {
//         return future_state<T...>::get0(get());
//     }

//     /// \cond internal
//     void wait();
//     [[gnu::always_inline]]
//     bool available() noexcept {
//         return state()->available();
//     }
//     [[gnu::always_inline]]
//     bool failed() noexcept {
//         return state()->failed();
//     }
//     template <typename Func, typename Result = futurize_t<std::result_of_t<Func(T&&...)>>>
//     GCC6_CONCEPT( requires CanApply<Func, T...> )
//     Result
//     then(Func&& func) noexcept {
//         using futurator = futurize<std::result_of_t<Func(T&&...)>>;
//         // 如果当前 future,已经完成且不需要抢占.
//         if (available() && !need_preempt()) {
//             //调试的时候这里不会执行到,need_preempt永远为true.
//             if (failed()) {
//                 // 如果失败，传播异常
//                 return futurator::make_exception_future(get_available_state().get_exception());
//             } else {
//                 // 如果成功，执行回调函数
//                 return futurator::apply(std::forward<Func>(func), get_available_state().get_value());
//             }
//         }
//         // 如果 future 还未完成，创建新的 promise 和 future
//         typename futurator::promise_type pr; // 这行代码是什么意思?
//         auto fut = pr.get_future();
//         try {
//             std::cout<<"开始执行schedule"<<std::endl;
//             //schedule接受一个lambda函数,捕捉pr和func,参数为state
//             schedule([pr = std::move(pr), func = std::forward<Func>(func)] (auto&& state) mutable
//             {
//                 //这个地方看不懂.auto &&state和state()有什么区别？为什么state不用引用捕获？
//                 if (state.failed()) {
//                     pr.set_exception(std::move(state).get_exception());
//                 }
//                 else{
//                     // 执行这个
//                     futurator::apply(std::forward<Func>(func), std::move(state).get_value()).forward_to(std::move(pr));
//                     // futuator::apply首先执行func(value)，返回类型是T.
//                     // 返回一个 future<T>. 然后调用future的forward_to.
//                 }
//             });
//         } catch (...) {
//             abort();
//         }
//         return fut;
//     }

//     template <typename Func, typename Result = futurize_t<std::result_of_t<Func(future)>>>
//     GCC6_CONCEPT( requires CanApply<Func, future> )
//     Result
//     then_wrapped(Func&& func) noexcept {
//         using futurator = futurize<std::result_of_t<Func(future)>>;
//         if (available() && !need_preempt()) {
//             return futurator::apply(std::forward<Func>(func), future(get_available_state()));
//         }
//         typename futurator::promise_type pr;
//         auto fut = pr.get_future();
//         try {
//             schedule([pr = std::move(pr), func = std::forward<Func>(func)] (auto&& state) mutable {
//                 futurator::apply(std::forward<Func>(func), future(std::move(state))).forward_to(std::move(pr));
//             });
//         } catch (...) {
//             abort();
//         }
//         return fut;
//     }
//     void forward_to(promise<T...>&& pr) noexcept {
//         if (state()->available()) {
//             std::cout<<"state available future调用forward_to"<<std::endl;
//             state()->forward_to(pr);
            
//         } else {
//             std::cout<<"state unavailable future调用forward_to"<<std::endl;
//             _promise->_future = nullptr;
//             *_promise = std::move(pr);
//             _promise = nullptr;
//         }
//     }

//     template <typename Func>
//     GCC6_CONCEPT( requires CanApply<Func> )
//     future<T...> finally(Func&& func) noexcept {
//         return then_wrapped(finally_body<Func, is_future<std::result_of_t<Func()>>::value>(std::forward<Func>(func)));
//     }


//     template <typename Func, bool FuncReturnsFuture>
//     struct finally_body;

//     template <typename Func>
//     struct finally_body<Func, true> {
//         Func _func;

//         finally_body(Func&& func) : _func(std::forward<Func>(func))
//         { }

//         future<T...> operator()(future<T...>&& result) {
//             using futurator = futurize<std::result_of_t<Func()>>;
//             return futurator::apply(_func).then_wrapped([result = std::move(result)](auto f_res) mutable {
//                 if (!f_res.failed()) {
//                     return std::move(result);
//                 } else {
//                     try {
//                         f_res.get();
//                     } catch (...) {
//                         return result.rethrow_with_nested();
//                     }
//                     assert(0 && "we should not be here");
//                 }
//             });
//         }
//     };

//     template <typename Func>
//     struct finally_body<Func, false> {
//         Func _func;
//         finally_body(Func&& func) : _func(std::forward<Func>(func))
//         {}
//         future<T...> operator()(future<T...>&& result) {
//             try {
//                 _func();
//                 return std::move(result);
//             } catch (...) {
//                 return result.rethrow_with_nested();
//             }
//         };
//     };

//     future<> or_terminate() noexcept {
//         return then_wrapped([] (auto&& f) {
//             try {
//                 f.get();
//             } catch (...) {
//                 engine_exit(std::current_exception());
//             }
//         });
//     }

//     future<> discard_result() noexcept {
//         return then([] (T&&...) {});
//     }
//     template <typename Func>
//     future<T...> handle_exception(Func&& func) noexcept {
//         using func_ret = std::result_of_t<Func(std::exception_ptr)>;
//         return then_wrapped([func = std::forward<Func>(func)]
//                              (auto&& fut) -> future<T...> {
//             if (!fut.failed()) {
//                 return make_ready_future<T...>(fut.get());
//             } else {
//                 return futurize<func_ret>::apply(func, fut.get_exception());
//             }
//         });
//     }
//     template <typename Func>
//     future<T...> handle_exception_type(Func&& func) noexcept {
//         using trait = function_traits<Func>;
//         static_assert(trait::arity == 1, "func can take only one parameter");
//         using ex_type = typename trait::template arg<0>::type;
//         using func_ret = typename trait::return_type;
//         return then_wrapped([func = std::forward<Func>(func)]
//                              (auto&& fut) -> future<T...> {
//             try {
//                 return make_ready_future<T...>(fut.get());
//             } catch(ex_type& ex) {
//                 return futurize<func_ret>::apply(func, ex);
//             }
//         });
//     }
//     void ignore_ready_future() noexcept {
//         state()->ignore();
//     }
//     /// \cond internal
//     template <typename... U>
//     friend class promise;
//     template <typename... U, typename... A>
//     friend future<U...> make_ready_future(A&&... value);
//     template <typename... U>
//     friend future<U...> make_exception_future(std::exception_ptr ex) noexcept;
//     template <typename... U, typename Exception>
//     friend future<U...> make_exception_future(Exception&& ex) noexcept;
//     /// \endcond
// };







// template <typename... T>
// class promise {
// public:
//     enum class urgent { no, yes };
//     future<T...>* _future = nullptr;
//     future_state<T...> _local_state;
//     future_state<T...>* _state;
//     std::unique_ptr<task> _task;
//     static constexpr bool copy_noexcept = future_state<T...>::copy_noexcept;
//     /// \brief Constructs an empty \c promise.
//     ///
//     /// Creates promise with no associated future yet (see get_future()).
//     promise() noexcept : _state(&_local_state) {}

//     /// \brief Moves a \c promise object.
//     promise(promise&& x) noexcept : _future(x._future), _state(x._state), _task(std::move(x._task)) {
//         if (_state == &x._local_state) {
//             _state = &_local_state;
//             _local_state = std::move(x._local_state);
//         }
//         x._future = nullptr;
//         x._state = nullptr;
//         migrated();
//     }
//     promise(const promise&) = delete;
//     __attribute__((always_inline))
//     ~promise() noexcept {
//         abandoned();
//     }
//     promise& operator=(promise&& x) noexcept {
//         if (this != &x) {
//             this->~promise();
//             new (this) promise(std::move(x));
//         }
//         return *this;
//     }
//     void operator=(const promise&) = delete;

//     future<T...> get_future() noexcept;

//     void set_value(const std::tuple<T...>& result) noexcept(copy_noexcept) {
//         do_set_value<urgent::no>(result);
//     }

//     void set_value(std::tuple<T...>&& result) noexcept {
//         do_set_value<urgent::no>(std::move(result));
//     }

//     template <typename... A>
//     void set_value(A&&... a) noexcept {
//         assert(_state);
//         _state->set(std::forward<A>(a)...);
//         make_ready<urgent::no>();
//     }

//     void set_exception(std::exception_ptr ex) noexcept {
//         do_set_exception<urgent::no>(std::move(ex));
//     }

//     template<typename Exception>
//     void set_exception(Exception&& e) noexcept {
//         set_exception(make_exception_ptr(std::forward<Exception>(e)));
//     }
//     template<urgent Urgent>
//     void do_set_value(std::tuple<T...> result) noexcept {
//         assert(_state);
//         _state->set(std::move(result));
//         make_ready<Urgent>();
//     }

//     void set_urgent_value(const std::tuple<T...>& result) noexcept(copy_noexcept) {
//         do_set_value<urgent::yes>(result);
//     }

//     void set_urgent_value(std::tuple<T...>&& result) noexcept {
//         do_set_value<urgent::yes>(std::move(result));
//     }

//     template<urgent Urgent>
//     void do_set_exception(std::exception_ptr ex) noexcept {
//         assert(_state);
//         _state->set_exception(std::move(ex));
//         make_ready<Urgent>();
//     }

//     void set_urgent_exception(std::exception_ptr ex) noexcept {
//         do_set_exception<urgent::yes>(std::move(ex));
//     }
//     template <typename Func>
//     void schedule(Func&& func) {
//         auto tws = std::make_unique<continuation<Func, T...>>(std::move(func));
//         _state = &tws->_state;
//         _task = std::move(tws); 
//     }
//     template<urgent Urgent>
//     void make_ready() noexcept;
//     void migrated() noexcept;
//     void abandoned() noexcept;
//     template <typename... U>
//     friend class future;
//     friend class future_state<T...>;
// };

// template<>
// class promise<void> : public promise<> {};
// // template<>
// // class promise<> {
// // public:
// //     enum class urgent { no, yes };
// //     future<>* _future = nullptr;
// //     future_state<> _local_state;
// //     future_state<>* _state;
// //     std::unique_ptr<task> _task;

// // public:
// //     promise() noexcept : _state(&_local_state) {}

// //     promise(promise&& x) noexcept : _future(x._future), _state(x._state), _task(std::move(x._task)) {
// //         if (_state == &x._local_state) {
// //             _state = &_local_state;
// //             _local_state = std::move(x._local_state);
// //         }
// //         x._future = nullptr;
// //         x._state = nullptr;
// //         migrated();
// //     }

// //     promise(const promise&) = delete;
// //     ~promise() noexcept {
// //         abandoned();
// //     }

// //     promise& operator=(promise&& x) noexcept {
// //         if (this != &x) {
// //             this->~promise();
// //             new (this) promise(std::move(x));
// //         }
// //         return *this;
// //     }

// //     void operator=(const promise&) = delete;
    
// //     future<> get_future() noexcept {
// //         return future<>(this);
// //     }

// //     void set_value() noexcept {
// //         assert(_state);
// //         _state->set();
// //         make_ready<urgent::no>();
// //     }

// //     void set_urgent_value(std::tuple<>) noexcept {
// //         assert(_state);
// //         _state->set();
// //         make_ready<urgent::yes>();
// //     }
    
// //     void set_exception(std::exception_ptr ex) noexcept {
// //         assert(_state);
// //         _state->set_exception(std::move(ex));
// //         make_ready<urgent::no>();
// //     }
    
// //     void set_urgent_exception(std::exception_ptr ex) noexcept {
// //         assert(_state);
// //         _state->set_exception(std::move(ex));
// //         make_ready<urgent::yes>();
// //     }
    
// //     template <typename Func>
// //     void schedule(Func&& func) {
// //         auto tws = std::make_unique<continuation<Func>>(std::forward<Func>(func));
// //         _state = &tws->_state;
// //         _task = std::move(tws);
// //     }
    
// //     template<urgent Urgent>
// //     void make_ready() noexcept {
// //         if (_future) {
// //             if (_state == &_local_state) {
// //                 _future->_local_state = std::move(*_state);
// //                 _future->_promise = nullptr;
// //             }
// //             _future = nullptr;
// //         }
// //         if (_task) {
// //             if (Urgent == urgent::yes) {
// //                 schedule_urgent(std::move(_task));
// //             } else {
// //                 schedule_normal(std::move(_task));
// //             }
// //         }
// //     }
    
// //     void migrated() noexcept {
// //         if (_future) {
// //             _future->_promise = this;
// //         }
// //     }
    
// //     void abandoned() noexcept {
// //         if (_future) {
// //             assert(_state);
// //             _future->_local_state = std::move(*_state);
// //             _future->_promise = nullptr;
// //         }
// //     }
// // };

// template <typename... T, typename... A>
// inline
// future<T...> make_ready_future(A&&... value) {
//     return future<T...>(ready_future_marker(), std::forward<A>(value)...);
// }

// template <typename... T>
// inline
// future<T...> make_exception_future(std::exception_ptr ex) noexcept {
//     return future<T...>(exception_future_marker(), std::move(ex));
// }

// class timed_out_error : public std::exception {
// public:
//     virtual const char* what() const noexcept {
//         return "timedout";
//     }
// };

// template<typename T>
// struct dummy_expiry {
//     void operator()(T&) noexcept {};
// };
// template<typename... T>
// struct promise_expiry {
//     void operator()(promise<T...>& pr) noexcept {
//         pr.set_exception(std::make_exception_ptr(timed_out_error()));
//     };
// };

// template <typename T, typename OnExpiry = dummy_expiry<T>, typename Clock = lowres_clock>
// class expiring_fifo {
// public:
//     using clock = Clock;
//     using time_point = typename Clock::time_point;
// private:
//     struct entry {
//         std::experimental::optional<T> payload;
//         timer<Clock> tr;
//         entry(T&& payload_) : payload(std::move(payload_)) {}
//         entry(const T& payload_) : payload(payload_) {}
//         entry(T payload_, expiring_fifo& ef, time_point timeout)
//                 : payload(std::move(payload_))
//                 , tr([this, &ef] {
//                     ef._on_expiry(*payload);
//                     payload = std::experimental::nullopt;
//                     --ef._size;
//                     ef.drop_expired_front();
//                 })
//         {
//             tr.arm(timeout);
//         }
//         entry(entry&& x) = delete;
//         entry(const entry& x) = delete;
//     };

//     std::deque<entry> _list;
//     OnExpiry _on_expiry;
//     size_t _size = 0;

//     void drop_expired_front() {
//         while (!_list.empty() && !_list.front().payload) {
//             _list.pop_front();
//         }
//     }
// public:
//     expiring_fifo() = default;
//     expiring_fifo(OnExpiry on_expiry) : _on_expiry(std::move(on_expiry)) {}

//     bool empty() const {
//         return _size == 0;
//     }

//     explicit operator bool() const {
//         return !empty();
//     }

//     T& front() {
//         return *_list.front().payload;
//     }

//     const T& front() const {
//         return *_list.front().payload;
//     }

//     size_t size() const {
//         return _size;
//     }
//     void reserve(size_t size) {
//         return _list.reserve(size);
//     }
//     void push_back(const T& payload) {
//         _list.emplace_back(payload);
//         ++_size;
//     }
//     void push_back(T&& payload) {
//         _list.emplace_back(std::move(payload));
//         ++_size;
//     }
//     void push_back(T payload, time_point timeout) {
//         if (timeout < time_point::max()) {
//             _list.emplace_back(std::move(payload), *this, timeout);
//         } else {
//             _list.emplace_back(std::move(payload));
//         }
//         ++_size;
//     }
//     void pop_front() {
//         _list.pop_front();
//         --_size;
//         drop_expired_front();
//     }
// };




// class broken_semaphore : public std::exception {
// public:
//     /// Reports the exception reason.
//     virtual const char* what() const noexcept {
//         return "Semaphore broken";
//     }
// };
// class semaphore_timed_out : public std::exception {
// public:
//     /// Reports the exception reason.
//     virtual const char* what() const noexcept {
//         return "Semaphore timedout";
//     }
// };
// struct semaphore_default_exception_factory {
//     static semaphore_timed_out timeout() {
//         return semaphore_timed_out();
//     }
//     static broken_semaphore broken() {
//         return broken_semaphore();
//     }
// };


// template<typename ExceptionFactory, typename Clock = typename timer<>::clock>
// class basic_semaphore {
// public:
//     using duration = typename timer<Clock>::duration;
//     using clock = typename timer<Clock>::clock;
//     using time_point = typename timer<Clock>::time_point;
// private:
//     ssize_t _count;
//     std::exception_ptr _ex;
//     struct entry {
//         promise<> pr;
//         size_t nr;
//         entry(promise<>&& pr_, size_t nr_) : pr(std::move(pr_)), nr(nr_) {}
//     };
//     struct expiry_handler {
//         void operator()(entry& e) noexcept {
//             e.pr.set_exception(std::make_exception_ptr(ExceptionFactory::timeout()));
//         }
//     };
//     expiring_fifo<entry, expiry_handler, clock> _wait_list;
//     bool has_available_units(size_t nr) const {
//         return _count >= 0 && (static_cast<size_t>(_count) >= nr);
//     }
//     bool may_proceed(size_t nr) const {
//         return has_available_units(nr) && _wait_list.empty();
//     }
// public:
//     static constexpr size_t max_counter() {
//         return std::numeric_limits<decltype(_count)>::max();
//     }
//     basic_semaphore(size_t count) : _count(count) {}
//     future<> wait(size_t nr = 1) {
//         return wait(time_point::max(), nr);
//     }

//     future<> wait(duration timeout, size_t nr = 1) {
//         return wait(Clock::now() + timeout, nr);
//     }
//     future<> wait(time_point timeout, size_t nr = 1) {
//         if (may_proceed(nr)) {
//             _count -= nr;
//             return make_ready_future<>();
//         }
//         if (_ex) {
//             return make_exception_future(_ex);
//         }
//         promise<> pr;
//         auto fut = pr.get_future();
//         _wait_list.push_back(entry(std::move(pr), nr), timeout);
//         return fut;
//     }
//      void signal(size_t nr = 1) {
//         if (_ex) {
//             return;
//         }
//         _count += nr;
//         while (!_wait_list.empty() && has_available_units(_wait_list.front().nr)) {
//             auto& x = _wait_list.front();
//             _count -= x.nr;
//             x.pr.set_value();
//             _wait_list.pop_front();
//         }
//     }

//     void consume(size_t nr = 1) {
//         if (_ex) {
//             return;
//         }
//         _count -= nr;
//     }
//     bool try_wait(size_t nr = 1) {
//         if (may_proceed(nr)) {
//             _count -= nr;
//             return true;
//         } else {
//             return false;
//         }
//     }
//     size_t current() const { return std::max(_count, ssize_t(0)); }
//     ssize_t available_units() const { return _count; }
//     size_t waiters() const { return _wait_list.size(); }
//     void broken() { broken(std::make_exception_ptr(ExceptionFactory::broken())); }
//     template <typename Exception>
//     void broken(const Exception& ex) {
//         broken(std::make_exception_ptr(ex));
//     }
//     void broken(std::exception_ptr ex);
//     void ensure_space_for_waiters(size_t n) {
//         _wait_list.reserve(n);
//     }
// };

// template<typename ExceptionFactory = semaphore_default_exception_factory, typename Clock = typename timer<>::clock>
// class semaphore_units {
//     basic_semaphore<ExceptionFactory, Clock>& _sem;
//     size_t _n;
// public:
//     semaphore_units(basic_semaphore<ExceptionFactory, Clock>& sem, size_t n) noexcept : _sem(sem), _n(n) {}
//     semaphore_units(semaphore_units&& o) noexcept : _sem(o._sem), _n(o._n) {
//         o._n = 0;
//     }
//     semaphore_units& operator=(semaphore_units&& o) noexcept {
//         if (this != &o) {
//             this->~semaphore_units();
//             new (this) semaphore_units(std::move(o));
//         }
//         return *this;
//     }
//     semaphore_units(const semaphore_units&) = delete;
//     ~semaphore_units() noexcept {
//         if (_n) {
//             _sem.signal(_n);
//         }
//     }
//     /// Releases ownership of the units. The semaphore will not be signalled.
//     ///
//     /// \return the number of units held
//     size_t release() {
//         return std::exchange(_n, 0);
//     }
// };

// using semaphore = basic_semaphore<semaphore_default_exception_factory>;

// class broken_condition_variable : public std::exception {
// public:
//     /// Reports the exception reason.
//     virtual const char* what() const noexcept {
//         return "Condition variable is broken";
//     }
// };

// class condition_variable_timed_out : public std::exception {
// public:
//     /// Reports the exception reason.
//     virtual const char* what() const noexcept {
//         return "Condition variable timed out";
//     }
// };


// class condition_variable {
//     using duration = semaphore::duration;
//     using clock = semaphore::clock;
//     using time_point = semaphore::time_point;
//     struct condition_variable_exception_factory {
//         static condition_variable_timed_out timeout() {
//             return condition_variable_timed_out();
//         }
//         static broken_condition_variable broken() {
//             return broken_condition_variable();
//         }
//     };
//     basic_semaphore<condition_variable_exception_factory> _sem;
// public:
//     condition_variable() : _sem(0) {}
//     future<> wait() {
//         return _sem.wait();
//     }

//     future<> wait(time_point timeout) {
//         return _sem.wait(timeout);
//     }
//     future<> wait(duration timeout) {
//         return _sem.wait(timeout);
//     }
//     template<typename Pred>
//     future<> wait(Pred&& pred) {
//         return do_until(std::forward<Pred>(pred), [this] {
//             return wait();
//         });
//     }
//     template<typename Pred>
//     future<> wait(time_point timeout, Pred&& pred) {
//         return do_until(std::forward<Pred>(pred), [this, timeout] () mutable {
//             return wait(timeout);
//         });
//     }
//     template<typename Pred>
//     future<> wait(duration timeout, Pred&& pred) {
//         return wait(clock::now() + timeout, std::forward<Pred>(pred));
//     }
//     void signal() {
//         if (_sem.waiters()) {
//             _sem.signal();
//         }
//     }
//     void broadcast() {
//         _sem.signal(_sem.waiters());
//     }
//     void broken() {
//         _sem.broken();
//     }
// };



// /*----------------------------metrics---------------------------------------*/

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
//  *
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


// class priority_class {
//     struct request {
//         promise<> pr;
//         unsigned weight;
//     };
//     friend class fair_queue;
//     uint32_t _shares = 0;
//     float _accumulated = 0;
//     std::deque<request> _queue;
//     bool _queued = false;
//     friend struct shared_ptr_no_esft<priority_class>;
//     explicit priority_class(uint32_t shares) : _shares(shares) {}
// };
// using priority_class_ptr = lw_shared_ptr<priority_class>;


// class fair_queue {
//     friend priority_class;
//     struct class_compare {
//         bool operator() (const priority_class_ptr& lhs, const priority_class_ptr& rhs) const {
//             return lhs->_accumulated > rhs->_accumulated;
//         }
//     };
//     semaphore _sem;
//     unsigned _capacity;
//     using clock_type = std::chrono::steady_clock::time_point;
//     clock_type _base;
//     std::chrono::microseconds _tau;
//     using prioq = std::priority_queue<priority_class_ptr, std::vector<priority_class_ptr>, class_compare>;
//     prioq _handles;
//     std::unordered_set<priority_class_ptr> _all_classes;
//     void push_priority_class(priority_class_ptr pc) {
//         if (!pc->_queued) {
//             _handles.push(pc);
//             pc->_queued = true;
//         }
//     }
//     priority_class_ptr pop_priority_class() {
//         assert(!_handles.empty());
//         auto h = _handles.top();
//         _handles.pop();
//         assert(h->_queued);
//         h->_queued = false;
//         return h;
//     }
//     void execute_one() {
//         _sem.wait().then([this] {
//             priority_class_ptr h;
//             do {
//                 h = pop_priority_class();
//             } while (h->_queue.empty());

//             auto req = std::move(h->_queue.front());
//             h->_queue.pop_front();
//             req.pr.set_value();
//             auto delta = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - _base);
//             auto req_cost  = float(req.weight) / h->_shares;
//             auto cost  = expf(1.0f/_tau.count() * delta.count()) * req_cost;
//             float next_accumulated = h->_accumulated + cost;
//             while (std::isinf(next_accumulated)) {
//                 normalize_stats();
//                 // If we have renormalized, our time base will have changed. This should happen very infrequently
//                 delta = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - _base);
//                 cost  = expf(1.0f/_tau.count() * delta.count()) * req_cost;
//                 next_accumulated = h->_accumulated + cost;
//             }
//             h->_accumulated = next_accumulated;
//             if (!h->_queue.empty()) {
//                 push_priority_class(h);
//             }
//             return make_ready_future<>();
//         });
//     }
//     float normalize_factor() const {
//         return std::numeric_limits<float>::min();
//     }
//     void normalize_stats() {
//         auto time_delta = std::log(normalize_factor()) * _tau;
//         // time_delta is negative; and this may advance _base into the future
//         _base -= std::chrono::duration_cast<clock_type::duration>(time_delta);
//         for (auto& pc: _all_classes) {
//             pc->_accumulated *= normalize_factor();
//         }
//     }
// public:
//     explicit fair_queue(unsigned capacity, std::chrono::microseconds tau = std::chrono::milliseconds(100))
//                                            : _sem(capacity)
//                                            , _capacity(capacity)
//                                            , _base(std::chrono::steady_clock::now())
//                                            , _tau(tau) {
//     }
//     priority_class_ptr register_priority_class(uint32_t shares) {
//         priority_class_ptr pclass = make_lw_shared<priority_class>(shares);
//         _all_classes.insert(pclass);
//         return pclass;
//     }
//     void unregister_priority_class(priority_class_ptr pclass) {
//         assert(pclass->_queue.empty());
//         _all_classes.erase(pclass);
//     }
//     size_t waiters() const {
//         return _sem.waiters();
//     }

//     template <typename Func>
//     futurize_t<std::result_of_t<Func()>> queue(priority_class_ptr pc, unsigned weight, Func func) {
//         // We need to return a future in this function on which the caller can wait.
//         // Since we don't know which queue we will use to execute the next request - if ours or
//         // someone else's, we need a separate promise at this point.
//         promise<> pr;
//         auto fut = pr.get_future();

//         push_priority_class(pc);
//         pc->_queue.push_back(priority_class::request{std::move(pr), weight});
//         try {
//             execute_one();
//         } catch (...) {
//             pc->_queue.pop_back();
//             throw;
//         }
//         return fut.then([func = std::move(func)] {
//             return func();
//         }).finally([this] {
//             _sem.signal();
//         });
//     }

//     /// Updates the current shares of this priority class
//     ///
//     /// \param new_shares the new number of shares for this priority class
//     static void update_shares(priority_class_ptr pc, uint32_t new_shares) {
//         pc->_shares = new_shares;
//     }
// };

// class io_queue;
// class io_priority_class {
//     unsigned val;
//     friend io_queue;
// public:
//     unsigned id() const {
//         return val;
//     }
// };

// class smp;
// class io_queue {
// private:
//     shard_id _coordinator;
//     size_t _capacity;
//     std::vector<shard_id> _io_topology;
//     struct priority_class_data {
//         priority_class_ptr ptr;
//         size_t bytes;
//         uint64_t ops;
//         uint32_t nr_queued;
//         std::chrono::duration<double> queue_time;
//         metrics::metric_groups _metric_groups;
//         priority_class_data(std::string name, priority_class_ptr ptr, shard_id owner);
//     };
//     std::unordered_map<unsigned, lw_shared_ptr<priority_class_data>> _priority_classes;
//     fair_queue _fq;
//     static constexpr unsigned _max_classes = 1024;
//     static std::array<std::atomic<uint32_t>, _max_classes> _registered_shares;
//     static std::array<std::string, _max_classes> _registered_names;
//     static io_priority_class register_one_priority_class(std::string name, uint32_t shares);
//     priority_class_data& find_or_create_class(const io_priority_class& pc, shard_id owner);
//     static void fill_shares_array();
//     friend smp;
// public:
//     io_queue(shard_id coordinator, size_t capacity, std::vector<shard_id> topology);
//     ~io_queue();
//     template <typename Func>
//     static future<io_event>
//     queue_request(shard_id coordinator, const io_priority_class& pc, size_t len, Func do_io);
//     size_t capacity() const {
//         return _capacity;
//     }
//     size_t queued_requests() const {
//         return _fq.waiters();
//     }

//     shard_id coordinator() const {
//         return _coordinator;
//     }
//     shard_id coordinator_of_shard(shard_id shard) const {
//         return _io_topology[shard];
//     }
//     friend class reactor;
// };



// /*-------------------------------------------------reactor类定义----------------------------------------------------------------*/
// #include "../resource/resource.hh"
// #include <boost/thread/barrier.hpp>
// #include <boost/range/irange.hpp>
// __thread bool g_need_preempt;

// struct reactor {
// /*---------构造函数和析构函数----------------*/
//     reactor();
//     reactor(unsigned int id);
//     ~reactor();
//     reactor(const reactor&) = delete;
//     reactor& operator=(const reactor&) = delete;
//     unsigned _id = 0;
// /*----------定时器相关------------------------*/
//     std::unique_ptr<lowres_clock> _lowres_clock;
//     lowres_clock::time_point _lowres_next_timeout;
//     timer_t _steady_clock_timer = {};
//     timer_set<timer<steady_clock_type>> _timers;
//     typename timer_set<timer<steady_clock_type>>::timer_list_t _expired_timers;
//     timer_set<timer<lowres_clock>> _lowres_timers;
//     typename timer_set<timer<lowres_clock>>::timer_list_t _expired_lowres_timers;
//     timer_set<timer<manual_clock>> _manual_timers;
//     typename timer_set<timer<manual_clock>>::timer_list_t _expired_manual_timers;
//     using steady_timer = timer<steady_clock_type>;
//     using lowres_timer = timer<lowres_clock>;
//     using manual_timer = timer<manual_clock>;
// /*---------------------------------------------*/
//     void add_timer(steady_timer* tmr);
//     bool queue_timer(steady_timer* tmr);
//     void del_timer(steady_timer* tmr);
//     void add_timer(lowres_timer* tmr);
//     bool queue_timer(lowres_timer* tmr);
//     void del_timer(lowres_timer* tmr);
//     void add_timer(manual_timer* tmr);
//     bool queue_timer(manual_timer* tmr);
//     void del_timer(manual_timer* tmr);
//     void enable_timer(steady_clock_type::time_point when);
//     bool do_expire_lowres_timers();
//     template <typename T, typename E, typename EnableFunc>
//     void complete_timers(T&, E&, EnableFunc&& enable_fn);
//     bool do_check_lowres_timers() const;
//     void expire_manual_timers();
// /*---------------信号处理相关------------------------*/
//     signals _signals;
// /*----------任务相关-------------------*/
//     bool _stopping = false;
//     bool _stopped = false;
//     int _return = 0;
//     condition_variable _stop_requested;
//     std::atomic<bool> _sleeping alignas(64);
//     std::deque<std::unique_ptr<task>> _pending_tasks;
//     void run_tasks(std::deque<std::unique_ptr<task>>& tasks);
//     std::vector<std::function<future<> ()>> _exit_funcs;//为什么不用引用?
//     void add_task(std::unique_ptr<task>&& t) { _pending_tasks.push_back(std::move(t)); }
//     void add_urgent_task(std::unique_ptr<task>&& t) { _pending_tasks.push_front(std::move(t)); }
//     void force_poll() {
//         g_need_preempt = true;
//     }
//     void at_exit(std::function<future<> ()> func);
//     void exit(int ret);
//     void stop();
//     future<> run_exit_tasks();

//     /*----------------全局--------------*/
//     int run();
//     /*----------配置相关----------------*/
//     static boost::program_options::options_description get_options_description();
//     void configure(boost::program_options::variables_map config);

//    /*----------------------资源分配相关-----------------------*/
//     shard_id _io_coordinator;
//     io_queue* _io_queue;
//     std::unique_ptr<io_queue> my_io_queue = {};
//     pthread_t _thread_id alignas(64) = pthread_self();
//     shard_id cpu_id() const { return _id; }
//     void wakeup() { pthread_kill(_thread_id, alarm_signal());}
// };

// struct reactor_deleter {
//     void operator()(reactor* p) {
//         p->~reactor();
//         free(p);
//     }
// };
// void schedule_normal(std::unique_ptr<task> t) {
//     std::cout<<"调用schedule normal"<<std::endl;
//     engine().add_task(std::move(t));
// }
// void schedule_urgent(std::unique_ptr<task> t) {
//     std::cout<<"调用schedule urgent"<<std::endl;
//     engine().add_urgent_task(std::move(t));
// }

// /*
// 报错:
// */
//     // using lf_queue_base = boost::lockfree::spsc_queue<
//     // work_item*,
//     // boost::lockfree::capacity<queue_length>,
//     // boost::lockfree::allocator<std::allocator<work_item*>>>;
//     /*
//     from include/../include/future/future_all5.hh:2483,
//     from test/new_tests/future_promise_test.cc:1:
//     /usr/include/boost/lockfree/detail/parameter.hpp: In instantiation of ‘struct boost::lockfree::detail::extract_allocator<boost::parameter::aux::arg_list<boost::lockfree::capacity<128>, boost::parameter::aux::empty_arg_list>, smp_message_queue::work_item*>’:
//     /usr/include/boost/lockfree/spsc_queue.hpp:650:48:   required from ‘struct boost::lockfree::detail::make_ringbuffer<smp_message_queue::work_item*, boost::lockfree::capacity<128> >’
//     /usr/include/boost/lockfree/spsc_queue.hpp:687:7:   required from ‘class boost::lockfree::spsc_queue<smp_message_queue::work_item*, boost::lockfree::capacity<128> >’
//     include/../include/future/future_all5.hh:2496:40:   required from here
//     /usr/include/boost/lockfree/detail/parameter.hpp:57:63: 
//     error: no class template named ‘rebind’ in ‘boost::lockfree::detail::extract_allocator<boost::parameter::aux::arg_list<boost::lockfree::capacity<128>, boost::parameter::aux::empty_arg_list>, smp_message_queue::work_item*>::allocator_arg’ {aka ‘class std::allocator<smp_message_queue::work_item*>’}
//    typedef typename allocator_arg::template rebind<T>::other type;
//       |                                                              
//     */


// template <typename T, size_t Capacity>
// class spsc_queue {
// private:
//     // 使用缓存行对齐来避免伪共享
//     alignas(64) std::atomic<size_t> _head{0};
//     alignas(64) std::atomic<size_t> _tail{0};
//     // 环形缓冲区
//     T _buffer[Capacity];
//     // 帮助函数，计算下一个索引位置
//     size_t next_index(size_t current) const {
//         return (current + 1) % Capacity;
//     }
// public:
//     spsc_queue() = default;
//     // 禁止复制和移动
//     spsc_queue(const spsc_queue&) = delete;
//     spsc_queue& operator=(const spsc_queue&) = delete;
//     spsc_queue(spsc_queue&&) = delete;
//     spsc_queue& operator=(spsc_queue&&) = delete;
//     // 检查队列是否为空
//     bool empty() const {
//         return _head.load(std::memory_order_relaxed) == _tail.load(std::memory_order_relaxed);
//     }
//     // 检查队列是否已满
//     bool full() const {
//         size_t next_tail = next_index(_tail.load(std::memory_order_relaxed));
//         return next_tail == _head.load(std::memory_order_relaxed);
//     }
//     // 入队操作 - 生产者调用
//     bool push(T item) {
//         size_t current_tail = _tail.load(std::memory_order_relaxed);
//         size_t next_tail = next_index(current_tail);
//         if (next_tail == _head.load(std::memory_order_acquire)) {
//             // 队列已满
//             return false;
//         }   
//         _buffer[current_tail] = std::move(item);
//         _tail.store(next_tail, std::memory_order_release);
//         return true;
//     }
//     // 入队多个元素 - 返回成功入队的元素结束迭代器
//     template <typename Iterator>
//     Iterator push(Iterator begin, Iterator end) {
//         Iterator current = begin;
//         while (current != end) {
//             if (!push(*current)) {
//                 break;
//             }
//             ++current;
//         }
//         return current;
//     }
//     // 出队操作 - 消费者调用
//     bool pop(T& item) {
//         size_t current_head = _head.load(std::memory_order_relaxed);
//         if (current_head == _tail.load(std::memory_order_acquire)) {
//             // 队列为空
//             return false;
//         }   
//         item = std::move(_buffer[current_head]);
//         _head.store(next_index(current_head), std::memory_order_release);
//         return true;
//     }
//     // 批量出队 - 返回成功出队的元素数量
//     template <size_t ArraySize>
//     size_t pop(T (&items)[ArraySize]) {
//         size_t popped = 0;
//         while (popped < ArraySize && pop(items[popped])) {
//             ++popped;
//         }
//         return popped;
//     }
// };

// class smp_message_queue {
//     static constexpr size_t queue_length = 128;
//     static constexpr size_t batch_size = 16;
//     static constexpr size_t prefetch_cnt = 2;
//     struct work_item;
//     struct lf_queue_remote {
//         reactor* remote;
//     };
//     // 使用自定义的无锁队列替换boost::lockfree::spsc_queue
//     using lf_queue_base = spsc_queue<work_item*, queue_length>;
//     // 使用继承来控制布局顺序
//     struct lf_queue : lf_queue_remote, lf_queue_base {
//         lf_queue(reactor* remote) : lf_queue_remote{remote} {}
//         void maybe_wakeup();
//     };
//     lf_queue _pending;
//     lf_queue _completed;
//     struct alignas(64) {
//         size_t _sent = 0;
//         size_t _compl = 0;
//         size_t _last_snt_batch = 0;
//         size_t _last_cmpl_batch = 0;
//         size_t _current_queue_length = 0;
//     };
//     // 在两个带有统计信息的结构体之间保持这个字段
//     // 这确保它们之间至少有一个缓存行
//     // 以便硬件预取器不会意外地预取另一个CPU使用的缓存行
//     metrics::metric_groups _metrics;
//     struct alignas(64) {
//         size_t _received = 0;
//         size_t _last_rcv_batch = 0;
//     };
//     struct work_item {
//         virtual ~work_item() {}
//         virtual future<> process() = 0;
//         virtual void complete() = 0;
//     };
//     template <typename Func>
//     struct async_work_item : work_item {
//         Func _func;
//         using futurator = futurize<std::result_of_t<Func()>>;
//         using future_type = typename futurator::type;
//         using value_type = typename future_type::value_type;
//         std::experimental::optional<value_type> _result;
//         std::exception_ptr _ex; // if !_result
//         typename futurator::promise_type _promise; // 在本地端使用
//         async_work_item(Func&& func) : _func(std::move(func)) {}
//         virtual future<> process() override {
//             try {
//                 return futurator::apply(this->_func).then_wrapped([this] (auto&& f) {
//                     try {
//                         _result = f.get();
//                     } catch (...) {
//                         _ex = std::current_exception();
//                     }
//                 });
//             } catch (...) {
//                 _ex = std::current_exception();
//                 return make_ready_future();
//             }
//         }
//         virtual void complete() override {
//             if (_result) {
//                 _promise.set_value(std::move(*_result));
//             } else {
//                 // FIXME: _ex was allocated on another cpu
//                 _promise.set_exception(std::move(_ex));
//             }
//         }
//         future_type get_future() { return _promise.get_future(); }
//     };
//     union tx_side {
//         tx_side() {}
//         ~tx_side() {}
//         void init() { new (&a) aa; }
//         struct aa {
//             std::deque<work_item*> pending_fifo;
//         } a;
//     } _tx;
//     std::vector<work_item*> _completed_fifo;
// public:
//     smp_message_queue(reactor* from, reactor* to);
//     template <typename Func>
//     futurize_t<std::result_of_t<Func()>> submit(Func&& func) {
//         auto wi = std::make_unique<async_work_item<Func>>(std::forward<Func>(func));
//         auto fut = wi->get_future();
//         submit_item(std::move(wi));
//         return fut;
//     }
//     void start(unsigned cpuid);
//     template<size_t PrefetchCnt, typename Func>
//     size_t process_queue(lf_queue& q, Func process);
//     size_t process_incoming();
//     size_t process_completions();
//     void stop();
    
// private:
//     void work();
//     void submit_item(std::unique_ptr<work_item> wi);
//     void respond(work_item* wi);
//     void move_pending();
//     void flush_request_batch();
//     void flush_response_batch();
//     bool has_unflushed_responses() const;
//     bool pure_poll_rx() const;
//     bool pure_poll_tx() const;

//     friend class smp;
// };







// timespec to_timespec(steady_clock_type::time_point t) {
//     using ns = std::chrono::nanoseconds;
//     auto n = std::chrono::duration_cast<ns>(t.time_since_epoch()).count();
//     return { n / 1'000'000'000, n % 1'000'000'000 };
// }

// reactor& engine(){
//     static reactor r;
//     return r;
// }


// bool queue_timer(timer<steady_clock_type>* tmr) {
//     return engine().queue_timer(tmr);
// }

// void add_timer(timer<steady_clock_type>* tmr) {
//     engine().add_timer(tmr);
// }

// void add_timer(timer<lowres_clock>* tmr) {
//     engine().add_timer(tmr);
// }

// void add_timer(timer<manual_clock>* tmr) {
//      engine().add_timer(tmr);
// }

// bool queue_timer(timer<manual_clock>* tmr) {
//     return engine().queue_timer(tmr);
// }

// bool queue_timer(timer<lowres_clock>* tmr) {
//     return engine().queue_timer(tmr);
// }


// void manual_clock::advance(manual_clock::duration d) {
//     _now.fetch_add(d.count());
//     // engine().schedule_urgent(make_task(&manual_clock::expire_timers));
//     //smp::invoke_on_all(&manual_clock::expire_timers);
//     return;
// }

// void del_timer(timer<lowres_clock>* tmr) {
//     engine().del_timer(tmr);
// }

// void del_timer(timer<steady_clock_type>* tmr) {
//     engine().del_timer(tmr);
// }
// void del_timer(timer<manual_clock>* tmr) {
//     engine().del_timer(tmr);
// }

// // Initialize static member
// std::atomic<manual_clock::rep> manual_clock::_now{0};
// template<int Signal, void(*Func)()>
// void install_oneshot_signal_handler() {
//     static bool handled = false;
//     static util::spinlock lock;
//     struct sigaction sa;
//     sa.sa_sigaction = [](int sig, siginfo_t *info, void *p) {
//         std::lock_guard<util::spinlock> g(lock);
//         if (!handled) {
//             handled = true;
//             Func();
//             signal(sig, SIG_DFL);
//         }
//     };
//     sigfillset(&sa.sa_mask);
//     sa.sa_flags = SA_SIGINFO | SA_RESTART;
//     if (Signal == SIGSEGV) {
//         sa.sa_flags |= SA_ONSTACK;
//     }
//     auto r = ::sigaction(Signal, &sa, nullptr);
//     // throw_system_error_on(false);//这是我改的.
// }

// static void sigsegv_action() noexcept {
//     std::cout<<"Segmentation fault";
// }

// static void sigabrt_action() noexcept {
//     std::cout<<"Aborting";
// }






// template<typename Clock>
// struct with_clock {};
// template <typename... T>
// struct future_option_traits;
// template <typename Clock, typename... T>
// struct future_option_traits<with_clock<Clock>, T...> {
//     using clock_type = Clock;
//     template<template <typename...> class Class>
//     struct parametrize {
//         using type = Class<T...>;
//     };
// };

// template <typename... T>
// struct future_option_traits {
//     using clock_type = lowres_clock;
//     template<template <typename...> class Class>
//     struct parametrize {
//         using type = Class<T...>;
//     };
// };


// __thread reactor* local_engine;




// inline
// void pin_this_thread(unsigned cpu_id) {
//     cpu_set_t cs;
//     CPU_ZERO(&cs);
//     CPU_SET(cpu_id, &cs);
//     auto r = pthread_setaffinity_np(pthread_self(), sizeof(cs), &cs);
//     assert(r == 0);
// }

// namespace bi = boost::intrusive;
// template <typename... T>
// class promise;

// template <typename... T>
// class future;
// class thread;
// class thread_attributes;
// class thread_scheduling_group;
// struct jmp_buf_link;
// template <class... T>
// class promise;
// template <class... T> 
// class future;
// template<> 
// class promise<void>;

// template <typename... T, typename... A>
// future<T...> make_ready_future(A&&... value);

// template <typename... T>
// future<T...> make_exception_future(std::exception_ptr value) noexcept;

// template<typename... T>
// class shared_future {
//     template <typename... U> friend class shared_promise;
//     using options = future_option_traits<T...>;
// public:
//     using clock = typename options::clock_type;
//     using time_point = typename clock::time_point;
//     using future_type = typename future_option_traits<T...>::template parametrize<future>::type;
//     using promise_type = typename future_option_traits<T...>::template parametrize<promise>::type;
//     using value_tuple_type = typename future_option_traits<T...>::template parametrize<std::tuple>::type;
// private:
//     using future_state_type = typename future_option_traits<T...>::template parametrize<future_state>::type;
//     using promise_expiry = typename future_option_traits<T...>::template parametrize<promise_expiry>::type;

//     class shared_state {
//         future_state_type _future_state;
//         expiring_fifo<promise_type, promise_expiry, clock> _peers;
//     public:
//         void resolve(future_type&& f) noexcept {
//             _future_state = f.get_available_state();
//             if (_future_state.failed()) {
//                 while (_peers) {
//                     _peers.front().set_exception(_future_state.get_exception());
//                     _peers.pop_front();
//                 }
//             } else {
//                 while (_peers) {
//                     auto& p = _peers.front();
//                     try {
//                         p.set_value(_future_state.get_value());
//                     } catch (...) {
//                         p.set_exception(std::current_exception());
//                     }
//                     _peers.pop_front();
//                 }
//             }
//         }

//         future_type get_future(time_point timeout = time_point::max()) {
//             if (!_future_state.available()) {
//                 promise_type p;
//                 auto f = p.get_future();
//                 _peers.push_back(std::move(p), timeout);
//                 return f;
//             } else if (_future_state.failed()) {
//                 return future_type(exception_future_marker(), _future_state.get_exception());
//             } else {
//                 try {
//                     return future_type(ready_future_marker(), _future_state.get_value());
//                 } catch (...) {
//                     return future_type(exception_future_marker(), std::current_exception());
//                 }
//             }
//         }
//     };
//     lw_shared_ptr<shared_state> _state;
// public:
//     shared_future(future_type&& f)
//         : _state(make_lw_shared<shared_state>())
//     {
//         f.then_wrapped([s = _state] (future_type&& f) mutable {
//             s->resolve(std::move(f));
//         });
//     }

//     shared_future() = default;
//     shared_future(const shared_future&) = default;
//     shared_future& operator=(const shared_future&) = default;
//     shared_future(shared_future&&) = default;
//     shared_future& operator=(shared_future&&) = default;
//     future_type get_future(time_point timeout = time_point::max()) const {
//         return _state->get_future(timeout);
//     }
//     operator future_type() const {
//         return get_future();
//     }
//     bool valid() const {
//         return bool(_state);
//     }
// };
// template <typename... T>
// class shared_promise {
// public:
//     using shared_future_type = shared_future<T...>;
//     using future_type = typename shared_future_type::future_type;
//     using promise_type = typename shared_future_type::promise_type;
//     using clock = typename shared_future_type::clock;
//     using time_point = typename shared_future_type::time_point;
//     using value_tuple_type = typename shared_future_type::value_tuple_type;
//     using future_state_type = typename shared_future_type::future_state_type;
// private:
//     promise_type _promise;
//     shared_future_type _shared_future;
//     static constexpr bool copy_noexcept = future_state_type::copy_noexcept;
// public:
//     shared_promise(const shared_promise&) = delete;
//     shared_promise(shared_promise&&) = default;
//     shared_promise& operator=(shared_promise&&) = default;
//     shared_promise() : _promise(), _shared_future(_promise.get_future()) {
//     }
//     /// \brief Gets new future associated with this promise.
//     /// If the promise is not resolved before timeout the returned future will resolve with \ref timed_out_error.
//     /// This instance doesn't have to be kept alive until the returned future resolves.
//     future_type get_shared_future(time_point timeout = time_point::max()) {
//         return _shared_future.get_future(timeout);
//     }
//     /// \brief Sets the shared_promise's value (as tuple; by copying), same as normal promise
//     void set_value(const value_tuple_type& result) noexcept(copy_noexcept) {
//         _promise.set_value(result);
//     }
//     /// \brief Sets the shared_promise's value (as tuple; by moving), same as normal promise
//     void set_value(value_tuple_type&& result) noexcept {
//         _promise.set_value(std::move(result));
//     }
//     /// \brief Sets the shared_promise's value (variadic), same as normal promise
//     template <typename... A>
//     void set_value(A&&... a) noexcept {
//         _promise.set_value(std::forward<A>(a)...);
//     }
//     /// \brief Marks the shared_promise as failed, same as normal promise
//     void set_exception(std::exception_ptr ex) noexcept {
//         _promise.set_exception(std::move(ex));
//     }
//     /// \brief Marks the shared_promise as failed, same as normal promise
//     template<typename Exception>
//     void set_exception(Exception&& e) noexcept {
//         set_exception(make_exception_ptr(std::forward<Exception>(e)));
//     }
// };



// class smp {
// public:
//     static std::vector<posix_thread> _threads;
//     static std::vector<std::function<void ()>> _thread_loops; // for dpdk
//     static std::experimental::optional<boost::barrier> _all_event_loops_done;
//     static std::vector<reactor*> _reactors;
//     static smp_message_queue** _qs; // 重新声明这个成员
//     static std::thread::id _tmain;
//     static bool _using_dpdk;
    
//     template <typename Func>
//     using returns_future = is_future<std::result_of_t<Func()>>;
    
//     template <typename Func>
//     using returns_void = std::is_same<std::result_of_t<Func()>, void>;
    
//     static boost::program_options::options_description get_options_description();
//     static void configure(boost::program_options::variables_map vm);
//     static void cleanup();
//     static void cleanup_cpu();
//     static void arrive_at_event_loop_end();
//     static void join_all();
//     static bool main_thread() { return std::this_thread::get_id() == _tmain; }
    
//     template <typename Func>
//     static futurize_t<std::result_of_t<Func()>> submit_to(unsigned t, Func&& func);
//     static bool poll_queues();
//     static bool pure_poll_queues();
//     static boost::integer_range<unsigned> all_cpus() {
//         return boost::irange(0u, count);
//     }
    
//     template<typename Func>
//     static future<> invoke_on_all(Func&& func);
    
//     static void start_all_queues();
//     static void pin(unsigned cpu_id);
//     static void allocate_reactor(unsigned id);
//     static void create_thread(std::function<void ()> thread_loop);
// public:
//     static unsigned count;
// };


// thread_local std::unique_ptr<reactor, reactor_deleter> reactor_holder;
// std::vector<posix_thread> smp::_threads;
// std::vector<std::function<void ()>> smp::_thread_loops;
// std::experimental::optional<boost::barrier> smp::_all_event_loops_done;
// std::vector<reactor*> smp::_reactors;
// smp_message_queue** smp::_qs;
// std::thread::id smp::_tmain;


// /// \brief Creates a \ref future in an available, failed state.
// ///
// /// Creates a \ref future object that is already resolved in a failed
// /// state.  This no I/O needs to be performed to perform a computation
// /// (for example, because the connection is closed and we cannot read
// /// from it).
// template <typename... T, typename Exception>
// inline
// future<T...> make_exception_future(Exception&& ex) noexcept {
//     return make_exception_future<T...>(std::make_exception_ptr(std::forward<Exception>(ex)));
// }

// /// @}

// /// \cond internal

// template<typename T>
// template<typename Func, typename... FuncArgs>
// typename futurize<T>::type futurize<T>::apply(Func&& func, std::tuple<FuncArgs...>&& args) noexcept {
//     try {
//         return convert(std::apply(std::forward<Func>(func), std::move(args)));
//         //执行这个函数,并返回结果.然后对结果执行convert。
//     } catch (...) {
//         return make_exception_future(std::current_exception());
//     }
// }

// template<typename T>
// template<typename Func, typename... FuncArgs>
// typename futurize<T>::type futurize<T>::apply(Func&& func, FuncArgs&&... args) noexcept {
//     try {
//         return convert(func(std::forward<FuncArgs>(args)...));
//     } catch (...) {
//         return make_exception_future(std::current_exception());
//     }
// }

// template<typename Func, typename... FuncArgs>
// inline
// std::enable_if_t<!is_future<std::result_of_t<Func(FuncArgs&&...)>>::value, future<>>
// do_void_futurize_apply(Func&& func, FuncArgs&&... args) noexcept {
//     try {
//         func(std::forward<FuncArgs>(args)...);
//         return make_ready_future<>();
//     } catch (...) {
//         return make_exception_future(std::current_exception());
//     }
// }

// template<typename Func, typename... FuncArgs>
// inline
// std::enable_if_t<is_future<std::result_of_t<Func(FuncArgs&&...)>>::value, future<>>
// do_void_futurize_apply(Func&& func, FuncArgs&&... args) noexcept {
//     try {
//         return func(std::forward<FuncArgs>(args)...);
//     } catch (...) {
//         return make_exception_future(std::current_exception());
//     }
// }

// template<typename Func, typename... FuncArgs>
// inline
// std::enable_if_t<!is_future<std::result_of_t<Func(FuncArgs&&...)>>::value, future<>>
// do_void_futurize_apply_tuple(Func&& func, std::tuple<FuncArgs...>&& args) noexcept {
//     try {
//         std::apply(std::forward<Func>(func), std::move(args));
//         return make_ready_future<>();
//     } catch (...) {
//         return make_exception_future(std::current_exception());
//     }
// }

// template<typename Func, typename... FuncArgs>
// inline
// std::enable_if_t<is_future<std::result_of_t<Func(FuncArgs&&...)>>::value, future<>>
// do_void_futurize_apply_tuple(Func&& func, std::tuple<FuncArgs...>&& args) noexcept {
//     try {
//         return std::apply(std::forward<Func>(func), std::move(args));
//     } catch (...) {
//         return make_exception_future(std::current_exception());
//     }
// }

// template<typename Func, typename... FuncArgs>
// typename futurize<void>::type futurize<void>::apply(Func&& func, std::tuple<FuncArgs...>&& args) noexcept {
//     return do_void_futurize_apply_tuple(std::forward<Func>(func), std::move(args));
// }

// template<typename Func, typename... FuncArgs>
// typename futurize<void>::type futurize<void>::apply(Func&& func, FuncArgs&&... args) noexcept {
//     return do_void_futurize_apply(std::forward<Func>(func), std::forward<FuncArgs>(args)...);
// }

// template<typename... Args>
// template<typename Func, typename... FuncArgs>
// typename futurize<future<Args...>>::type futurize<future<Args...>>::apply(Func&& func, std::tuple<FuncArgs...>&& args) noexcept {
//     try {
//         return std::apply(std::forward<Func>(func), std::move(args));
//     } catch (...) {
//         return make_exception_future(std::current_exception());
//     }
// }

// template<typename... Args>
// template<typename Func, typename... FuncArgs>
// typename futurize<future<Args...>>::type futurize<future<Args...>>::apply(Func&& func, FuncArgs&&... args) noexcept {
//     try {
//         return func(std::forward<FuncArgs>(args)...);
//     } catch (...) {
//         return make_exception_future(std::current_exception());
//     }
// }

// template<typename Func, typename... Args>
// auto futurize_apply(Func&& func, Args&&... args) {
//     using futurator = futurize<std::result_of_t<Func(Args&&...)>>;
//     return futurator::apply(std::forward<Func>(func), std::forward<Args>(args)...);
// }
// /// Executes a callable in a seastar thread.
// /// Runs a block of code in a threaded context,
// /// which allows it to block (using \ref future::get()).  The
// /// result of the callable is returned as a future.
// /// \param func a callable to be executed in a thread
// /// \param args a parameter pack to be forwarded to \c func.
// /// \return whatever \c func returns, as a future.
// /// Clock used for scheduling threads
// using thread_clock = std::chrono::steady_clock;
// struct thread_attributes {
//         thread_scheduling_group* scheduling_group = nullptr;
// };

// class thread_scheduling_group {
//     public:
//         std::chrono::nanoseconds _period;
//         std::chrono::nanoseconds _quota;
//         std::chrono::time_point<thread_clock> _this_period_ends = {};
//         std::chrono::time_point<thread_clock> _this_run_start = {};
//         std::chrono::nanoseconds _this_period_remain = {};
//         /// \brief Constructs a \c thread_scheduling_group object
//         ///
//         /// \param period a duration representing the period
//         /// \param usage which fraction of the \c period to assign for the scheduling group. Expected between 0 and 1.
//         thread_scheduling_group(std::chrono::nanoseconds period, float usage);
//         /// \brief changes the current maximum usage per period
//         ///
//         /// \param new_usage The new fraction of the \c period (Expected between 0 and 1) during which to run
//         void update_usage(float new_usage) {
//             _quota = std::chrono::duration_cast<std::chrono::nanoseconds>(new_usage * _period);
//         }
//         void account_start();
//         void account_stop();
//         std::chrono::steady_clock::time_point* next_scheduling_point() const;
// };

// class thread_context;
// struct jmp_buf_link {
//     jmp_buf jmpbuf;
//     jmp_buf_link* link;
//     thread_context* thread;
//     bool has_yield_at = false;
//     std::chrono::time_point<thread_clock> yield_at_value;
//     void initial_switch_in(ucontext_t* initial_context, const void* stack_bottom, size_t stack_size);
//     void switch_in();
//     void switch_out();
//     void initial_switch_in_completed();
//     void final_switch_out();
//     std::chrono::time_point<thread_clock>* get_yield_at() {
//         return has_yield_at ? &yield_at_value : nullptr;
//     }
//     void set_yield_at(const std::chrono::time_point<thread_clock>& value) {
//         yield_at_value = value;
//         has_yield_at = true;
//     }    
//     void clear_yield_at() {
//         has_yield_at = false;
//     }
// };

// thread_local jmp_buf_link g_unthreaded_context; //在jmp_buf_link init_switch_in的时候用来初始化g_current_context
// thread_local jmp_buf_link* g_current_context;

// struct thread_context {
//     struct stack_deleter {
//         void operator()(char *ptr) const noexcept;
//     };
//     using stack_holder = std::unique_ptr<char[], stack_deleter>;
//     thread_attributes _attr;
//     static constexpr size_t _stack_size = 128*1024;
//     stack_holder _stack{make_stack()};
//     std::function<void ()> _func;
//     jmp_buf_link _context;
//     promise<> _done;
//     bool _joined = false;
//     timer<> _sched_timer{[this] { reschedule(); }};
//     promise<>* _sched_promise_ptr = nullptr;
//     promise<> _sched_promise_value;
//     std::list<thread_context*>::iterator _preempted_it;
//     std::list<thread_context*>::iterator _all_it;
//     // Replace boost::intrusive::list with std::list
//     static thread_local std::list<thread_context*> _preempted_threads;
//     static thread_local std::list<thread_context*> _all_threads;
//     static void s_main(unsigned int lo, unsigned int hi);
//     void setup();
//     void main();
//     static stack_holder make_stack();
//     thread_context(thread_attributes attr, std::function<void ()> func);
//     ~thread_context();
//     void switch_in();
//     void switch_out();
//     bool should_yield() const;
//     void reschedule();
//     void yield();
//     promise<>* get_sched_promise() {
//         return _sched_promise_ptr;
//     }
//     void set_sched_promise() {
//         _sched_promise_ptr = &_sched_promise_value;
//     }
//     void clear_sched_promise() {
//         _sched_promise_ptr = nullptr;
//     }
// };
// namespace thread_impl {
//     inline thread_context* get() {
//         return g_current_context->thread;
//     }
//     inline bool should_yield() {
//         if (need_preempt()) {
//             return true;
//         } else if (g_current_context->get_yield_at()) {
//             return std::chrono::steady_clock::now() >= *(g_current_context->get_yield_at());
//         } else {
//             return false;
//         }
//     }
//     void yield(){
//         g_current_context->thread->yield();
//     }
//     void switch_in(thread_context* to){
//         to->switch_in();
//     }
//     void switch_out(thread_context* from){
//         from->switch_out();
//     }
//     void init(){
//         g_unthreaded_context.link = nullptr;
//         g_unthreaded_context.thread = nullptr;
//         g_current_context = &g_unthreaded_context;
//     }
// }


// class thread {
//     std::unique_ptr<thread_context> _context;
//     static thread_local thread* _current;
// public:
//     /// \brief Constructs a \c thread object that does not represent a thread
//     /// of execution.
//     thread() = default;

//     /// \brief Constructs a \c thread object that represents a thread of execution
//     ///
//     /// \param func Callable object to execute in thread.  The callable is
//     ///             called immediately.
//     template <typename Func>
//     thread(Func func);

//     /// \brief Constructs a \c thread object that represents a thread of execution
//     /// \param attr Attributes describing the new thread.
//     /// \param func Callable object to execute in thread.  The callable is
//     ///             called immediately.
//     template <typename Func>
//     thread(thread_attributes attr, Func func);

//     /// \brief Moves a thread object.
//     thread(thread&& x) noexcept = default;

//     /// \brief Move-assigns a thread object.
//     thread& operator=(thread&& x) noexcept = default;

//     /// \brief Destroys a \c thread object.
//     /// The thread must not represent a running thread of execution (see join()).
//     ~thread();

//     future<> join();

//     /// \brief Voluntarily defer execution of current thread.
//     /// Gives other threads/fibers a chance to run on current CPU.
//     /// The current thread will resume execution promptly.
//     static void yield();

//     /// \brief Checks whether this thread ought to call yield() now
//     /// Useful where we cannot call yield() immediately because we
//     /// Need to take some cleanup action first.
//     static bool should_yield();

//     static bool running_in_thread() {
//         return thread_impl::get() != nullptr;
//     }

// private:
//     static bool try_run_one_yielded_thread();
// };



// class gate {
//     size_t _count = 0;
//     promise<>* _stopped_ptr = nullptr;
//     promise<> _stopped_value;
// public:
//     void enter() {
//         if (_stopped_ptr) {
//             throw 1;
//         }
//         ++_count;
//     }
//     void leave() {
//         --_count;
//         if (!_count && _stopped_ptr) {
//             _stopped_ptr->set_value();
//         }
//     }
//     void check() {
//         if (_stopped_ptr) {
//             throw 1;
//         }
//     }
//     future<> close() {
//         assert(!_stopped_ptr && "gate::close() cannot be called more than once");
//         _stopped_ptr = &_stopped_value;
//         if (!_count) {
//             _stopped_ptr->set_value();
//         }
//         return _stopped_ptr->get_future();
//     }
//     size_t get_count() const {
//         return _count;
//     }
// };
// template <typename Func>
// inline
// auto
// with_gate(gate& g, Func&& func) {
//     g.enter();
//     return func().finally([&g] { g.leave(); });
// }


// // future<> later() {
// //     promise<> p;
// //     auto f = p.get_future();
// //     engine().force_poll(); //把need_preempted改为true(这句是没有意义的)
// //     ::schedule_normal(make_task([p = std::move(p)]() mutable {
// //         p.set_value(); // 这段代码把一个p.set_value封装为一个task加到调度器中.
// //     }));
// //     return f;
// // }


// template<typename ExceptionFactory, typename Clock = typename timer<>::clock>
// future<semaphore_units<ExceptionFactory, Clock>>
// get_units(basic_semaphore<ExceptionFactory, Clock>& sem, size_t units) {
//     return sem.wait(units).then([&sem, units] {
//         return semaphore_units<ExceptionFactory, Clock>{ sem, units };
//     });
// }

// template<typename ExceptionFactory, typename Clock = typename timer<>::clock>
// future<semaphore_units<ExceptionFactory, Clock>>
// get_units(basic_semaphore<ExceptionFactory, Clock>& sem, size_t units, typename basic_semaphore<ExceptionFactory, Clock>::time_point timeout) {
//     return sem.wait(timeout, units).then([&sem, units] {
//         return semaphore_units<ExceptionFactory, Clock>{ sem, units };
//     });
// }
// template<typename ExceptionFactory, typename Clock = typename timer<>::clock>
// semaphore_units<ExceptionFactory, Clock>
// consume_units(basic_semaphore<ExceptionFactory, Clock>& sem, size_t units) {
//     sem.consume(units);
//     return semaphore_units<ExceptionFactory, Clock>{ sem, units };
// }

// template <typename ExceptionFactory, typename Func, typename Clock = typename timer<>::clock>
// inline
// futurize_t<std::result_of_t<Func()>>
// with_semaphore(basic_semaphore<ExceptionFactory, Clock>& sem, size_t units, Func&& func) {
//     return get_units(sem, units).then([func = std::forward<Func>(func)] (auto units) mutable {
//         return futurize_apply(std::forward<Func>(func)).finally([units = std::move(units)] {});
//     });
// }



// template <typename Func, typename... Args>
// inline futurize_t<std::result_of_t<std::decay_t<Func>(std::decay_t<Args>...)>>
// async(Func&& func, Args&&... args) {
//     return async(thread_attributes{}, std::forward<Func>(func), std::forward<Args>(args)...);
// }

// template <typename Func, typename... Args>
// inline
// futurize_t<std::result_of_t<std::decay_t<Func>(std::decay_t<Args>...)>>
// async(thread_attributes attr, Func&& func, Args&&... args) {
//     using return_type = std::result_of_t<std::decay_t<Func>(std::decay_t<Args>...)>;
//     struct work {
//         thread_attributes attr;
//         Func func;
//         std::tuple<Args...> args;
//         promise<return_type> pr;
//         thread th;
//     };
//     return do_with(work{std::move(attr), std::forward<Func>(func), std::forward_as_tuple(std::forward<Args>(args)...)}, [] (work& w) mutable {
//         auto ret = w.pr.get_future();
//         w.th = thread(std::move(w.attr), [&w] {
//             futurize<return_type>::apply(std::move(w.func), std::move(w.args)).forward_to(std::move(w.pr));
//         });
//         return w.th.join().then([ret = std::move(ret)] () mutable {
//             return std::move(ret);
//         });
//     });
// }


// void report_failed_future(std::exception_ptr eptr) {
//    std::cout<<"####"<<std::endl;
// }


// // Define the static members
// thread_local std::list<thread_context*> thread_context::_preempted_threads;
// thread_local std::list<thread_context*> thread_context::_all_threads;








// #include "../util/shared_ptr.hh"
// // #include "do_with.hh"
// // #include "timer.hh"
// #include "../util/bool_class.hh"
// #include <tuple>
// #include <iterator>
// #include <vector>
// #include <experimental/optional>
// #include "util/tuple_utils.hh"
// extern __thread size_t task_quota;
// struct parallel_for_each_state {
//     // use optional<> to avoid out-of-line constructor
//     std::experimental::optional<std::exception_ptr> ex;
//     size_t waiting = 0;
//     promise<> pr;
//     void complete() {
//         if (--waiting == 0) {
//             if (ex) {
//                 pr.set_exception(std::move(*ex));
//             } else {
//                 pr.set_value();
//             }
//         }
//     }
// };

// //这里？
// template <typename Iterator, typename Func>
// GCC6_CONCEPT(requires requires (Func f, Iterator i) { { f(*i++) } -> std::same_as<future<>>; })
// inline
// future<>
// parallel_for_each(Iterator begin, Iterator end, Func&& func) {
//     if (begin == end) {
//         return make_ready_future<>();
//     }
//     return do_with(parallel_for_each_state(), [&] (parallel_for_each_state& state) -> future<> {
//         // increase ref count to ensure all functions run
//         ++state.waiting;
//         while (begin != end) {
//             ++state.waiting;
//             try {
//                 func(*begin++).then_wrapped([&] (future<> f) {
//                     if (f.failed()) {
//                         // We can only store one exception.  For more, use when_all().
//                         if (!state.ex) {
//                             state.ex = f.get_exception();
//                         } else {
//                             f.ignore_ready_future();
//                         }
//                     }
//                     state.complete();
//                 });
//             } catch (...) {
//                 if (!state.ex) {
//                     state.ex = std::move(std::current_exception());
//                 }
//                 state.complete();
//             }
//         }
//         // match increment on top
//         state.complete();
//         return state.pr.get_future();
//     });
// }


// template <typename Range, typename Func>
// GCC6_CONCEPT(requires requires (Func f, Range r) { { f(*r.begin()) } -> std::same_as<future<>>; })
// inline
// future<>
// parallel_for_each(Range&& range, Func&& func) {
//     return parallel_for_each(std::begin(range), std::end(range),
//             std::forward<Func>(func));
// }


// template<typename AsyncAction, typename StopCondition>
// static inline
// void do_until_continued(StopCondition&& stop_cond, AsyncAction&& action, promise<> p) {
//     while (!stop_cond()) {
//         try {
//             auto&& f = action();
//             if (!f.available() || need_preempt()) {
//                 f.then_wrapped([action = std::forward<AsyncAction>(action),
//                                 stop_cond = std::forward<StopCondition>(stop_cond), 
//                                 p = std::move(p)]  // 修复：移动捕获p
//                                 (std::result_of_t<AsyncAction()> fut) mutable {
//                     if (!fut.failed()) {
//                         do_until_continued(std::forward<StopCondition>(stop_cond), 
//                                           std::forward<AsyncAction>(action), 
//                                           std::move(p));  // 修复：移动p
//                     } else {
//                         p.set_exception(fut.get_exception());  // 此时p已经被捕获
//                     }
//                 });
//                 return;
//             }
//             if (f.failed()) {
//                 f.forward_to(std::move(p));
//                 return;
//             }
//         } catch (...) {
//             p.set_exception(std::current_exception());
//             return;
//         }
//     }
//     p.set_value();
// }


// struct stop_iteration_tag { };
// using stop_iteration = bool_class<stop_iteration_tag>;


// template<typename AsyncAction>
// GCC6_CONCEPT( requires ApplyReturns<AsyncAction, stop_iteration> || ApplyReturns<AsyncAction, future<stop_iteration>> )
// static inline
// future<> repeat(AsyncAction&& action) {
//     using futurator = futurize<std::result_of_t<AsyncAction()>>;
//     static_assert(std::is_same<future<stop_iteration>, typename futurator::type>::value, "bad AsyncAction signature");

//     try {
//         do {
//             auto f = futurator::apply(action);

//             if (!f.available()) {
//                 return f.then([action = std::forward<AsyncAction>(action)] (stop_iteration stop) mutable {
//                     if (stop == stop_iteration::yes) {
//                         return make_ready_future<>();
//                     } else {
//                         return repeat(std::forward<AsyncAction>(action));
//                     }
//                 });
//             }

//             if (f.get0() == stop_iteration::yes) {
//                 return make_ready_future<>();
//             }
//         } while (!need_preempt());

//         promise<> p;
//         auto f = p.get_future();
//         schedule(make_task([action = std::forward<AsyncAction>(action), p = std::move(p)]() mutable {
//             repeat(std::forward<AsyncAction>(action)).forward_to(std::move(p));
//         }));
//         return f;
//     } catch (...) {
//         return make_exception_future(std::current_exception());
//     }
// }


// template <typename T>
// struct repeat_until_value_type_helper;


// /// Type helper for repeat_until_value()
// template <typename T>
// struct repeat_until_value_type_helper<future<std::experimental::optional<T>>> {
//     using value_type = T;
//     using optional_type = std::experimental::optional<T>;
//     using future_type = future<value_type>;
//     using future_optional_type = future<optional_type>;
// };

// template <typename AsyncAction>
// using repeat_until_value_return_type
//         = typename repeat_until_value_type_helper<std::result_of_t<AsyncAction()>>::future_type;

// template<typename AsyncAction>
// GCC6_CONCEPT( requires requires (AsyncAction aa) {
//     requires is_future<decltype(aa())>::value;
//     bool(aa().get0());
//     aa().get0().value();
// } )
// repeat_until_value_return_type<AsyncAction>
// repeat_until_value(AsyncAction&& action) {
//     using type_helper = repeat_until_value_type_helper<std::result_of_t<AsyncAction()>>;
//     // the "T" in the documentation
//     using value_type = typename type_helper::value_type;
//     using optional_type = typename type_helper::optional_type;
//     using futurator = futurize<typename type_helper::future_optional_type>;
//     do {
//         auto f = futurator::apply(action);

//         if (!f.available()) {
//             return f.then([action = std::forward<AsyncAction>(action)] (auto&& optional) mutable {
//                 if (optional) {
//                     return make_ready_future<value_type>(std::move(optional.value()));
//                 } else {
//                     return repeat_until_value(std::forward<AsyncAction>(action));
//                 }
//             });
//         }

//         if (f.failed()) {
//             return make_exception_future<value_type>(f.get_exception());
//         }

//         optional_type&& optional = std::move(f).get0();
//         if (optional) {
//             return make_ready_future<value_type>(std::move(optional.value()));
//         }
//     } while (!need_preempt());

//     try {
//         promise<value_type> p;
//         auto f = p.get_future();
//         schedule(make_task([action = std::forward<AsyncAction>(action), p = std::move(p)] () mutable {
//             repeat_until_value(std::forward<AsyncAction>(action)).forward_to(std::move(p));
//         }));
//         return f;
//     } catch (...) {
//         return make_exception_future<value_type>(std::current_exception());
//     }
// }

// template<typename AsyncAction, typename StopCondition>
// GCC6_CONCEPT( requires ApplyReturns<StopCondition, bool> && ApplyReturns<AsyncAction, future<>> )
// static inline
// future<> do_until(StopCondition&& stop_cond, AsyncAction&& action) {
//     promise<> p;
//     auto f = p.get_future();
//     do_until_continued(std::forward<StopCondition>(stop_cond),
//         std::forward<AsyncAction>(action), std::move(p));
//     return f;
// }

// template<typename AsyncAction>
// GCC6_CONCEPT( requires ApplyReturns<AsyncAction, future<>> )
// static inline
// future<> keep_doing(AsyncAction&& action) {
//     return repeat([action = std::forward<AsyncAction>(action)] () mutable {
//         return action().then([] {
//             return stop_iteration::no;
//         });
//     });
// }


// template<typename Iterator, typename AsyncAction>
// GCC6_CONCEPT( requires requires (Iterator i, AsyncAction aa) { { aa(*i) } -> std::same_as<future<>>; } )
// static inline
// future<> do_for_each(Iterator begin, Iterator end, AsyncAction&& action) {
//     if (begin == end) {
//         return make_ready_future<>();
//     }
//     while (true) {
//         auto f = action(*begin++);
//         if (begin == end) {
//             return f;
//         }
//         if (!f.available() || need_preempt()) {
//             return std::move(f).then([action = std::forward<AsyncAction>(action),
//                     begin = std::move(begin), end = std::move(end)] () mutable {
//                 return do_for_each(std::move(begin), std::move(end), std::forward<AsyncAction>(action));
//             });
//         }
//         if (f.failed()) {
//             return std::move(f);
//         }
//     }
// }

// template<typename Container, typename AsyncAction>
// GCC6_CONCEPT( requires requires (Container c, AsyncAction aa) { { aa(*c.begin()) } -> std::same_as<future<>>; } )
// static inline
// future<> do_for_each(Container& c, AsyncAction&& action) {
//     return do_for_each(std::begin(c), std::end(c), std::forward<AsyncAction>(action));
// }

// namespace internal {

// template<typename... Futures>
// struct identity_futures_tuple {
//     using future_type = future<std::tuple<Futures...>>;
//     using promise_type = typename future_type::promise_type;

//     static void set_promise(promise_type& p, std::tuple<Futures...> futures) {
//         p.set_value(std::move(futures));
//     }
// };

// template<typename ResolvedTupleTransform, typename... Futures>
// class when_all_state : public enable_lw_shared_from_this<when_all_state<ResolvedTupleTransform, Futures...>> {
//     using type = std::tuple<Futures...>;
//     type tuple;
// public:
//     typename ResolvedTupleTransform::promise_type p;
//     when_all_state(Futures&&... t) : tuple(std::make_tuple(std::move(t)...)) {}
//     ~when_all_state() {
//         ResolvedTupleTransform::set_promise(p, std::move(tuple));
//     }
// private:
//     template<size_t Idx>
//     int wait() {
//         auto& f = std::get<Idx>(tuple);
//         static_assert(is_future<std::remove_reference_t<decltype(f)>>::value, "when_all parameter must be a future");
//         if (!f.available()) {
//             f = f.then_wrapped([s = this->shared_from_this()] (auto&& f) {
//                 return std::move(f);
//             });
//         }
//         return 0;
//     }
// public:
//     template <size_t... Idx>
//     typename ResolvedTupleTransform::future_type wait_all(std::index_sequence<Idx...>) {
//         [] (...) {} (this->template wait<Idx>()...);
//         return p.get_future();
//     }
// };
// }

// // GCC6_CONCEPT(
// // /// \cond internal
// // namespace impl {
// // // Want: folds
// // template <typename T>
// // struct is_tuple_of_futures : std::false_type {
// // };
// // template <>
// // struct is_tuple_of_futures<std::tuple<>> : std::true_type {
// // };
// // template <typename... T, typename... Rest>
// // struct is_tuple_of_futures<std::tuple<future<T...>, Rest...>> : is_tuple_of_futures<std::tuple<Rest...>> {
// // };
// // }

// // template <typename... Futs>
// // concept bool AllAreFutures = impl::is_tuple_of_futures<std::tuple<Futs...>>::value;
// // )


// GCC6_CONCEPT(
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
// concept AllAreFutures = impl::is_tuple_of_futures<std::tuple<Futs...>>::value;



// // template <typename Func, typename... T>
// // concept ApplyReturnsAnyFuture = requires (Func f, T... args) {
// //     requires is_future<decltype(f(std::forward<T>(args)...))>::value;
// // };
// )


// template <typename... Futs>
// GCC6_CONCEPT( requires AllAreFutures<Futs...> )
// inline
// future<std::tuple<Futs...>>
// when_all(Futs&&... futs) {
//     namespace si = internal;
//     using state = si::when_all_state<si::identity_futures_tuple<Futs...>, Futs...>;
//     auto s = make_lw_shared<state>(std::forward<Futs>(futs)...);
//     return s->wait_all(std::make_index_sequence<sizeof...(Futs)>());
// }

// /// \cond internal
// namespace internal {

// template <typename Iterator, typename IteratorCategory>
// inline
// size_t
// when_all_estimate_vector_capacity(Iterator begin, Iterator end, IteratorCategory category) {
//     // For InputIterators we can't estimate needed capacity
//     return 0;
// }

// template <typename Iterator>
// inline
// size_t
// when_all_estimate_vector_capacity(Iterator begin, Iterator end, std::forward_iterator_tag category) {
//     // May be linear time below random_access_iterator_tag, but still better than reallocation
//     return std::distance(begin, end);
// }

// template<typename Future>
// struct identity_futures_vector {
//     using future_type = future<std::vector<Future>>;
//     static future_type run(std::vector<Future> futures) {
//         return make_ready_future<std::vector<Future>>(std::move(futures));
//     }
// };

// // Internal function for when_all().
// template <typename ResolvedVectorTransform, typename Future>
// inline
// typename ResolvedVectorTransform::future_type
// complete_when_all(std::vector<Future>&& futures, typename std::vector<Future>::iterator pos) {
//     // If any futures are already ready, skip them.
//     while (pos != futures.end() && pos->available()) {
//         ++pos;
//     }
//     // Done?
//     if (pos == futures.end()) {
//         return ResolvedVectorTransform::run(std::move(futures));
//     }
//     // Wait for unready future, store, and continue.
//     return pos->then_wrapped([futures = std::move(futures), pos] (auto fut) mutable {
//         *pos++ = std::move(fut);
//         return complete_when_all<ResolvedVectorTransform>(std::move(futures), pos);
//     });
// }

// template<typename ResolvedVectorTransform, typename FutureIterator>
// inline auto
// do_when_all(FutureIterator begin, FutureIterator end) {
//     using itraits = std::iterator_traits<FutureIterator>;
//     std::vector<typename itraits::value_type> ret;
//     ret.reserve(when_all_estimate_vector_capacity(begin, end, typename itraits::iterator_category()));
//     // Important to invoke the *begin here, in case it's a function iterator,
//     // so we launch all computation in parallel.
//     std::move(begin, end, std::back_inserter(ret));
//     return complete_when_all<ResolvedVectorTransform>(std::move(ret), ret.begin());
// }

// }


// template <typename FutureIterator>
// GCC6_CONCEPT( requires requires (FutureIterator i) { { *i++ }; requires is_future<std::remove_reference_t<decltype(*i)>>::value; } )





// inline
// future<std::vector<typename std::iterator_traits<FutureIterator>::value_type>>
// when_all(FutureIterator begin, FutureIterator end) {
//     namespace si = internal;
//     using itraits = std::iterator_traits<FutureIterator>;
//     using result_transform = si::identity_futures_vector<typename itraits::value_type>;
//     return si::do_when_all<result_transform>(std::move(begin), std::move(end));
// }

// template <typename T, bool IsFuture>
// struct reducer_with_get_traits;

// template <typename T>
// struct reducer_with_get_traits<T, false> {
//     using result_type = decltype(std::declval<T>().get());
//     using future_type = future<result_type>;
//     static future_type maybe_call_get(future<> f, lw_shared_ptr<T> r) {
//         return f.then([r = std::move(r)] () mutable {
//             return make_ready_future<result_type>(std::move(*r).get());
//         });
//     }
// };

// template <typename T>
// struct reducer_with_get_traits<T, true> {
//     using future_type = decltype(std::declval<T>().get());
//     static future_type maybe_call_get(future<> f, lw_shared_ptr<T> r) {
//         return f.then([r = std::move(r)] () mutable {
//             return r->get();
//         }).then_wrapped([r] (future_type f) {
//             return f;
//         });
//     }
// };

// template <typename T, typename V = void>
// struct reducer_traits {
//     using future_type = future<>;
//     static future_type maybe_call_get(future<> f, lw_shared_ptr<T> r) {
//         return f.then([r = std::move(r)] {});
//     }
// };

// template <typename T>
// struct reducer_traits<T, decltype(std::declval<T>().get(), void())> : public reducer_with_get_traits<T, is_future<std::result_of_t<decltype(&T::get)(T)>>::value> {};


// template <typename Iterator, typename Mapper, typename Reducer>
// inline
// auto
// map_reduce(Iterator begin, Iterator end, Mapper&& mapper, Reducer&& r)
//     -> typename reducer_traits<Reducer>::future_type
// {
//     auto r_ptr = make_lw_shared(std::forward<Reducer>(r));
//     future<> ret = make_ready_future<>();
//     using futurator = futurize<decltype(mapper(*begin))>;
//     while (begin != end) {
//         ret = futurator::apply(mapper, *begin++).then_wrapped([ret = std::move(ret), r_ptr] (auto f) mutable {
//             return ret.then_wrapped([f = std::move(f), r_ptr] (auto rf) mutable {
//                 if (rf.failed()) {
//                     f.ignore_ready_future();
//                     return std::move(rf);
//                 } else {
//                     return futurize<void>::apply(*r_ptr, std::move(f.get()));
//                 }
//             });
//         });
//     }
//     return reducer_traits<Reducer>::maybe_call_get(std::move(ret), r_ptr);
// }


// template <typename Iterator, typename Mapper, typename Initial, typename Reduce>
// GCC6_CONCEPT( requires requires (Iterator i, Mapper mapper, Initial initial, Reduce reduce) {
//     *i++;
//     { i != i } -> std::same_as<bool>;//为什么?
//     mapper(*i);
//     requires is_future<decltype(mapper(*i))>::value;
//     { reduce(std::move(initial), mapper(*i).get0()) } -> std::same_as<Initial>;
// } )
// inline
// future<Initial>
// map_reduce(Iterator begin, Iterator end, Mapper&& mapper, Initial initial, Reduce reduce) {
//     struct state {
//         Initial result;
//         Reduce reduce;
//     };
//     auto s = make_lw_shared(state{std::move(initial), std::move(reduce)});
//     future<> ret = make_ready_future<>();
//     using futurator = futurize<decltype(mapper(*begin))>;
//     while (begin != end) {
//         ret = futurator::apply(mapper, *begin++).then_wrapped([s = s.get(), ret = std::move(ret)] (auto f) mutable {
//             try {
//                 s->result = s->reduce(std::move(s->result), std::move(f.get0()));
//                 return std::move(ret);
//             } catch (...) {
//                 return std::move(ret).then_wrapped([ex = std::current_exception()] (auto f) {
//                     f.ignore_ready_future();
//                     return make_exception_future<>(ex);
//                 });
//             }
//         });
//     }
//     return ret.then([s] {
//         return make_ready_future<Initial>(std::move(s->result));
//     });
// }

// template <typename Range, typename Mapper, typename Initial, typename Reduce>
// GCC6_CONCEPT( requires requires (Range range, Mapper mapper, Initial initial, Reduce reduce) {
//      std::begin(range);
//      std::end(range);
//      mapper(*std::begin(range));
//      requires is_future<std::remove_reference_t<decltype(mapper(*std::begin(range)))>>::value;
//     { reduce(std::move(initial), mapper(*std::begin(range)).get0()) } -> std::same_as<Initial>;
// } )
// inline
// future<Initial>
// map_reduce(Range&& range, Mapper&& mapper, Initial initial, Reduce reduce) {
//     return map_reduce(std::begin(range), std::end(range), std::forward<Mapper>(mapper),
//             std::move(initial), std::move(reduce));
// }

// template <typename Result, typename Addend = Result>
// class adder {
// private:
//     Result _result;
// public:
//     future<> operator()(const Addend& value) {
//         _result += value;
//         return make_ready_future<>();
//     }
//     Result get() && {
//         return std::move(_result);
//     }
// };

// static inline future<> now() {
//     return make_ready_future<>();
// }

// future<> later(){
//     promise<> p;
//     auto f = p.get_future();
//     engine().force_poll(); //把need_preempted改为true(这句是没有意义的)
//     ::schedule_normal(make_task([p = std::move(p)]() mutable {
//         p.set_value(); // 这段代码把一个p.set_value封装为一个task加到调度器中.
//     }));
//     return f;
// }



// struct default_timeout_exception_factory {
//     static auto timeout() {
//         return timed_out_error();
//     }
// };

// template<typename ExceptionFactory = default_timeout_exception_factory, typename Clock, typename Duration, typename... T>
// future<T...> with_timeout(std::chrono::time_point<Clock, Duration> timeout, future<T...> f) {
//     if (f.available()) {
//         return f;
//     }
//     auto pr = std::make_unique<promise<T...>>();
//     auto result = pr->get_future();
//     timer<Clock> timer([&pr = *pr] {
//         pr.set_exception(std::make_exception_ptr(ExceptionFactory::timeout()));
//     });
//     timer.arm(timeout);
//     f.then_wrapped([pr = std::move(pr), timer = std::move(timer)] (auto&& f) mutable {
//         if (timer.cancel()) {
//             f.forward_to(std::move(*pr));
//         } else {
//             f.ignore_ready_future();
//         }
//     });
//     return result;
// }

// namespace internal {
// template<typename Future>
// struct future_has_value {
//     enum {
//         value = !std::is_same<std::decay_t<Future>, future<>>::value
//     };
// };

// template<typename Tuple>
// struct tuple_to_future;
// template<typename... Elements>
// struct tuple_to_future<std::tuple<Elements...>> {
//     using type = future<Elements...>;
//     using promise_type = promise<Elements...>;
//     static auto make_ready(std::tuple<Elements...> t) {
//         auto create_future = [] (auto&&... args) {
//             return make_ready_future<Elements...>(std::move(args)...);
//         };
//         return apply(create_future, std::move(t));
//     }
//     static auto make_failed(std::exception_ptr excp) {
//         return make_exception_future<Elements...>(std::move(excp));
//     }
// };

// template<typename... Futures>
// class extract_values_from_futures_tuple {
//     static auto transform(std::tuple<Futures...> futures) {
//         auto prepare_result = [] (auto futures) {
//             auto fs = tuple_filter_by_type<internal::future_has_value>(std::move(futures));
//             return tuple_map(std::move(fs), [] (auto&& e) {
//                 return internal::untuple(e.get());
//             });
//         };
//         using tuple_futurizer = internal::tuple_to_future<decltype(prepare_result(std::move(futures)))>;
//         std::exception_ptr excp;
//         tuple_for_each(futures, [&excp] (auto& f) {
//             if (!excp) {
//                 if (f.failed()) {
//                     excp = f.get_exception();
//                 }
//             } else {
//                 f.ignore_ready_future();
//             }
//         });
//         if (excp) {
//             return tuple_futurizer::make_failed(std::move(excp));
//         }
//         return tuple_futurizer::make_ready(prepare_result(std::move(futures)));
//     }
// public:
//     using future_type = decltype(transform(std::declval<std::tuple<Futures...>>()));
//     using promise_type = typename future_type::promise_type;
//     static void set_promise(promise_type& p, std::tuple<Futures...> tuple) {
//         transform(std::move(tuple)).forward_to(std::move(p));
//     }
// };

// template<typename Future>
// struct extract_values_from_futures_vector {
//     using value_type = decltype(untuple(std::declval<typename Future::value_type>()));
//     using future_type = future<std::vector<value_type>>;
//     static future_type run(std::vector<Future> futures) {
//         std::vector<value_type> values;
//         values.reserve(futures.size());

//         std::exception_ptr excp;
//         for (auto&& f : futures) {
//             if (!excp) {
//                 if (f.failed()) {
//                     excp = f.get_exception();
//                 } else {
//                     values.emplace_back(untuple(f.get()));
//                 }
//             } else {
//                 f.ignore_ready_future();
//             }
//         }
//         if (excp) {
//             return make_exception_future<std::vector<value_type>>(std::move(excp));
//         }
//         return make_ready_future<std::vector<value_type>>(std::move(values));
//     }
// };

// template<>
// struct extract_values_from_futures_vector<future<>> {
//     using future_type = future<>;

//     static future_type run(std::vector<future<>> futures) {
//         std::exception_ptr excp;
//         for (auto&& f : futures) {
//             if (!excp) {
//                 if (f.failed()) {
//                     excp = f.get_exception();
//                 }
//             } else {
//                 f.ignore_ready_future();
//             }
//         }
//         if (excp) {
//             return make_exception_future<>(std::move(excp));
//         }
//         return make_ready_future<>();
//     }
// };

// }
// template<typename... Futures>
// GCC6_CONCEPT( requires AllAreFutures<Futures...> )
// inline auto when_all_succeed(Futures&&... futures) {
//     using state = internal::when_all_state<internal::extract_values_from_futures_tuple<Futures...>, Futures...>;
//     auto s = make_lw_shared<state>(std::forward<Futures>(futures)...);
//     return s->wait_all(std::make_index_sequence<sizeof...(Futures)>());
// }

// template <typename FutureIterator, typename = typename std::iterator_traits<FutureIterator>::value_type>
// GCC6_CONCEPT( requires requires (FutureIterator i) {
//     *i++;
//     {i!= i} ->std::same_as<bool>;
//      requires is_future<std::remove_reference_t<decltype(*i)>>::value;
// })

// inline auto when_all_succeed(FutureIterator begin, FutureIterator end) {
//     using itraits = std::iterator_traits<FutureIterator>;
//     using result_transform = internal::extract_values_from_futures_vector<typename itraits::value_type>;
//     return internal::do_when_all<result_transform>(std::move(begin), std::move(end));
// }
// // Define the static member
// thread_local thread* thread::_current = nullptr;
// // Implementation of global functions
// void enable_timer(steady_clock_type::time_point when)
// {
//     engine().enable_timer(when);
// }
// #endif
