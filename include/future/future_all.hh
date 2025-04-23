// #pragma once
// #include<memory>
// #include <cassert>


// #include <utility>
// #include <memory>
// #include <tuple>
// #include <exception>
// #include <memory>
// #include <utility>
// #include <type_traits>
// #include <cassert>
// #include <boost/optional.hpp>
// #include <boost/variant.hpp>
// #include <boost/any.hpp>
// #include <functional>
// #include <boost/range/adaptor/transformed.hpp>
// #include <boost/range/algorithm/copy.hpp>
// #include <boost/range/algorithm/for_each.hpp>





// struct ready_future_marker {};
// struct ready_future_from_tuple_marker {};
// struct exception_future_marker {};


// template<typename T, typename F>
// inline
// auto do_with(T&& rvalue, F&& f) {
//     auto obj = std::make_unique<T>(std::forward<T>(rvalue));
//     auto fut = f(*obj);
//     return fut.then_wrapped([obj = std::move(obj)] (auto&& fut) {
//         return std::move(fut);
//     });
// }

// /// \cond internal
// template <typename Tuple, size_t... Idx>
// inline
// auto
// cherry_pick_tuple(std::index_sequence<Idx...>, Tuple&& tuple) {
//     return std::make_tuple(std::get<Idx>(std::forward<Tuple>(tuple))...);
// }
// /// \endcond

// /// Executes the function \c func making sure the lock \c lock is taken,
// /// and later on properly released.
// ///
// /// \param lock the lock, which is any object having providing a lock() / unlock() semantics.
// ///        Caller must make sure that it outlives \ref func.
// /// \param func function to be executed
// /// \returns whatever \c func returns
// template<typename Lock, typename Func>
// inline
// auto with_lock(Lock& lock, Func&& func) {
//     return lock.lock().then([func = std::forward<Func>(func)] () mutable {
//         return func();
//     }).then_wrapped([&lock] (auto&& fut) {
//         lock.unlock();
//         return std::move(fut);
//     });
// }


// template <typename T1, typename T2, typename T3_or_F, typename... More>
// inline
// auto
// do_with(T1&& rv1, T2&& rv2, T3_or_F&& rv3, More&&... more) {
//     auto all = std::forward_as_tuple(
//             std::forward<T1>(rv1),
//             std::forward<T2>(rv2),
//             std::forward<T3_or_F>(rv3),
//             std::forward<More>(more)...);
//     constexpr size_t nr = std::tuple_size<decltype(all)>::value - 1;
//     using idx = std::make_index_sequence<nr>;
//     auto&& just_values = cherry_pick_tuple(idx(), std::move(all));
//     auto&& just_func = std::move(std::get<nr>(std::move(all)));
//     auto obj = std::make_unique<std::remove_reference_t<decltype(just_values)>>(std::move(just_values));
//     auto fut = std::apply(just_func, *obj);
//     return fut.then_wrapped([obj = std::move(obj)] (auto&& fut) {
//         return std::move(fut);
//     });
// }







// template <typename... T>
// struct future_state {
//     enum class state {
//          invalid,
//          future,
//          result,
//          exception,
//     } _state = state::future;
//     //future可以理解为异步操作正在运行中，表示没有结果
//     union any {
//         any() noexcept {}
//         ~any(){}
//         std::tuple<T...> value;//需要理解这个是什么意思
//         std::exception_ptr ex;
//     } _u;

//     future_state() noexcept {}

//     //移动构造函数,如果是result和exception需要转移所有权(placement new + 析构函数)
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

//     //析构函数(如果result和exception就调用析构函数)
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

//     //移动赋值运算符，调用移动构造函数
//     future_state& operator=(future_state&& x) noexcept {
//         if (this != &x) {
//             this->~future_state();
//             new (this) future_state(std::move(x));
//         }
//         return *this;
//     }

//     //如果是reuslt或者excepiton返回
//     bool available() const noexcept { return _state == state::result || _state == state::exception; }
//     bool failed() const noexcept { return _state == state::exception; }
//     void wait();

//     //设置值(传入的是左值)
//     void set(const std::tuple<T...>& value) noexcept {
//         assert(_state == state::future);
//         new (&_u.value) std::tuple<T...>(value);
//         _state = state::result;
//     }
//     //设置值(传入的是右值)，思考为什么不用通用引用？
//     void set(std::tuple<T...>&& value) noexcept {
//         assert(_state == state::future);
//         new (&_u.value) std::tuple<T...>(std::move(value));
//         _state = state::result;
//     }

//     //下面就是通用引用 (思考：只用这一个够吗？)
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

//     //只能由右值调用的函数(思考:为什么右值调用要移动)，转移所有权
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
//     U get_value() const& noexcept requires std::is_copy_constructible_v<U> {
//         assert(_state == state::result);
//         return _u.value;
//     }

//     std::tuple<T...> get() && {
//         //assert(_state != state::future);这行不应该去掉
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
//     //ignore有什么用
//     void ignore() noexcept {
//         assert(_state != state::future);
//         this->~future_state();
//         _state = state::invalid;
//     }

//     void forward_to(promise<T...>& pr) noexcept{
//         assert(_state != state::future);
//         if(_state == state::exception) {
//             pr.set_urgent_exception(std::move(_u.ex));
//             _u.ex.~exception_ptr();
//         }else{
//             pr.set_urgent_value(std::move(_u.value));
//             _u.value.~tuple();
//         }
//         _state = state::invalid;
//     }
// };



// template <>
// struct future_state<> {
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
//         _u.st = state::result;
//     }
//     void set_exception(std::exception_ptr ex) noexcept {
//         assert(_u.st == state::future);
//         new (&_u.ex) std::exception_ptr(ex);
//         assert(_u.st >= state::exception_min);
//     }
//     std::tuple<> get() && {
//         assert(_u.st != state::future);
//         if (_u.st >= state::exception_min) {
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
//     void forward_to(promise<>& pr) noexcept{
//         assert(_u.st != state::future && _u.st != state::invalid);
//         if (_u.st >= state::exception_min) {
//             pr.set_urgent_exception(std::move(_u.ex));
//             _u.ex.~exception_ptr();
//         } else {
//             pr.set_urgent_value(std::tuple<>());
//         }
//         _u.st = state::invalid;
//     }
// };

// // Explicit specialization for void
// template<>
// struct future_state<void> : public future_state<> {};



// // Forward declarations
// template <typename T>
// struct futurize;

// template <typename T>
// using futurize_t = typename futurize<T>::type;








// void report_failed_future(std::exception_ptr eptr);

// // Now include continuation.hh after the forward declarations
// #include "../task/continuation.hh"
// template <typename... T>
// class future;
// // Now we can define is_future
// template <typename T> struct is_future : std::false_type {};
// template <typename... T> struct is_future<future<T...>> : std::true_type {};
// template <typename T>
// inline constexpr bool is_future_v = is_future<T>::value;


// template <typename... T>
// class future {
// public:
//     using value_type = std::tuple<T...>;
//     using promise_type = promise<T...>;
//     using future_state_type = future_state<T...>;
//     promise_type* _promise;
//     future_state_type _local_state;
// /*----------------------------------构造函数和析构函数-----------------------------------------------------*/
//     future(promise_type* pr) noexcept : _promise(pr) {
//         _promise->_future = this;
//     }

//     /*设置值*/
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

//     explicit future(future_state_type&& state) noexcept
//             : _promise(nullptr), _local_state(std::move(state)) {
//     }



//     future(const future&) = delete;
//     void operator=(const future&) = delete;

//     //这个地方没有看懂
//     future(future&& x) noexcept : _promise(x._promise) {
//         if (!_promise) {
//             _local_state = std::move(x._local_state);
//         }
//         x._promise = nullptr;
//         if (_promise) {
//             _promise->_future = this;
//         }
//     }


//     future& operator=(future&& x) noexcept {
//         if (this != &x) {
//             this->~future();
//             new (this) future(std::move(x));
//         }
//         return *this;
//     }

//     ~future() {
//         if (_promise) {
//             _promise->_future = nullptr;
//         }
//         if (failed()) {
//             report_failed_future(state()->get_exception());
//         }
//     }
//  /*------------------------------------------------------------------------------------------*/
 
//     future_state_type* state() noexcept {
//         return _promise ? _promise->_state : &_local_state;
//     }
//     bool available() noexcept {
//         return state()->available();
//     }

//     bool failed() noexcept {
//         return state()->failed();
//     }

//     future_state_type get_available_state() noexcept {
//         auto st = state();
//         if (_promise) {
//             _promise->_future = nullptr;
//             _promise = nullptr;
//         }
//         return std::move(*st);
//     }

//     //这个Func是从哪来的？
//     template <typename Func>
//     void schedule(Func&& func){
//         if (state()->available()){
//             auto ptr = std::make_unique<continuation<Func, T...>>(std::move(func), std::move(*state()));
//             // Explicitly cast to task* for proper conversion
//             schedule_urgent(std::unique_ptr<task>(static_cast<task*>(ptr.release())));
//         } else {
//             assert(_promise);
//             _promise->schedule(std::move(func));
//             _promise->_future = nullptr;
//             _promise = nullptr; //清空两个双向指针
//         }
//     }
//     // Helper method to determine if preemption is needed
//     bool need_preempt() const {
//         return false; // Simplified implementation for now
//     }

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

//     std::tuple<T...> get() {
//         if (!state()->available()) {
//             wait();
//         } else if (thread_impl::get() && thread_impl::should_yield()) {
//             thread_impl::yield();
//         }
//         return get_available_state().get();
//     }

//      std::exception_ptr get_exception() {
//         return get_available_state().get_exception();
//     }

//     void wait() {
//         auto thread = thread_impl::get();
//         assert(thread);
//         schedule([this, thread] (future_state_type<T...>&& new_state) {
//              *state() = std::move(new_state);
//              thread_impl::switch_in(thread);
//         });
//         thread_impl::switch_out(thread);
//     }

// // Helper function to handle exceptions
// void engine_exit(std::exception_ptr ep) {
//     try {
//         if (ep) {
//             std::rethrow_exception(ep);
//         }
//     } catch (const std::exception& e) {
//         std::cerr << "Exiting on exception: " << e.what() << std::endl;
//     } catch (...) {
//         std::cerr << "Exiting on unknown exception" << std::endl;
//     }
//     std::abort();
// }

// template <typename Func, typename Result = futurize_t<std::result_of_t<Func(T&&...)>>>
// Result
// then(Func&& func) noexcept {
//         using futurator = futurize<std::result_of_t<Func(T&&...)>>;
//         if (available() && !need_preempt()) {
//             if (failed()) {
//                 return futurator::make_exception_future(get_available_state().get_exception());
//             } else {
//                 return futurator::apply(std::forward<Func>(func), get_available_state().get_value());
//             }
//         }
//         typename futurator::promise_type pr;
//         auto fut = pr.get_future();
//         try {
//             schedule([pr = std::move(pr), func = std::forward<Func>(func)] (auto&& state) mutable {
//                 if (state.failed()) {
//                     pr.set_exception(std::move(state).get_exception());
//                 } else {
//                     futurator::apply(std::forward<Func>(func), std::move(state).get_value()).forward_to(std::move(pr));
//                     //这段代码有问题.
//                 }
//             });
//         } catch (...) {
//             // catch possible std::bad_alloc in schedule() above
//             // nothing can be done about it, we cannot break future chain by returning
//             // ready future while 'this' future is not ready
//             abort();
//         }
//         return fut;
//     }

//     template <typename Func, typename Result = futurize_t<std::result_of_t<Func(future<T...>)>>>
//     Result
//     then_wrapped(Func&& func) noexcept {
//         // using futurator = futurize<std::result_of_t<Func(future<T...>)>>;
//         using futurator = futurize<std::result_of_t<Func(future<T...>)>>;
//         if (available() && !need_preempt()) {
//             return futurator::apply(std::forward<Func>(func), future<T...>(get_available_state()));
//         }
//         typename futurator::promise_type pr;
//         auto fut = pr.get_future();
//         try {
//             schedule([pr = std::move(pr), func = std::forward<Func>(func)] (auto&& state) mutable {
//                 futurator::apply(std::forward<Func>(func), future<T...>(std::move(state))).forward_to(std::move(pr));
//             });
//         } catch (...) {
//             // catch possible std::bad_alloc in schedule() above
//             // nothing can be done about it, we cannot break future chain by returning
//             // ready future while 'this' future is not ready
//             abort();
//         }
//         return fut;
//     }

    
//     template <typename Func>
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
//             using futurator = futurize<std::invoke_result_t<Func>>;
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

//     // Helper struct to extract function traits
//     template <typename F>
//     struct function_traits : public function_traits<decltype(&F::operator())> {};

//     // For regular functions
//     template <typename R, typename... Args>
//     struct function_traits<R(Args...)> {
//         using return_type = R;
//         static constexpr size_t arity = sizeof...(Args);
        
//         template <size_t N>
//         struct arg {
//             using type = typename std::tuple_element<N, std::tuple<Args...>>::type;
//         };
//     };

//     // For function pointers
//     template <typename R, typename... Args>
//     struct function_traits<R(*)(Args...)> : public function_traits<R(Args...)> {};

//     // For member function pointers
//     template <typename C, typename R, typename... Args>
//     struct function_traits<R(C::*)(Args...)> : public function_traits<R(Args...)> {};

//     // For const member function pointers
//     template <typename C, typename R, typename... Args>
//     struct function_traits<R(C::*)(Args...) const> : public function_traits<R(Args...)> {};

//     // For lambdas
//     template <typename C, typename R, typename... Args>
//     struct function_traits<R(C::*)(Args...) const noexcept> : public function_traits<R(Args...)> {};

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
// };

// // Explicit specialization for future<void>
// template<>
// class future<void> : public future<> {
// public:
//     // Inherit constructors from base class
//     using future<>::future;
    
//     // Add explicit conversion constructor from future<>
//     future(future<>&& f) noexcept : future<>(std::move(f)) {}
// };

// /*
// 辅助函数，根据标签类直接设置future的state.
// */




// template <typename... T, typename... A>
// inline future<T...> make_ready_future(A&&... value) {
//     return future<T...>(ready_future_marker(), std::forward<A>(value)...);
// }

// template <typename... T>
// inline future<T...> make_exception_future(std::exception_ptr ex) noexcept {
//     return future<T...>(exception_future_marker(), std::move(ex));
// }

// template <typename... T, typename Exception>
// inline
// future<T...> make_exception_future(Exception&& ex) noexcept {
//     return make_exception_future<T...>(std::make_exception_ptr(std::forward<Exception>(ex)));
// }



// #include <tuple>
// #include <type_traits> // For std::invoke_result_t and other type traits

// // Forward declarations instead of includes to prevent circular dependencies
// template <typename... T>
// class promise;

// template <typename... T>
// class future;

// // Forward declare functions from future.hh that we need
// template <typename... T, typename... A>
// future<T...> make_ready_future(A&&... value);

// template <typename... T>
// future<T...> make_exception_future(std::exception_ptr ex) noexcept;

// template <typename T>
// struct futurize; // 前向声明

// template <typename T>
// using futurize_t = typename futurize<T>::type;

// template <typename T>
// struct futurize {
//     using type = future<T>;
//     using promise_type = promise<T>;
//     using value_type = std::tuple<T>;
//     // 把func转为future<T>
//     template<typename Func, typename... FuncArgs>
//     static inline type apply(Func&& func, std::tuple<FuncArgs...>&& args) noexcept{
//         try {
//             return convert(std::apply(std::forward<Func>(func), std::move(args)));
//         } catch (...) {
//             return make_exception_future(std::current_exception());
//         }
//     }

//     template<typename Func, typename... FuncArgs>
//     static inline type apply(Func&& func, FuncArgs&&... args) noexcept{
//         try {
//             return convert(func(std::forward<FuncArgs>(args)...));
//         }catch (...){
//             return make_exception_future(std::current_exception());
//         }
//     }
//     //把T转为future<T>.
//     static inline type convert(T&& value) { return make_ready_future<T>(std::move(value)); }
//     static inline type convert(type&& value) { return std::move(value); }
//     //把tuple<T>转为future<T>.
//     static type from_tuple(value_type&& value){
//         return make_ready_future<T>(std::move(value));
//     }
//     static type from_tuple(const value_type& value){
//         return make_ready_future<T>(value);
//     }
//     //返回一个表示一个exception的错误.
//     template <typename Arg>
//     static type make_exception_future(Arg&& arg){
//         return ::make_exception_future<T>(std::forward<Arg>(arg));
//     }
// };

// template <>
// struct futurize<void> {
//     using type = future<>;
//     using promise_type = promise<>;
//     using value_type = std::tuple<>;
//     template<typename Func, typename... FuncArgs>
//     static inline type apply(Func&& func, std::tuple<FuncArgs...>&& args) noexcept{
//         return do_void_futurize_apply_tuple(std::forward<Func>(func), std::move(args));
//     }

//     template<typename Func, typename... FuncArgs>
//     static inline type apply(Func&& func, FuncArgs&&... args) noexcept{
//         return do_void_futurize_apply(std::forward<Func>(func), std::forward<FuncArgs>(args)...);
//     }
//     static inline type from_tuple(value_type&& value){ return make_ready_future<>();}
//     static inline type from_tuple(const value_type& value){ return make_ready_future<>();}

//     template <typename Arg>
//     static type make_exception_future(Arg&& arg){
//         return ::make_exception_future<>(std::forward<Arg>(arg));
//     }
// };

// /*
//     为什么没有tuple?
// */
// template <typename... Args>
// struct futurize<future<Args...>> {
//     using type = future<Args...>;
//     using promise_type = promise<Args...>;

//     template<typename Func, typename... FuncArgs>
//     static inline type apply(Func&& func, std::tuple<FuncArgs...>&& args) noexcept{
//         try {
//             return std::apply(std::forward<Func>(func), std::move(args));
//         } catch (...) {
//             return make_exception_future(std::current_exception());
//         }
//     }

//     template<typename Func, typename... FuncArgs>
//     static inline type apply(Func&& func, FuncArgs&&... args) noexcept{
//         try {
//             return func(std::forward<FuncArgs>(args)...);
//         } catch (...) {
//             return make_exception_future(std::current_exception());
//         }
//     }
//     static inline type convert(Args&&... values) { return make_ready_future<Args...>(std::move(values)...); }
//     static inline type convert(type&& value) { return std::move(value); }

//     template <typename Arg>
//     static type make_exception_future(Arg&& arg){
//         return ::make_exception_future<Args...>(std::forward<Arg>(arg));
//     }
// };



// #include <atomic>
// extern __thread bool g_need_preempt;
// inline bool need_preempt() {
// #ifndef DEBUG
//     // prevent compiler from eliminating loads in a loop
//     std::atomic_signal_fence(std::memory_order_seq_cst);
//     return g_need_preempt;
// #else
//     return true;
// #endif
// }



// #include <tuple>
// #include <exception>
// #include <memory>
// #include <utility>
// #include <type_traits>
// #include <cassert>
// #include <boost/optional.hpp>
// #include <boost/variant.hpp>
// #include <boost/any.hpp>
// #include <functional>
// #include <boost/range/adaptor/transformed.hpp>
// #include <boost/range/algorithm/copy.hpp>
// #include <boost/range/algorithm/for_each.hpp>
// // Include only what's needed directly
// #include "future_state.hh"
// #include "../task/task.hh"
// // #include "../task/schedule_.hh"

// // Forward declarations
// template <typename... T> class future;
// class task;

// template <typename Func, typename... T>
// struct continuation;

// void schedule_urgent(std::unique_ptr<task> t);
// void report_failed_future(std::exception_ptr eptr);

// template <typename... T>
// class promise {
// public:
//     enum class urgent { no, yes };
//     future<T...>* _future = nullptr;
//     future_state<T...> _local_state;
//     future_state<T...>* _state;
//     // 注意模板中指针的写法
//     std::unique_ptr<task> _task;
//     // promise和一个task关联,这个task是什么?为什么要和task关联?这个task什么时候执行?什么时候初始化?

// public:
//     /// \brief Constructs an empty \c promise.
//     /// Creates promise with no associated future yet(see get_future()).
//     promise() noexcept : _state(&_local_state) {}

// //右值构造 
//     promise(promise&& x) noexcept : _future(x._future), _state(x._state), _task(std::move(x._task)) {
//         if (_state == &x._local_state) {
//             _state = &_local_state;
//             _local_state = std::move(x._local_state);
//         }
//         x._future = nullptr;
//         x._state = nullptr;
//         migrated();
//     }//这个地方看不懂
//     //不能拷贝构造
//     promise(const promise&) = delete;
//     ~promise() noexcept {
//         abandoned();
//     }
//     promise& operator=(promise&& x) noexcept {
//         if (this != &x) {
//             this->~promise();
//             new (this) promise(std::move(x));//为什么要调用placement new?为什么我在其他地方的构造函数就没有看到？
//         }
//         return *this;
//     }
//     void operator=(const promise&) = delete;

//     /*
//         get_future()通过构造函数构造一个future并返回,
//         构造函数就是把promise的future指针指向future,把future的promise指针指向promise。
//         这样future和promise就关联起来了。
//     */
//     future<T...> get_future() noexcept{
//         //如果future是空指针,task是空指针，
//         assert(!_future && _state && !_task);
//         return future<T...>(this);
//     }

//     void set_value(const std::tuple<T...>& result) noexcept {
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

//     /// \brief Marks the promise as failed
//     /// Forwards the exception argument to the future and makes it
//     /// available. May be called either before or after \c get_future().
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

//     // Move these to public since they need to be accessed by future_state
//     void set_urgent_value(const std::tuple<T...>& result) noexcept {
//         do_set_value<urgent::yes>(result);
//     }

//     void set_urgent_value(std::tuple<T...>&& result) noexcept {
//         do_set_value<urgent::yes>(std::move(result));
//     }

//     void set_urgent_exception(std::exception_ptr ex) noexcept {
//         do_set_exception<urgent::yes>(std::move(ex));
//     }

//     template<urgent Urgent>
//     void do_set_value(std::tuple<T...> result) noexcept {
//         assert(_state);
//         _state->set(std::move(result));
//         make_ready<Urgent>();
//     }

//     template<urgent Urgent>
//     void do_set_exception(std::exception_ptr ex) noexcept {
//         assert(_state);
//         _state->set_exception(std::move(ex));
//         make_ready<Urgent>();
//     }

//     template <typename Func>
//     void schedule(Func&& func) {
//         auto tws = std::make_unique<continuation<Func, T...>>(std::move(func));
//         _state = &tws->_state;
//         // Explicitly cast to std::unique_ptr<task> to ensure proper conversion
//         _task = std::unique_ptr<task>(static_cast<task*>(tws.release()));
//     }
//     template<urgent Urgent>
//     void make_ready() noexcept;
//     void migrated() noexcept{
//         if(_future){
//             _future->_promise = this;
//         }
//     }
//     void abandoned() noexcept{
//         if(_future) {
//             assert(_state);
//             assert(_state->available() || !_task);
//             _future->_local_state = std::move(*_state);
//             _future->_promise = nullptr;
//         }else if(_state && _state->failed()) {
//             report_failed_future(_state->get_exception());
//         }
//     }
// };


// template<>
// class promise<void> : public promise<>{};

// void report_failed_future(std::exception_ptr eptr) {
//     throw("Exceptional future");
// }

// /*
//     promise中 make_ready 如果 task 不为空,就把 task 加到调度独立里面. 
// */

// template <typename... T>
// template<typename promise<T...>::urgent Urgent>
// inline void promise<T...>::make_ready() noexcept {
//     if (_task) {
//         _state = nullptr;
//         if (Urgent == urgent::yes){
//             schedule_urgent(std::move(_task));
//         } else {
//             schedule(std::move(_task));
//         }
//     }
// }
