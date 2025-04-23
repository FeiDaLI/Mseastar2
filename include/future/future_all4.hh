// #ifndef FUTURE_ALL_HH
// #define FUTURE_ALL_HH

// #include "../task/task.hh"
// #include <stdexcept>
// #include <atomic>
// #include <memory>
// #include <utility>
// #include <tuple>
// #include <type_traits>
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

// #include "../timer/timer_all.hh"
// /*----------------------------------------thread-----------------------------------------------------*/

// #ifndef HAVE_GCC6_CONCEPTS
// #define GCC6_CONCEPT(x...)
// #define GCC6_NO_CONCEPT(x...) x
// #endif
// namespace bi = boost::intrusive;
// // Forward declarations for template classes
// template <typename... T>
// class promise;

// template<>
// class promise<void>;


// extern __thread bool g_need_preempt;
// inline bool need_preempt() {
//     return true;
//     // prevent compiler from eliminating loads in a loop
//     std::atomic_signal_fence(std::memory_order_seq_cst);
//     return g_need_preempt;
// }


// template <typename... T>
// class future;
// class thread;
// // Forward declarations for other classes

// class thread_attributes;
// class thread_scheduling_group;
// struct jmp_buf_link;



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



// template <class... T>
// class promise;

// template <class... T>
// class future;

// /// \brief Creates a \ref future in an available, value state.
// ///
// /// Creates a \ref future object that is already resolved.  This
// /// is useful when it is determined that no I/O needs to be performed
// /// to perform a computation (for example, because the data is cached
// /// in some buffer).
// template <typename... T, typename... A>
// future<T...> make_ready_future(A&&... value);

// /// \brief Creates a \ref future in an available, failed state.
// ///
// /// Creates a \ref future object that is already resolved in a failed
// /// state.  This is useful when no I/O needs to be performed to perform
// /// a computation (for example, because the connection is closed and
// /// we cannot read from it).
// template <typename... T>
// future<T...> make_exception_future(std::exception_ptr value) noexcept;

// /// \cond internal
// void engine_exit(std::exception_ptr eptr = {});

// void report_failed_future(std::exception_ptr ex);
// /// \endcond

// //
// // A future/promise pair maintain one logical value (a future_state).
// // To minimize allocations, the value is stored in exactly one of three
// // locations:
// //
// // - in the promise (so long as it exists, and before a .then() is called)
// //
// // - in the task associated with the .then() clause (after .then() is called,
// //   if a value was not set)
// //
// // - in the future (if the promise was destroyed, or if it never existed, as
// //   with make_ready_future()), before .then() is called
// //
// // Both promise and future maintain a pointer to the state, which is modified
// // the the state moves to a new location due to events (such as .then() being
// // called) or due to the promise or future being mobved around.
// //

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
//     void forward_to(promise<T...>& pr) noexcept {
//         assert(_state != state::future);
//         if (_state == state::exception) {
//             pr.set_urgent_exception(std::move(_u.ex));
//             _u.ex.~exception_ptr();
//         } else {
//             pr.set_urgent_value(std::move(_u.value));
//             _u.value.~tuple();
//         }
//         _state = state::invalid;
//     }
// };

// // Specialize future_state<> to overlap the state enum with the exception, as there
// // is no value to hold.
// //
// // Assumes std::exception_ptr is really a pointer.
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

// /// \endcond

// /// \brief promise - allows a future value to be made available at a later time.
// ///
// ///
// template <typename... T>
// class promise {
//     enum class urgent { no, yes };
//     future<T...>* _future = nullptr;
//     future_state<T...> _local_state;
//     future_state<T...>* _state;
//     std::unique_ptr<task> _task;
//     static constexpr bool copy_noexcept = future_state<T...>::copy_noexcept;
// public:
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

//     /// \brief Gets the promise's associated future.
//     ///
//     /// The future and promise will be remember each other, even if either or
//     /// both are moved.  When \c set_value() or \c set_exception() are called
//     /// on the promise, the future will be become ready, and if a continuation
//     /// was attached to the future, it will run.
//     future<T...> get_future() noexcept;

//     /// \brief Sets the promise's value (as tuple; by copying)
//     ///
//     /// Copies the tuple argument and makes it available to the associated
//     /// future.  May be called either before or after \c get_future().
//     void set_value(const std::tuple<T...>& result) noexcept(copy_noexcept) {
//         do_set_value<urgent::no>(result);
//     }

//     /// \brief Sets the promises value (as tuple; by moving)
//     ///
//     /// Moves the tuple argument and makes it available to the associated
//     /// future.  May be called either before or after \c get_future().
//     void set_value(std::tuple<T...>&& result) noexcept {
//         do_set_value<urgent::no>(std::move(result));
//     }

//     /// \brief Sets the promises value (variadic)
//     ///
//     /// Forwards the arguments and makes them available to the associated
//     /// future.  May be called either before or after \c get_future().
//     template <typename... A>
//     void set_value(A&&... a) noexcept {
//         assert(_state);
//         _state->set(std::forward<A>(a)...);
//         make_ready<urgent::no>();
//     }

//     /// \brief Marks the promise as failed
//     ///
//     /// Forwards the exception argument to the future and makes it
//     /// available.  May be called either before or after \c get_future().
//     void set_exception(std::exception_ptr ex) noexcept {
//         do_set_exception<urgent::no>(std::move(ex));
//     }

//     /// \brief Marks the promise as failed
//     ///
//     /// Forwards the exception argument to the future and makes it
//     /// available.  May be called either before or after \c get_future().
//     template<typename Exception>
//     void set_exception(Exception&& e) noexcept {
//         set_exception(make_exception_ptr(std::forward<Exception>(e)));
//     }
// private:
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
// private:
//     template <typename Func>
//     void schedule(Func&& func) {
//         auto tws = std::make_unique<continuation<Func, T...>>(std::move(func));
//         _state = &tws->_state;
//         _task = std::move(tws); 
//         // 调用then的时候,帮then的回调函数绑定到future对应的promise的task上.
//     }
//     template<urgent Urgent>
//     __attribute__((always_inline))
//     void make_ready() noexcept;
//     void migrated() noexcept;
//     void abandoned() noexcept;

//     template <typename... U>
//     friend class future;

//     friend class future_state<T...>;
// };

// /// \brief Specialization of \c promise<void>
// ///
// /// This is an alias for \c promise<>, for generic programming purposes.
// /// For example, You may have a \c promise<T> where \c T can legally be
// /// \c void.
// template<>
// class promise<void> : public promise<> {};

// /// @}

// /// \addtogroup future-util
// /// @{


// /// \brief Check whether a type is a future
// ///
// /// This is a type trait evaluating to \c true if the given type is a
// /// future.
// ///
// template <typename... T> struct is_future : std::false_type {};

// /// \cond internal
// /// \addtogroup future-util
// template <typename... T> struct is_future<future<T...>> : std::true_type {};

// struct ready_future_marker {};
// struct ready_future_from_tuple_marker {};
// struct exception_future_marker {};

// /// \endcond


// /// \brief Converts a type to a future type, if it isn't already.
// ///
// /// \return Result in member type 'type'.
// template <typename T>
// struct futurize;

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
// /// \endcond

// // Converts a type to a future type, if it isn't already.
// template <typename T>
// using futurize_t = typename futurize<T>::type;

// /// @}


// GCC6_CONCEPT(

// template <typename T>
// concept bool Future = is_future<T>::value;

// template <typename Func, typename... T>
// concept bool CanApply = requires (Func f, T... args) {
//     f(std::forward<T>(args)...);
// };
// template <typename Func, typename Return, typename... T>
// concept bool ApplyReturns = requires (Func f, T... args) {
//     { f(std::forward<T>(args)...) } -> Return;
// };
// template <typename Func, typename... T>
// concept bool ApplyReturnsAnyFuture = requires (Func f, T... args) {
//     requires is_future<decltype(f(std::forward<T>(args)...))>::value;
// };

// )


// /// \addtogroup future-module
// /// @{

// /// \brief A representation of a possibly not-yet-computed value.
// ///
// /// A \c future represents a value that has not yet been computed
// /// (an asynchronous computation).  It can be in one of several
// /// states:
// ///    - unavailable: the computation has not been completed yet
// ///    - value: the computation has been completed successfully and a
// ///      value is available.
// ///    - failed: the computation completed with an exception.
// ///
// /// methods in \c future allow querying the state and, most importantly,
// /// scheduling a \c continuation to be executed when the future becomes
// /// available.  Only one such continuation may be scheduled.
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
//     /// \brief gets the value returned by the computation
//     ///
//     /// Requires that the future be available.  If the value
//     /// was computed successfully, it is returned (as an
//     /// \c std::tuple).  Otherwise, an exception is thrown.
//     ///
//     /// If get() is called in a \ref seastar::thread context,
//     /// then it need not be available; instead, the thread will
//     /// be paused until the future becomes available.
//     [[gnu::always_inline]]
//     std::tuple<T...> get();

//     [[gnu::always_inline]]
//      std::exception_ptr get_exception() {
//         return get_available_state().get_exception();
//     }

//     /// Gets the value returned by the computation.
//     ///
//     /// Similar to \ref get(), but instead of returning a
//     /// tuple, returns the first value of the tuple.  This is
//     /// useful for the common case of a \c future<T> with exactly
//     /// one type parameter.
//     ///
//     /// Equivalent to: \c std::get<0>(f.get()).
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
//             // catch possible std::bad_alloc in schedule() above
//             // nothing can be done about it, we cannot break future chain by returning
//             // ready future while 'this' future is not ready
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
//         { }

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

//     /// \brief Ignore any result hold by this future
//     ///
//     /// Ignore any result (value or exception) hold by this future.
//     /// Use with caution since usually ignoring exception is not what
//     /// you want
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




// inline
// void future_state<>::forward_to(promise<>& pr) noexcept {
//     assert(_u.st != state::future && _u.st != state::invalid);
//     if (_u.st >= state::exception_min) {
//         pr.set_urgent_exception(std::move(_u.ex));
//         _u.ex.~exception_ptr();
//     } else {
//         pr.set_urgent_value(std::tuple<>());
//     }
//     _u.st = state::invalid;
// }

// template <typename... T>
// inline
// future<T...>
// promise<T...>::get_future() noexcept {
//     assert(!_future && _state && !_task);
//     return future<T...>(this);
// }

// template <typename... T>
// template<typename promise<T...>::urgent Urgent>
// inline
// void promise<T...>::make_ready() noexcept {
//     if (_task) {
//         _state = nullptr;
//         if (Urgent == urgent::yes && !need_preempt()) {
//             //这个不会调用.
//             ::schedule_urgent(std::move(_task));
//         } else {
//             ::schedule_normal(std::move(_task));
//         }
//     }
// }

// template <typename... T>
// inline
// void promise<T...>::migrated() noexcept {
//     if (_future) {
//         _future->_promise = this;
//     }
// }

// template <typename... T>
// inline
// void promise<T...>::abandoned() noexcept {
//     if (_future) {
//         assert(_state);
//         assert(_state->available() || !_task);
//         _future->_local_state = std::move(*_state);
//         _future->_promise = nullptr;
//     } else if (_state && _state->failed()) {
//         report_failed_future(_state->get_exception());
//     }
// }

// template <typename... T, typename... A>
// inline
// future<T...> make_ready_future(A&&... value) {
//     return future<T...>(ready_future_marker(), std::forward<A>(value)...);
// }
// /*
// 这里为什么要用两个模板参数,
// 一个T，一个A?
// */

// template <typename... T>
// inline
// future<T...> make_exception_future(std::exception_ptr ex) noexcept {
//     return future<T...>(exception_future_marker(), std::move(ex));
// }

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

// template <typename T>
// template <typename Arg>
// inline
// future<T>
// futurize<T>::make_exception_future(Arg&& arg) {
//     return ::make_exception_future<T>(std::forward<Arg>(arg));
// }

// template <typename... T>
// template <typename Arg>
// inline
// future<T...>
// futurize<future<T...>>::make_exception_future(Arg&& arg) {
//     return ::make_exception_future<T...>(std::forward<Arg>(arg));
// }

// template <typename Arg>
// inline
// future<>
// futurize<void>::make_exception_future(Arg&& arg) {
//     return ::make_exception_future<>(std::forward<Arg>(arg));
// }

// template <typename T>
// inline
// future<T>
// futurize<T>::from_tuple(std::tuple<T>&& value) {
//     return make_ready_future<T>(std::move(value));
// }

// template <typename T>
// inline
// future<T>
// futurize<T>::from_tuple(const std::tuple<T>& value) {
//     return make_ready_future<T>(value);
// }
// inline future<> futurize<void>::from_tuple(std::tuple<>&& value) {
//     return make_ready_future<>();
// }

// inline future<> futurize<void>::from_tuple(const std::tuple<>& value) {
//     return make_ready_future<>();
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









// // class thread_scheduling_group {
// //     public:
// //         std::chrono::nanoseconds _period;
// //         std::chrono::nanoseconds _quota;
// //         std::chrono::time_point<thread_clock> _this_period_ends = {};
// //         std::chrono::time_point<thread_clock> _this_run_start = {};
// //         std::chrono::nanoseconds _this_period_remain = {};
// //         /// \brief Constructs a \c thread_scheduling_group object
// //         ///
// //         /// \param period a duration representing the period
// //         /// \param usage which fraction of the \c period to assign for the scheduling group. Expected between 0 and 1.
// //         thread_scheduling_group(std::chrono::nanoseconds period, float usage);
// //         /// \brief changes the current maximum usage per period
// //         ///
// //         /// \param new_usage The new fraction of the \c period (Expected between 0 and 1) during which to run
// //         void update_usage(float new_usage) {
// //             _quota = std::chrono::duration_cast<std::chrono::nanoseconds>(new_usage * _period);
// //         }
// //         void account_start();
// //         void account_stop();
// //         std::chrono::steady_clock::time_point* next_scheduling_point() const;
// // };

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


// void thread_context::stack_deleter::operator()(char* ptr) const noexcept {
//     delete[] ptr;
// }

// void
// thread_context::setup() {
//     // use setcontext() for the initial jump, as it allows us
//     // to set up a stack, but continue with longjmp() as it's much faster.
//     ucontext_t initial_context;
//     auto q = uint64_t(reinterpret_cast<uintptr_t>(this));//将thread_
//     auto main = reinterpret_cast<void (*)()>(&thread_context::s_main);
//     auto r = getcontext(&initial_context);//保存当前上下文到initial_context中.
//     // throw_system_error_on(r == -1);
//     initial_context.uc_stack.ss_sp = _stack.get(); //设置栈空间
//     initial_context.uc_stack.ss_size = _stack_size;
//     initial_context.uc_link = nullptr;
//     makecontext(&initial_context, main, 2, int(q), int(q >> 32));  //makecontext前32位，后32位.
//     _context.thread = this;//_context是jmp_buf_link类型.(绑定父类型)
//     _context.initial_switch_in(&initial_context, _stack.get(), _stack_size);//进入这个函数准备执行了s_main
//     std::cout<<"执行完了回调函数"<<std::endl;
// }

// void thread_context::switch_in() {
//     if (_attr.scheduling_group) {
//         _attr.scheduling_group->account_start();
//         _context.set_yield_at(_attr.scheduling_group->_this_run_start + _attr.scheduling_group->_this_period_remain);
//     } else {
//         _context.clear_yield_at();//设置_context的yield_为false
//     }
//     _context.switch_in();
// }

// void thread_context::switch_out() {
//     if (_attr.scheduling_group) {
//         _attr.scheduling_group->account_stop();
//     }
//     _context.switch_out();
// }

// bool thread_context::should_yield() const {
//     if (!_attr.scheduling_group) {
//         return need_preempt();
//     }
//     return need_preempt() || bool(_attr.scheduling_group->next_scheduling_point());
// }

// // thread_local thread_context::preempted_thread_list thread_context::_preempted_threads;
// // thread_local thread_context::all_thread_list thread_context::_all_threads;
// future<> later() {
//     promise<> p;
//     auto f = p.get_future();
//     engine().force_poll(); //把need_preempted改为true(这句是没有意义的)
//     ::schedule_normal(make_task([p = std::move(p)]() mutable {
//         p.set_value(); // 这段代码把一个p.set_value封装为一个task加到调度器中.
//     }));
//     return f;
// }

// void
// thread_context::yield() {
//     if (!_attr.scheduling_group) {
//         later().get();
//     } 
//     else
//     {
//         std::cout<<"yield 有scheduling group"<<std::endl;
//         auto when = _attr.scheduling_group->next_scheduling_point();
//         if (when) {
//             _preempted_it = _preempted_threads.insert(_preempted_threads.end(), this);
//             set_sched_promise();
//             auto fut = get_sched_promise()->get_future();
//             _sched_timer.arm(*when);
//             fut.get();
//             clear_sched_promise();
//         } else if (need_preempt()) {
//             later().get();
//         }
//     }
// }

// void
// thread_context::reschedule() {
//     _preempted_threads.erase(_preempted_it);
//     _sched_promise_ptr->set_value();
// }

// void
// thread_context::s_main(unsigned int lo, unsigned int hi) {
//     uintptr_t q = lo | (uint64_t(hi) << 32);
//     std::cout<<"执行s_main"<<std::endl;
//     reinterpret_cast<thread_context*>(q)->main();
// }
// /* 
//     因为makecontext绑定的函数传入的参数只能是32位，所以一个64位指针需要拆分为2个参数传入. 
//     参数传入的是thread_context
// */
 
// void
// thread_context::main() {
//     _context.initial_switch_in_completed();//这里什么都没有执行.
//     if (_attr.scheduling_group) {
//         std::cout<<"attr有scheduling group"<<std::endl;
//         _attr.scheduling_group->account_start();
//         //没有执行到这里.
//     }
//     try {
//         std::cout<<"开始执行回调函数"<<std::endl;
//         _func();            //执行线程绑定在context的函数.
//         _done.set_value(); // done的类型是promise<>，set_value把done对应的future_state<>状态设置为result.
//     } catch (...) {
//         _done.set_exception(std::current_exception());
//     }
//     if (_attr.scheduling_group) {
//         _attr.scheduling_group->account_stop();
//     }
//     _context.final_switch_out();
// }




// class gate_closed_exception : public std::exception {
// public:
//     virtual const char* what() const noexcept override {
//         return "gate closed";
//     }
// };

























// bool thread::try_run_one_yielded_thread() {
//     if (thread_context::_preempted_threads.empty()) {
//         return false;
//     }
//     auto* t = thread_context::_preempted_threads.front();
//     t->_sched_timer.cancel();
//     t->_sched_promise_ptr->set_value();
//     thread_context::_preempted_threads.pop_front();
//     return true;
// }
// thread_scheduling_group::thread_scheduling_group(std::chrono::nanoseconds period, float usage)
//         : _period(period), _quota(std::chrono::duration_cast<std::chrono::nanoseconds>(usage * period)) {
// }

// void thread_scheduling_group::account_start() {
//     auto now = thread_clock::now();
//     if (now >= _this_period_ends) {
//         _this_period_ends = now + _period;
//         _this_period_remain = _quota;
//     }
//     _this_run_start = now;
// }

// void thread_scheduling_group::account_stop() {
//     _this_period_remain -= thread_clock::now() - _this_run_start;
// }

// std::chrono::steady_clock::time_point*
// thread_scheduling_group::next_scheduling_point() const {
//     auto now = thread_clock::now();
//     auto current_remain = _this_period_remain - (now - _this_run_start);
//     if (current_remain > std::chrono::nanoseconds(0)) {
//         return nullptr;
//     }
//     static std::chrono::steady_clock::time_point result;
//     result = _this_period_ends - current_remain;
//     return &result;
// }

// void thread::yield() {
//     thread_impl::get()->yield();
// }

// bool thread::should_yield() {
//     return thread_impl::get()->should_yield();
// }

// // Define the static member
// thread_local thread* thread::_current = nullptr;

// // Constructor that takes a callable object
// template <typename Func>
// thread::thread(Func func) : thread(thread_attributes(), std::move(func)){}

// // Constructor that takes thread attributes and a callable object
// template <typename Func>
// thread::thread(thread_attributes attr, Func func)
//     : _context(std::make_unique<thread_context>(std::move(attr), std::move(func))) {}
//     /*
//         因为context是使用unique_ptr管理,所以当退出作用域时，unique会析构到，在析构时自动释放管理的内存.
//     */

// // Destructor
// thread::~thread() {
//     assert(!_context || _context->_joined);
// }

// // Join function
// future<> thread::join() {
//     _context->_joined = true;
//     return _context->_done.get_future();
// }


// #endif


// /// \brief Waits for the future to become available
// /// This method blocks the current thread until the future becomes available.
// template <typename... T>
// void future<T...>::wait() {
//     std::cout<<"future wait"<<std::endl;
//     auto thread = thread_impl::get();
//     assert(thread);//这里报错.

//     schedule([this, thread] (future_state<T...>&& new_state) {
//         *state() = std::move(new_state);
//         thread_impl::switch_in(thread);
//     });
//     thread_impl::switch_out(thread);
// }

// /// \brief Gets the value returned by the computation
// ///
// /// Requires that the future be available. If the value
// /// was computed successfully, it is returned (as an
// /// std::tuple). Otherwise, an exception is thrown.
// ///
// /// If get() is called in a seastar::thread context,
// /// then it need not be available; instead, the thread will
// /// be paused until the future becomes available.
// template <typename... T>
// [[gnu::always_inline]]
// std::tuple<T...> future<T...>::get() {
//     if (!state()->available()) {
//         std::cout<<"future.get  调用这里的wait"<<std::endl;
//         wait();
//     } else if (thread_impl::get() && thread_impl::should_yield()) {
//         std::cout<<"future.get  should_yield"<<std::endl;
//         thread_impl::yield();
//     }
//     return get_available_state().get();
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

// inline void jmp_buf_link::initial_switch_in(ucontext_t* initial_context, const void*, size_t)
// {
//     if(g_current_context){
//         std::cout<<"g_current_context非空"<<std::endl;
//     }else{
//         std::cout<<"g_current_context不空"<<std::endl;
//     }
//     auto prev = std::exchange(g_current_context, this);
//     link = prev;
//     if (setjmp(prev->jmpbuf) == 0) {
//         std::cout<<"init setjmp"<<std::endl;
//         //  如果第一次setjmp
//         setcontext(initial_context);  
//         //  这里会跳转到initial_context的入口函数中去执行.
//         //  使用setcontext而不是longjmp，需要设置完整的初始上下文.
//     }
//     /*
//     在这个过程中,已经执行完了绑定在线程上的回调函数。
//     */
//     std::cout<<"final long jmp"<<std::endl;
// }


// inline void jmp_buf_link::switch_in()
// {
//     auto prev = std::exchange(g_current_context, this);
//     link = prev;
//     std::cout<<"####!###"<<std::endl;
//     if (setjmp(prev->jmpbuf) == 0) {
//         longjmp(jmpbuf, 1);
//     }
// }

// inline void jmp_buf_link::switch_out()
// {
//     g_current_context = link;
//     if (setjmp(jmpbuf) == 0) {
//         longjmp(g_current_context->jmpbuf, 1);
//     }
// }

// inline void jmp_buf_link::initial_switch_in_completed()
// {

// }

// inline void jmp_buf_link::final_switch_out()
// {
//     g_current_context = link;//link就是该context对应的上一个context(恢复).
//     std::cout<<"final_switch_out"<<std::endl;
//     longjmp(g_current_context->jmpbuf, 1);//使用longjmp跳转到当前context的jmpbuf
//     //这个可能没用？
// }

// thread_context::~thread_context() {
//     std::cout<<"开始析构thread_context"<<std::endl;
//     _all_threads.erase(_all_it);//为什么？
// }

// // Define the static members
// thread_local std::list<thread_context*> thread_context::_preempted_threads;
// thread_local std::list<thread_context*> thread_context::_all_threads;

// thread_context::thread_context(thread_attributes attr, std::function<void ()> func)
//         : _attr(std::move(attr))
//         , _func(std::move(func)) {
//     setup();
//     std::cout<<"添加this到all_threads"<<std::endl;
//     _all_threads.push_front(this);
//     _all_it = _all_threads.begin();
//     //为什么这里是this,而不是*this，而不是_all_it?思考
//     //因为_all_threads存放的就是thread_context*，所以添加的也是指针。
// }


// thread_context::stack_holder
// thread_context::make_stack() {
//     auto stack = stack_holder(new char[_stack_size]);
//     return stack;
// }







// #include "../util/shared_ptr.hh"
// // #include "do_with.hh"
// // #include "timer.hh"
// #include "../util/bool_class.hh"
// #include <tuple>
// #include <iterator>
// #include <vector>
// #include <experimental/optional>
// #include "util/tuple_utils.hh"

// /// \cond internal
// extern __thread size_t task_quota;
// /// \endcond


// /// \addtogroup future-util
// /// @{

// /// \cond internal

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

// /// \endcond

// /// Run tasks in parallel (iterator version).
// ///
// /// Given a range [\c begin, \c end) of objects, run \c func on each \c *i in
// /// the range, and return a future<> that resolves when all the functions
// /// complete.  \c func should return a future<> that indicates when it is
// /// complete.  All invocations are performed in parallel.
// ///
// /// \param begin an \c InputIterator designating the beginning of the range
// /// \param end an \c InputIterator designating the end of the range
// /// \param func Function to apply to each element in the range (returning
// ///             a \c future<>)
// /// \return a \c future<> that resolves when all the function invocations
// ///         complete.  If one or more return an exception, the return value
// ///         contains one of the exceptions.
// template <typename Iterator, typename Func>
// GCC6_CONCEPT( requires requires (Func f, Iterator i) { { f(*i++) } -> future<>; } )
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

// /// Run tasks in parallel (range version).
// ///
// /// Given a \c range of objects, apply \c func to each object
// /// in the range, and return a future<> that resolves when all
// /// the functions complete.  \c func should return a future<> that indicates
// /// when it is complete.  All invocations are performed in parallel.
// ///
// /// \param range A range of objects to iterate run \c func on
// /// \param func  A callable, accepting reference to the range's
// ///              \c value_type, and returning a \c future<>.
// /// \return a \c future<> that becomes ready when the entire range
// ///         was processed.  If one or more of the invocations of
// ///         \c func returned an exceptional future, then the return
// ///         value will contain one of those exceptions.
// template <typename Range, typename Func>
// GCC6_CONCEPT( requires requires (Func f, Range r) { { f(*r.begin()) } -> future<>; } )
// inline
// future<>
// parallel_for_each(Range&& range, Func&& func) {
//     return parallel_for_each(std::begin(range), std::end(range),
//             std::forward<Func>(func));
// }

// // The AsyncAction concept represents an action which can complete later than
// // the actual function invocation. It is represented by a function which
// // returns a future which resolves when the action is done.

// /// \cond internal
// template<typename AsyncAction, typename StopCondition>
// static inline
// void do_until_continued(StopCondition&& stop_cond, AsyncAction&& action, promise<> p) {
//     while (!stop_cond()) {
//         try {
//             auto&& f = action();
//             if (!f.available() || need_preempt()) {
//                 f.then_wrapped([action = std::forward<AsyncAction>(action),
//                     stop_cond = std::forward<StopCondition>(stop_cond), p = std::move(p)](std::result_of_t<AsyncAction()> fut) mutable {
//                     if (!fut.failed()) {
//                         do_until_continued(stop_cond, std::forward<AsyncAction>(action), std::move(p));
//                     } else {
//                         p.set_exception(fut.get_exception());
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
// /// \endcond

// struct stop_iteration_tag { };
// using stop_iteration = bool_class<stop_iteration_tag>;

// /// Invokes given action until it fails or the function requests iteration to stop by returning
// /// \c stop_iteration::yes.
// ///
// /// \param action a callable taking no arguments, returning a future<stop_iteration>.  Will
// ///               be called again as soon as the future resolves, unless the
// ///               future fails, action throws, or it resolves with \c stop_iteration::yes.
// ///               If \c action is an r-value it can be moved in the middle of iteration.
// /// \return a ready future if we stopped successfully, or a failed future if
// ///         a call to to \c action failed.
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

// /// \cond internal

// template <typename T>
// struct repeat_until_value_type_helper;

// /// \endcond

// /// Type helper for repeat_until_value()
// template <typename T>
// struct repeat_until_value_type_helper<future<std::experimental::optional<T>>> {
//     /// The type of the value we are computing
//     using value_type = T;
//     /// Type used by \c AsyncAction while looping
//     using optional_type = std::experimental::optional<T>;
//     /// Return type of repeat_until_value()
//     using future_type = future<value_type>;
//     /// Return type of \c AsyncAction
//     using future_optional_type = future<optional_type>;
// };

// /// Return value of repeat_until_value()
// template <typename AsyncAction>
// using repeat_until_value_return_type
//         = typename repeat_until_value_type_helper<std::result_of_t<AsyncAction()>>::future_type;

// /// Invokes given action until it fails or the function requests iteration to stop by returning
// /// an engaged \c future<std::experimental::optional<T>>.  The value is extracted from the
// /// \c optional, and returned, as a future, from repeat_until_value().
// ///
// /// \param action a callable taking no arguments, returning a future<std::experimental::optional<T>>.
// ///               Will be called again as soon as the future resolves, unless the
// ///               future fails, action throws, or it resolves with an engaged \c optional.
// ///               If \c action is an r-value it can be moved in the middle of iteration.
// /// \return a ready future if we stopped successfully, or a failed future if
// ///         a call to to \c action failed.  The \c optional's value is returned.
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

// /// Invokes given action until it fails or given condition evaluates to true.
// ///
// /// \param stop_cond a callable taking no arguments, returning a boolean that
// ///                  evalutes to true when you don't want to call \c action
// ///                  any longer
// /// \param action a callable taking no arguments, returning a future<>.  Will
// ///               be called again as soon as the future resolves, unless the
// ///               future fails, or \c stop_cond returns \c true.
// /// \return a ready future if we stopped successfully, or a failed future if
// ///         a call to to \c action failed.
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

// /// Invoke given action until it fails.
// ///
// /// Calls \c action repeatedly until it returns a failed future.
// ///
// /// \param action a callable taking no arguments, returning a \c future<>
// ///        that becomes ready when you wish it to be called again.
// /// \return a future<> that will resolve to the first failure of \c action
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

// /// Call a function for each item in a range, sequentially (iterator version).
// ///
// /// For each item in a range, call a function, waiting for the previous
// /// invocation to complete before calling the next one.
// ///
// /// \param begin an \c InputIterator designating the beginning of the range
// /// \param end an \c InputIterator designating the endof the range
// /// \param action a callable, taking a reference to objects from the range
// ///               as a parameter, and returning a \c future<> that resolves
// ///               when it is acceptable to process the next item.
// /// \return a ready future on success, or the first failed future if
// ///         \c action failed.
// template<typename Iterator, typename AsyncAction>
// GCC6_CONCEPT( requires requires (Iterator i, AsyncAction aa) { { aa(*i) } -> future<> } )
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

// /// Call a function for each item in a range, sequentially (range version).
// ///
// /// For each item in a range, call a function, waiting for the previous
// /// invocation to complete before calling the next one.
// ///
// /// \param range an \c Range object designating input values
// /// \param action a callable, taking a reference to objects from the range
// ///               as a parameter, and returning a \c future<> that resolves
// ///               when it is acceptable to process the next item.
// /// \return a ready future on success, or the first failed future if
// ///         \c action failed.
// template<typename Container, typename AsyncAction>
// GCC6_CONCEPT( requires requires (Container c, AsyncAction aa) { { aa(*c.begin()) } -> future<> } )
// static inline
// future<> do_for_each(Container& c, AsyncAction&& action) {
//     return do_for_each(std::begin(c), std::end(c), std::forward<AsyncAction>(action));
// }

// /// \cond internal
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

// /// \endcond

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
// /// \endcond
// template <typename... Futs>
// concept bool AllAreFutures = impl::is_tuple_of_futures<std::tuple<Futs...>>::value;
// )



// /// Wait for many futures to complete, capturing possible errors (variadic version).
// ///
// /// Given a variable number of futures as input, wait for all of them
// /// to resolve (either successfully or with an exception), and return
// /// them as a tuple so individual values or exceptions can be examined.
// ///
// /// \param futs futures to wait for
// /// \return an \c std::tuple<> of all the futures in the input; when
// ///         ready, all contained futures will be ready as well.
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

// /// \endcond

// /// Wait for many futures to complete, capturing possible errors (iterator version).
// ///
// /// Given a range of futures as input, wait for all of them
// /// to resolve (either successfully or with an exception), and return
// /// them as a \c std::vector so individual values or exceptions can be examined.
// ///
// /// \param begin an \c InputIterator designating the beginning of the range of futures
// /// \param end an \c InputIterator designating the end of the range of futures
// /// \return an \c std::vector<> of all the futures in the input; when
// ///         ready, all contained futures will be ready as well.
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

// // @Mapper is a callable which transforms values from the iterator range
// // into a future<T>. @Reducer is an object which can be called with T as
// // parameter and yields a future<>. It may have a get() method which returns
// // a value of type U which holds the result of reduction. This value is wrapped
// // in a future and returned by this function. If the reducer has no get() method
// // then this function returns future<>.
// //
// // TODO: specialize for non-deferring reducer
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

// /// Asynchronous map/reduce transformation.
// ///
// /// Given a range of objects, an asynchronous unary function
// /// operating on these objects, an initial value, and a
// /// binary function for reducing, map_reduce() will
// /// transform each object in the range, then apply
// /// the the reducing function to the result.
// ///
// /// Example:
// ///
// /// Calculate the total size of several files:
// ///
// /// \code
// ///  map_reduce(files.begin(), files.end(),
// ///             std::mem_fn(file::size),
// ///             size_t(0),
// ///             std::plus<size_t>())
// /// \endcode
// ///
// /// Requirements:
// ///    - Iterator: an InputIterator.
// ///    - Mapper: unary function taking Iterator::value_type and producing a future<...>.
// ///    - Initial: any value type
// ///    - Reduce: a binary function taking two Initial values and returning an Initial
// ///
// /// Return type:
// ///    - future<Initial>
// ///
// /// \param begin beginning of object range to operate on
// /// \param end end of object range to operate on
// /// \param mapper map function to call on each object, returning a future
// /// \param initial initial input value to reduce function
// /// \param reduce binary function for merging two result values from \c mapper
// ///
// /// \return equivalent to \c reduce(reduce(initial, mapper(obj0)), mapper(obj1)) ...
// template <typename Iterator, typename Mapper, typename Initial, typename Reduce>
// GCC6_CONCEPT( requires requires (Iterator i, Mapper mapper, Initial initial, Reduce reduce) {
//      *i++;
//      { i != i} -> bool;
//      mapper(*i);
//      requires is_future<decltype(mapper(*i))>::value;
//      { reduce(std::move(initial), mapper(*i).get0()) } -> Initial;
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

// /// Asynchronous map/reduce transformation (range version).
// ///
// /// Given a range of objects, an asynchronous unary function
// /// operating on these objects, an initial value, and a
// /// binary function for reducing, map_reduce() will
// /// transform each object in the range, then apply
// /// the the reducing function to the result.
// ///
// /// Example:
// ///
// /// Calculate the total size of several files:
// ///
// /// \code
// ///  std::vector<file> files = ...;
// ///  map_reduce(files,
// ///             std::mem_fn(file::size),
// ///             size_t(0),
// ///             std::plus<size_t>())
// /// \endcode
// ///
// /// Requirements:
// ///    - Iterator: an InputIterator.
// ///    - Mapper: unary function taking Iterator::value_type and producing a future<...>.
// ///    - Initial: any value type
// ///    - Reduce: a binary function taking two Initial values and returning an Initial
// ///
// /// Return type:
// ///    - future<Initial>
// ///
// /// \param range object range to operate on
// /// \param mapper map function to call on each object, returning a future
// /// \param initial initial input value to reduce function
// /// \param reduce binary function for merging two result values from \c mapper
// ///
// /// \return equivalent to \c reduce(reduce(initial, mapper(obj0)), mapper(obj1)) ...
// template <typename Range, typename Mapper, typename Initial, typename Reduce>
// GCC6_CONCEPT( requires requires (Range range, Mapper mapper, Initial initial, Reduce reduce) {
//      std::begin(range);
//      std::end(range);
//      mapper(*std::begin(range));
//      requires is_future<std::remove_reference_t<decltype(mapper(*std::begin(range)))>>::value;
//      { reduce(std::move(initial), mapper(*std::begin(range)).get0()) } -> Initial;
// } )
// inline
// future<Initial>
// map_reduce(Range&& range, Mapper&& mapper, Initial initial, Reduce reduce) {
//     return map_reduce(std::begin(range), std::end(range), std::forward<Mapper>(mapper),
//             std::move(initial), std::move(reduce));
// }

// // Implements @Reducer concept. Calculates the result by
// // adding elements to the accumulator.
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

// static inline
// future<> now() {
//     return make_ready_future<>();
// }

// // Returns a future which is not ready but is scheduled to resolve soon.
// future<> later();

// class timed_out_error : public std::exception {
// public:
//     virtual const char* what() const noexcept {
//         return "timedout";
//     }
// };

// struct default_timeout_exception_factory {
//     static auto timeout() {
//         return timed_out_error();
//     }
// };

// /// \brief Wait for either a future, or a timeout, whichever comes first
// ///
// /// When timeout is reached the returned future resolves with an exception
// /// produced by ExceptionFactory::timeout(). By default it is \ref timed_out_error exception.
// ///
// /// Note that timing out doesn't cancel any tasks associated with the original future.
// /// It also doesn't cancel the callback registerred on it.
// ///
// /// \param f future to wait for
// /// \param timeout time point after which the returned future should be failed
// ///
// /// \return a future which will be either resolved with f or a timeout exception
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

// /// Wait for many futures to complete (variadic version).
// ///
// /// Given a variable number of futures as input, wait for all of them
// /// to resolve, and return a future containing the values of each individual
// /// resolved future.
// /// In case any of the given futures fails one of the exceptions is returned
// /// by this function as a failed future.
// ///
// /// \param futures futures to wait for
// /// \return future containing values of input futures
// template<typename... Futures>
// GCC6_CONCEPT( requires AllAreFutures<Futures...> )
// inline auto when_all_succeed(Futures&&... futures) {
//     using state = internal::when_all_state<internal::extract_values_from_futures_tuple<Futures...>, Futures...>;
//     auto s = make_lw_shared<state>(std::forward<Futures>(futures)...);
//     return s->wait_all(std::make_index_sequence<sizeof...(Futures)>());
// }

// /// Wait for many futures to complete (iterator version).
// ///
// /// Given a range of futures as input, wait for all of them
// /// to resolve, and return a future containing a vector of values of the
// /// original futures.
// /// In case any of the given futures fails one of the exceptions is returned
// /// by this function as a failed future.
// /// \param begin an \c InputIterator designating the beginning of the range of futures
// /// \param end an \c InputIterator designating the end of the range of futures
// /// \return an \c std::vector<> of all the valus in the input
// template <typename FutureIterator, typename = typename std::iterator_traits<FutureIterator>::value_type>
// GCC6_CONCEPT( requires requires (FutureIterator i) {
//      *i++;
//      { i != i } -> bool;
//      requires is_future<std::remove_reference_t<decltype(*i)>>::value;
// } )
// inline auto
// when_all_succeed(FutureIterator begin, FutureIterator end) {
//     using itraits = std::iterator_traits<FutureIterator>;
//     using result_transform = internal::extract_values_from_futures_vector<typename itraits::value_type>;
//     return internal::do_when_all<result_transform>(std::move(begin), std::move(end));
// }




// /// \addtogroup future-module
// /// @{

// /// Changes the clock used by shared_future<> and shared_promise<> when passed as the first template parameter.
// template<typename Clock>
// struct with_clock {};

// /// \cond internal

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

// /// \endcond

// /// \brief Like \ref future except the result can be waited for by many fibers.
// /// Represents a value which may not yet be ready. A fiber can wait for the value using
// /// the \ref future obtained by calling \ref get_future() or casting to \ref future type.
// /// Multiple fibers are allowed to obtain a \ref future for the result using the same
// /// instance of \ref shared_future.

// /// All futures obtained from shared_future should end up in the same state. However,
// /// if the value's copy constructor throws, some of the futures may end up in a failed state
// /// with an exception thrown from the copy constructor and end up with a state
// /// different than other futures.

// /// The scope of shared_future instance doesn't have to include scopes of the futures
// /// obtained from that instance. In that sense the returned futures are independent.

// /// shared_future can be copied at any time and all copies will resolve with the same value.

// /// shared_future can be in a disengaged state when it's default-constructed or moved-from.
// /// When it's in such a state we say it's invalid and obtaining futures must not be attempted.
// ///
// /// The types in the parameter pack T must all be copy-constructible.
// ///
// /// When the first type in the parameter pack is \ref with_clock then it has the effect
// /// of changing the clock used for timeouts by this instance. This type is omitted from
// /// the parameter of the future<> objects.
// ///
// /// Example:
// ///
// ///    future<int> f;
// ///    shared_future<with_clock<manual_clock>, int> sf(std::move(f));
// ///    future<int> f2 = sf;
// ///
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

//     /// \cond internal
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
//     /// \endcond
//     lw_shared_ptr<shared_state> _state;
// public:
//     /// \brief Forwards the result of future \c f into this shared_future.
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

//     /// \brief Creates a new \c future which will resolve with the result of this shared_future
//     ///
//     /// \param timeout When engaged, the returned future will resolve with \ref timed_out_error
//     /// if this shared_future doesn't resolve before timeout is reached.
//     ///
//     /// This object must be in a valid state.
//     future_type get_future(time_point timeout = time_point::max()) const {
//         return _state->get_future(timeout);
//     }

//     /// \brief Equivalent to \ref get_future()
//     operator future_type() const {
//         return get_future();
//     }

//     /// \brief Returns true if the instance is in valid state
//     bool valid() const {
//         return bool(_state);
//     }
// };

// /// \brief Like \ref promise except that its counterpart is \ref shared_future instead of \ref future
// ///
// /// When the shared_promise is made ready, every waiter is also made ready.
// ///
// /// Like the shared_future, the types in the parameter pack T must all be copy-constructible.
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


