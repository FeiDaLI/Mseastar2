// #include "future_all6.hh"

// template<typename Func>
// future<> smp::invoke_on_all(Func&& func) {
//         static_assert(std::is_same<future<>, typename futurize<std::result_of_t<Func()>>::type>::value, "bad Func signature");
//         return parallel_for_each(all_cpus(), [&func] (unsigned id) {
//             return smp::submit_to(id, Func(func));
//         });
// }



// template <typename Func>
// futurize_t<std::result_of_t<Func()>> smp::submit_to(unsigned t, Func&& func) {
//         using ret_type = std::result_of_t<Func()>;
//         if (t == engine().cpu_id()) {
//             try {
//                 if (!is_future<ret_type>::value) {
//                     // Non-deferring function, so don't worry about func lifetime
//                     return futurize<ret_type>::apply(std::forward<Func>(func));
//                 } else if (std::is_lvalue_reference<Func>::value) {
//                     // func is an lvalue, so caller worries about its lifetime
//                     return futurize<ret_type>::apply(func);
//                 } else {
//                     // Deferring call on rvalue function, make sure to preserve it across call
//                     auto w = std::make_unique<std::decay_t<Func>>(std::move(func));
//                     auto ret = futurize<ret_type>::apply(*w);
//                     return ret.finally([w = std::move(w)] {});
//                 }
//             } catch (...) {
//                 // Consistently return a failed future rather than throwing, to simplify callers
//                 return futurize<std::result_of_t<Func()>>::make_exception_future(std::current_exception());
//             }
//         } else {
//             // 这里是修复的地方
//             if (_qs != nullptr) {
//                 return _qs[t][engine().cpu_id()].submit(std::forward<Func>(func));
//             } else {
//                 return futurize<std::result_of_t<Func()>>::make_exception_future(std::runtime_error("smp::_qs is null"));
//             }
//         }
// }



// void io_queue::fill_shares_array() {
//     for (unsigned i = 0; i < _max_classes; ++i) {
//         _registered_shares[i].store(0);
//     }
// }

// template <typename... T>
// void future_state<T...>::forward_to(promise<T...>& pr) noexcept{
//     assert(_state != state::future);
//     if (_state == state::exception) {
//         pr.set_urgent_exception(std::move(_u.ex));
//         _u.ex.~exception_ptr();
//     } else {
//         pr.set_urgent_value(std::move(_u.value));
//         _u.value.~tuple();
//     }
//     _state = state::invalid;
// }

// void engine_exit(std::exception_ptr eptr) {
//     if (!eptr) {
//         engine().exit(0);
//         return;
//     }
//     std::cout<<"Exception: "<< std::endl;
//     engine().exit(1);
// }




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


// void thread::yield() {
//     thread_impl::get()->yield();
// }

// bool thread::should_yield() {
//     return thread_impl::get()->should_yield();
// }


// // Destructor
// thread::~thread() {
//     assert(!_context || _context->_joined);
// }

// void reactor::expire_manual_timers() {
//     complete_timers(engine()._manual_timers, engine()._expired_manual_timers, []{
//         std::cout<<"到期"<<std::endl;
//     });
// }

// void manual_clock::expire_timers() {
//     engine().expire_manual_timers();
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
//     if (setjmp(prev->jmpbuf) == 0) {
//         longjmp(jmpbuf, 1);
//     }
// }

// inline void jmp_buf_link::switch_out(){
//     g_current_context = link;
//     if (setjmp(jmpbuf) == 0) {
//         longjmp(g_current_context->jmpbuf, 1);
//     }
// }

// inline void jmp_buf_link::initial_switch_in_completed(){}

// inline void jmp_buf_link::final_switch_out(){
//     g_current_context = link;//link就是该context对应的上一个context(恢复).
//     std::cout<<"final_switch_out"<<std::endl;
//     longjmp(g_current_context->jmpbuf, 1);//使用longjmp跳转到当前context的jmpbuf
//     //这个可能没用？
// }

// thread_context::~thread_context() {
//     std::cout<<"开始析构thread_context"<<std::endl;
//     _all_threads.erase(_all_it);//为什么？
// }


// void thread_context::yield() {
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

// void thread_context::reschedule() {
//     _preempted_threads.erase(_preempted_it);
//     _sched_promise_ptr->set_value();
// }

// void thread_context::s_main(unsigned int lo, unsigned int hi) {
//     uintptr_t q = lo | (uint64_t(hi) << 32);
//     std::cout<<"执行s_main"<<std::endl;
//     reinterpret_cast<thread_context*>(q)->main();
// }

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

// smp_message_queue::smp_message_queue(reactor* from, reactor* to)
//     : _pending(to)
//     , _completed(from)
// {
// }

// void smp_message_queue::stop() {
//     _metrics.clear();
// }
// void smp_message_queue::move_pending() {
//     auto begin = _tx.a.pending_fifo.cbegin();
//     auto end = _tx.a.pending_fifo.cend();
//     end = _pending.push(begin, end);
//     if (begin == end) {
//         return;
//     }
//     auto nr = end - begin;
//     _pending.maybe_wakeup();
//     _tx.a.pending_fifo.erase(begin, end);
//     _current_queue_length += nr;
//     _last_snt_batch = nr;
//     _sent += nr;
// }

// bool smp_message_queue::pure_poll_tx() const {
//     // can't use read_available(), not available on older boost
//     // empty() is not const, so need const_cast.
//     return !const_cast<lf_queue&>(_completed).empty();
// }

// void smp_message_queue::submit_item(std::unique_ptr<smp_message_queue::work_item> item) {
//     _tx.a.pending_fifo.push_back(item.get());
//     item.release();
//     if (_tx.a.pending_fifo.size() >= batch_size) {
//         move_pending();
//     }
// }

// void smp_message_queue::respond(work_item* item) {
//     _completed_fifo.push_back(item);
//     if (_completed_fifo.size() >= batch_size || engine()._stopped) {
//         flush_response_batch();
//     }
// }

// void smp_message_queue::flush_response_batch() {
//     if (!_completed_fifo.empty()) {
//         auto begin = _completed_fifo.cbegin();
//         auto end = _completed_fifo.cend();
//         end = _completed.push(begin, end);
//         if (begin == end) {
//             return;
//         }
//         _completed.maybe_wakeup();
//         _completed_fifo.erase(begin, end);
//     }
// }

// bool smp_message_queue::has_unflushed_responses() const {
//     return !_completed_fifo.empty();
// }

// bool smp_message_queue::pure_poll_rx() const {
//     // can't use read_available(), not available on older boost
//     // empty() is not const, so need const_cast.
//     return !const_cast<lf_queue&>(_pending).empty();
// }

// void
// smp_message_queue::lf_queue::maybe_wakeup() {
//     // Called after lf_queue_base::push().
//     //
//     // This is read-after-write, which wants memory_order_seq_cst,
//     // but we insert that barrier using systemwide_memory_barrier()
//     // because seq_cst is so expensive.
//     //
//     // However, we do need a compiler barrier:
//     std::atomic_signal_fence(std::memory_order_seq_cst);
//     if (remote->_sleeping.load(std::memory_order_relaxed)) {
//         // We are free to clear it, because we're sending a signal now
//         remote->_sleeping.store(false, std::memory_order_relaxed);
//         remote->wakeup();
//     }
// }

// template<size_t PrefetchCnt, typename Func>
// size_t smp_message_queue::process_queue(lf_queue& q, Func process) {
//     // copy batch to local memory in order to minimize
//     // time in which cross-cpu data is accessed
//     work_item* items[queue_length + PrefetchCnt];
//     work_item* wi;
//     if (!q.pop(wi))
//         return 0;
//     // start prefecthing first item before popping the rest to overlap memory
//     // access with potential cache miss the second pop may cause
//     prefetch<2>(wi);
//     auto nr = q.pop(items);
//     std::fill(std::begin(items) + nr, std::begin(items) + nr + PrefetchCnt, nr ? items[nr - 1] : wi);
//     unsigned i = 0;
//     do {
//         prefetch_n<2>(std::begin(items) + i, std::begin(items) + i + PrefetchCnt);
//         process(wi);
//         wi = items[i++];
//     } while(i <= nr);

//     return nr + 1;
// }

// size_t smp_message_queue::process_completions() {
//     auto nr = process_queue<prefetch_cnt*2>(_completed, [] (work_item* wi) {
//         wi->complete();
//         delete wi;
//     });
//     _current_queue_length -= nr;
//     _compl += nr;
//     _last_cmpl_batch = nr;

//     return nr;
// }

// void smp_message_queue::flush_request_batch() {
//     if (!_tx.a.pending_fifo.empty()) {
//         move_pending();
//     }
// }

// size_t smp_message_queue::process_incoming() {
//     auto nr = process_queue<prefetch_cnt>(_pending, [this] (work_item* wi) {
//         wi->process().then([this, wi] {
//             respond(wi);
//         });
//     });
//     _received += nr;
//     _last_rcv_batch = nr;
//     return nr;
// }

// void smp_message_queue::start(unsigned cpuid) {
//     _tx.init();
//     namespace sm = metrics;
//     char instance[10];
//     std::snprintf(instance, sizeof(instance), "%u-%u", engine().cpu_id(), cpuid);
//     _metrics.add_group("smp", {
//             // queue_length     value:GAUGE:0:U
//             // Absolute value of num packets in last tx batch.
//             sm::make_queue_length("send_batch_queue_length", _last_snt_batch, sm::description("Current send batch queue length"), {sm::shard_label(instance)})(sm::metric_disabled),
//             sm::make_queue_length("receive_batch_queue_length", _last_rcv_batch, sm::description("Current receive batch queue length"), {sm::shard_label(instance)})(sm::metric_disabled),
//             sm::make_queue_length("complete_batch_queue_length", _last_cmpl_batch, sm::description("Current complete batch queue length"), {sm::shard_label(instance)})(sm::metric_disabled),
//             sm::make_queue_length("send_queue_length", _current_queue_length, sm::description("Current send queue length"), {sm::shard_label(instance)})(sm::metric_disabled),
//             // total_operations value:DERIVE:0:U
//             sm::make_derive("total_received_messages", _received, sm::description("Total number of received messages"), {sm::shard_label(instance)})(sm::metric_disabled),
//             // total_operations value:DERIVE:0:U
//             sm::make_derive("total_sent_messages", _sent, sm::description("Total number of sent messages"), {sm::shard_label(instance)})(sm::metric_disabled),
//             // total_operations value:DERIVE:0:U
//             sm::make_derive("total_completed_messages", _compl, sm::description("Total number of messages completed"), {sm::shard_label(instance)})(sm::metric_disabled)
//     });
// }




// // // 实现maybe_wakeup方法
// // void smp_message_queue::lf_queue::maybe_wakeup() {
// //     // 在调用lf_queue_base::push()之后调用
    
// //     // 这是读后写操作，通常需要memory_order_seq_cst，
// //     // 但我们使用systemwide_memory_barrier()插入该屏障，
// //     // 因为seq_cst成本很高。
    
// //     // 然而，我们确实需要一个编译器屏障：
// //     std::atomic_signal_fence(std::memory_order_seq_cst);
// //     if (remote->_sleeping.load(std::memory_order_relaxed)) {
// //         // 我们可以自由地清除它，因为我们现在正在发送信号
// //         remote->_sleeping.store(false, std::memory_order_relaxed);
// //         remote->wakeup();
// //     }
// // }

// // // 实现pure_poll_tx方法
// // bool smp_message_queue::pure_poll_tx() const {
// //     // 检查完成队列是否为空
// //     return !_completed.empty();
// // }

// // // 实现pure_poll_rx方法 
// // bool smp_message_queue::pure_poll_rx() const {
// //     // 检查挂起队列是否为空
// //     return !_pending.empty();
// // }



// // bool reactor::do_expire_lowres_timers() {
// //     if (engine()._lowres_next_timeout == lowres_clock::time_point()) {
// //         return false;
// //     }
// //     auto now = lowres_clock::now();
// //     if (now > engine()._lowres_next_timeout) {
// //         complete_timers(engine()._lowres_timers, engine()._expired_lowres_timers, [] {
// //             if (!engine()._lowres_timers.empty()) {
// //                 engine()._lowres_next_timeout = engine()._lowres_timers.get_next_timeout();
// //             } else {
// //                 engine()._lowres_next_timeout = lowres_clock::time_point();
// //             }
// //         });
// //         return true;
// //     }
// //     return false;
// // }

// bool reactor::do_check_lowres_timers() const{
//     if (engine()._lowres_next_timeout == lowres_clock::time_point()) {
//         return false;
//     }
//     return lowres_clock::now() > engine()._lowres_next_timeout;
// } 




// thread_context::stack_holder
// thread_context::make_stack() {
//     auto stack = stack_holder(new char[_stack_size]);
//     return stack;
// }




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














// signals::signals() : _pending_signals(0) {
// }

// signals::~signals() {
//     sigset_t mask;
//     sigfillset(&mask);
//     ::pthread_sigmask(SIG_BLOCK, &mask, NULL);
// }

// signals::signal_handler::signal_handler(int signo, std::function<void ()>&& handler)
//         : _handler(std::move(handler)) {
//             std::cout<<"调用signal_handler"<<std::endl;
//     struct sigaction sa;
//     sa.sa_sigaction = action;//这个是信号处理函数.
//     sa.sa_mask = make_empty_sigset_mask();
//     sa.sa_flags = SA_SIGINFO | SA_RESTART;
//     engine()._signals._pending_signals.fetch_or(1ull << signo, std::memory_order_relaxed);
//     std::cout<<"engine().signals"<<engine()._signals._pending_signals<<std::endl;
//     auto r = ::sigaction(signo, &sa, nullptr);
//     // throw_system_error_on(r == -1);
//     auto mask = make_sigset_mask(signo);
//     r = ::pthread_sigmask(SIG_UNBLOCK, &mask, NULL);
//     throw_pthread_error(r);
// }

// void signals::handle_signal(int signo, std::function<void ()>&& handler) {
//     std::cout<<"handle signal"<<std::endl;
//     _signal_handlers.emplace(std::piecewise_construct,std::make_tuple(signo), std::make_tuple(signo, std::move(handler)));
//     //插入singo,和对应的handler.
// }

// void signals::handle_signal_once(int signo, std::function<void ()>&& handler) {
//     return handle_signal(signo, [fired = false, handler = std::move(handler)] () mutable {
//         if (!fired) {
//             fired = true;
//             handler();
//         }
//     });
// }

// bool signals::poll_signal() {
//     auto signals = _pending_signals.load(std::memory_order_relaxed);
//     //为什么一直是0.

//     if (signals) {
//             std::cout<<"signals "<<signals<<std::endl;
//         _pending_signals.fetch_and(~signals, std::memory_order_relaxed);
//         for (size_t i = 0; i < sizeof(signals)*8; i++) {
//             // std::cout<<"遍历"<<std::endl;
//             if (signals & (1ull << i)) {
//                 //执行handler
//                 std::cout<<"执行handler"<<std::endl;
//                _signal_handlers.at(i)._handler();
//             }
//         }
//     }
//     return signals;
// }
// bool signals::pure_poll_signal() const {
//     return _pending_signals.load(std::memory_order_relaxed);
// }

// void signals::action(int signo, siginfo_t* siginfo, void* ignore) {
//     std::cout<<"action########"<<std::endl;
//     engine()._signals._pending_signals.fetch_or(1ull << signo, std::memory_order_relaxed);
// }




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


// // Join function
// future<> thread::join() {
//     _context->_joined = true;
//     return _context->_done.get_future();
// }



// template <typename T, typename E, typename EnableFunc>
// void reactor::complete_timers(T& timers, E& expired_timers, EnableFunc&& enable_fn) {
//     expired_timers = timers.expire(timers.now()); // 获取过期的定时器
//     std::cout << "Expired " << expired_timers.size() << " timers" << std::endl;
//     // 处理所有过期定时器
//     for (auto* timer_ptr : expired_timers) {
//         if (timer_ptr) {
//             std::cout << "Marking timer " << " as expired" << std::endl;
//             timer_ptr->_expired = true;
//         }
//     }
//     // arm表示定时器是否有一个过期时间.
//     while (!expired_timers.empty()) {
//         auto* timer_ptr = expired_timers.front();
//         expired_timers.pop_front();
//         if (timer_ptr) {
//             std::cout << "Processing timer "<<std::endl;
//             timer_ptr->_queued = false;
//             if (timer_ptr->_armed) {
//                 timer_ptr->_armed = false;
//                 if (timer_ptr->_period) {
//                     // std::cout << "Re-adding periodic timer " << timer_ptr->_timerid << std::endl;
//                     timer_ptr->readd_periodic();//周期定时器

//                 }
//                 try {
//                     std::cout << "Executing timer callback for " << std::endl;
//                     timer_ptr->_callback();
//                 } catch (const std::exception& e) {
//                     // std::cerr << "Timer " << timer_ptr->_timerid << " callback failed: " << e.what() << std::endl;
//                 } catch (...) {
//                     // std::cerr << "Timer " << timer_ptr->_timerid << " callback failed with unknown error" << std::endl;
//                 }
//             }
//         }
//     }
    
//     enable_fn();
// }

// int reactor::run(){
//     _signals.handle_signal(alarm_signal(), [this] {
//         complete_timers(_timers, _expired_timers, [this] {
//         if (!_timers.empty()) {
//                 std::cout << "Enabling timer for " << _timers.get_next_timeout()<<std::endl;//这行是有执行的.
//                 enable_timer(_timers.get_next_timeout());
//             }
//         });
//     });
//     while(true){
//         run_tasks(_pending_tasks);
//          _signals.poll_signal();
//         // do_check_lowres_timers();
//         // do_expire_lowres_timers();
//         // std::this_thread::sleep_for(100ms);
//     }
//     return 0;
// }


// boost::program_options::options_description
// reactor::get_options_description() {
//     namespace bpo = boost::program_options;
//     bpo::options_description opts("Core options");
//     return opts;
// }


// void reactor::configure(boost::program_options::variables_map vm) {


// }
// future<> reactor::run_exit_tasks() {
//     // _stop_requested.broadcast();
//     // _stopping = true;
//     // // stop_aio_eventfd_loop();
//     // return do_for_each(_exit_funcs.rbegin(), _exit_funcs.rend(), [] (auto& func) {
//     //     return func();
//     // });
// }

// reactor::reactor() {
//     // 初始化系统定时器
//     sigevent sev;
//     memset(&sev, 0, sizeof(sev));
//     sev.sigev_notify = SIGEV_SIGNAL;
//     sev.sigev_signo = SIGALRM;
//     int ret = timer_create(CLOCK_MONOTONIC, &sev, &_steady_clock_timer);
//     if (ret < 0) {
//         throw std::system_error(errno, std::system_category());
//     }
//     // thread_impl::init();
// }

// reactor::reactor(unsigned int id) {
//     // 初始化系统定时器
//     sigevent sev;
//     memset(&sev, 0, sizeof(sev));
//     sev.sigev_notify = SIGEV_SIGNAL;
//     sev.sigev_signo = SIGALRM;
//     int ret = timer_create(CLOCK_MONOTONIC, &sev, &_steady_clock_timer);
//     if (ret < 0) {
//         throw std::system_error(errno, std::system_category());
//     }
//     // thread_impl::init();
// }


// reactor::~reactor() {
//     if (_steady_clock_timer) {
//         timer_delete(_steady_clock_timer);
//     }
// }

// void reactor::at_exit(std::function<future<> ()> func) {
//     assert(!_stopping);
//     _exit_funcs.push_back(std::move(func));
// }


// void reactor::run_tasks(std::deque<std::unique_ptr<task>>& tasks) {
//     while (!tasks.empty()) {
//         std::cout<<"run_task开始执行"<<std::endl;
//         auto tsk = std::move(tasks.front());
//         tasks.pop_front();
//         tsk->run();
//         std::cout<<"run_task结束执行"<<std::endl;
//         tsk.reset();
//     }
// }

// void reactor::exit(int ret) {
//     smp::submit_to(0, [this, ret] { _return = ret; stop(); });
// }

// void reactor::stop() {
//     assert(engine()._id == 0);
//     smp::cleanup_cpu();
//     if (!_stopping) {
        
//     }
// }



// void smp::pin(unsigned cpu_id) {
//     if (_using_dpdk) {
//         // dpdk does its own pinning
//         return;
//     }
//     pin_this_thread(cpu_id);
// }

// void smp::arrive_at_event_loop_end() {
//     if (_all_event_loops_done) {
//         _all_event_loops_done->wait();
//     }
// }

// void smp::allocate_reactor(unsigned id) {
//     assert(!reactor_holder);

//     // we cannot just write "local_engin = new reactor" since reactor's constructor
//     // uses local_engine
//     void *buf;
//     int r = posix_memalign(&buf, 64, sizeof(reactor));
//     assert(r == 0);
//     local_engine = reinterpret_cast<reactor*>(buf);
//     new (buf) reactor(id);
//     reactor_holder.reset(local_engine);
// }

// void smp::cleanup() {
//     smp::_threads = std::vector<posix_thread>();
//     _thread_loops.clear();
// }

// void smp::cleanup_cpu() {
//     size_t cpuid = engine().cpu_id();

//     if (_qs) {
//         for(unsigned i = 0; i < smp::count; i++) {
//             _qs[i][cpuid].stop();
//         }
//     }
// }

// void smp::create_thread(std::function<void ()> thread_loop) {
//     if (_using_dpdk) {
//         _thread_loops.push_back(std::move(thread_loop));
//     } else {
//         _threads.emplace_back(std::move(thread_loop));
//     }
// }


// static inline std::vector<char> string2vector(std::string str) {
//     auto v = std::vector<char>(str.begin(), str.end());
//     v.push_back('\0');
//     return v;
// }


// #include <boost/lexical_cast.hpp>

// size_t parse_memory_size(std::string s) {
//     size_t factor = 1;
//     if (s.size()) {
//         auto c = s[s.size() - 1];
//         static std::string suffixes = "kMGT";
//         auto pos = suffixes.find(c);
//         if (pos == suffixes.npos) {
//             throw std::runtime_error("Cannot parse memory size");
//         }
//         factor <<= (pos + 1) * 10;
//         s = s.substr(0, s.size() - 1);
//     }
//     return boost::lexical_cast<size_t>(s) * factor;
// }


// template <typename... A>
// std::string format(const char* fmt, A&&... a) {
//     return "";
// }


// void smp::configure(boost::program_options::variables_map configuration)
// {
//     // Mask most, to prevent threads (esp. dpdk helper threads)
//     // from servicing a signal.  Individual reactors will unmask signals
//     // as they become prepared to handle them.
//     //
//     // We leave some signals unmasked since we don't handle them ourself.
//     sigset_t sigs;
//     sigfillset(&sigs);
//     for (auto sig : {SIGHUP, SIGQUIT, SIGILL, SIGABRT, SIGFPE, SIGSEGV,
//             SIGALRM, SIGCONT, SIGSTOP, SIGTSTP, SIGTTIN, SIGTTOU}) {
//         sigdelset(&sigs, sig);
//     }
//     pthread_sigmask(SIG_BLOCK, &sigs, nullptr);

//     install_oneshot_signal_handler<SIGSEGV, sigsegv_action>();
//     install_oneshot_signal_handler<SIGABRT, sigabrt_action>();


//     auto thread_affinity = configuration["thread-affinity"].as<bool>();
//     if (configuration.count("overprovisioned")
//            && configuration["thread-affinity"].defaulted()) {
//         thread_affinity = false;
//     }
//     if (!thread_affinity && _using_dpdk) {
//         printf("warning: --thread-affinity 0 ignored in dpdk mode\n");
//     }

//     smp::count = 1;
//     smp::_tmain = std::this_thread::get_id();
//     auto nr_cpus = resource::nr_processing_units();
//     resource::cpuset cpu_set;

//     // std::copy(boost::counting_iterator<unsigned>(0), boost::counting_iterator<unsigned>(nr_cpus),std::inserter(cpu_set, cpu_set.end()));

//     for (unsi i = 0; i < nr_cpus; ++i) {
//         cpu_set.insert(i);
//     }


//     if (configuration.count("cpuset")) {
//         cpu_set = configuration["cpuset"].as<cpuset_bpo_wrapper>().value;
//     }
//     if (configuration.count("smp")) {
//         nr_cpus = configuration["smp"].as<unsigned>();
//     } else {
//         nr_cpus = cpu_set.size();
//     }
//     smp::count = nr_cpus;
//     _reactors.resize(nr_cpus);
//     resource::configuration rc;
//     if (configuration.count("memory")) {
//         rc.total_memory = parse_memory_size(configuration["memory"].as<std::string>());

//     }
//     if (configuration.count("reserve-memory")) {
//         rc.reserve_memory = parse_memory_size(configuration["reserve-memory"].as<std::string>());
//     }
//     std::experimental::optional<std::string> hugepages_path;
//     if (configuration.count("hugepages")) {
//         hugepages_path = configuration["hugepages"].as<std::string>();
//     }
//     auto mlock = false;
//     if (configuration.count("lock-memory")) {
//         mlock = configuration["lock-memory"].as<bool>();
//     }
//     if (mlock) {
//         auto r = mlockall(MCL_CURRENT | MCL_FUTURE);
//         if (r) {
//             // Don't hard fail for now, it's hard to get the configuration right
//             printf("warning: failed to mlockall");
//         }
//     }

//     rc.cpus = smp::count;
//     rc.cpu_set = std::move(cpu_set);
//     if (configuration.count("max-io-requests")) {
//         rc.max_io_requests = configuration["max-io-requests"].as<unsigned>();
//     }

//     if (configuration.count("num-io-queues")) {
//         rc.io_queues = configuration["num-io-queues"].as<unsigned>();
//     }

//     auto resources = resource::allocate(rc);
//     std::vector<resource::cpu> allocations = std::move(resources.cpus);
//     if (thread_affinity) {
//         smp::pin(allocations[0].cpu_id);
//     }
//     memory::configure(allocations[0].mem, hugepages_path);

//     if (configuration.count("abort-on-seastar-bad-alloc")) {
//         memory::enable_abort_on_allocation_failure();
//     }

//     bool heapprof_enabled = configuration.count("heapprof");
//     memory::set_heap_profiling_enabled(heapprof_enabled);


//     // Better to put it into the smp class, but at smp construction time
//     // correct smp::count is not known.
//     static boost::barrier reactors_registered(smp::count);
//     static boost::barrier smp_queues_constructed(smp::count);
//     static boost::barrier inited(smp::count);

//     auto io_info = std::move(resources.io_queues);

//     std::vector<io_queue*> all_io_queues;
//     all_io_queues.resize(io_info.coordinators.size());
//     io_queue::fill_shares_array();

//     auto alloc_io_queue = [io_info, &all_io_queues] (unsigned shard) {
//         auto cid = io_info.shard_to_coordinator[shard];
//         int vec_idx = 0;
//         for (auto& coordinator: io_info.coordinators) {
//             if (coordinator.id != cid) {
//                 vec_idx++;
//                 continue;
//             }
//             if (shard == cid) {
//                 all_io_queues[vec_idx] = new io_queue(coordinator.id, coordinator.capacity, io_info.shard_to_coordinator);
//             }
//             return vec_idx;
//         }
//         assert(0); // Impossible
//     };

//     auto assign_io_queue = [&all_io_queues] (shard_id id, int queue_idx) {
//         if (all_io_queues[queue_idx]->coordinator() == id) {
//             engine().my_io_queue.reset(all_io_queues[queue_idx]);
//         }
//         engine()._io_queue = all_io_queues[queue_idx];
//         engine()._io_coordinator = all_io_queues[queue_idx]->coordinator();
//     };

//     _all_event_loops_done.emplace(smp::count);

//     unsigned i;
//     for (i = 1; i < smp::count; i++) {
//         auto allocation = allocations[i];
//         create_thread([configuration, hugepages_path, i, allocation, assign_io_queue, alloc_io_queue, thread_affinity, heapprof_enabled] {
//             auto thread_name = format("reactor-{}", i);
//             pthread_setname_np(pthread_self(), thread_name.c_str());
//             if (thread_affinity) {
//                 smp::pin(allocation.cpu_id);
//             }
//             memory::configure(allocation.mem, hugepages_path);
//             memory::set_heap_profiling_enabled(heapprof_enabled);
//             sigset_t mask;
//             sigfillset(&mask);
//             for (auto sig : { SIGSEGV }) {
//                 sigdelset(&mask, sig);
//             }
//             auto r = ::pthread_sigmask(SIG_BLOCK, &mask, NULL);
//             throw_pthread_error(r);
//             allocate_reactor(i);
//             _reactors[i] = &engine();
//             auto queue_idx = alloc_io_queue(i);
//             reactors_registered.wait();
//             smp_queues_constructed.wait();
//             start_all_queues();
//             assign_io_queue(i, queue_idx);
//             inited.wait();
//             engine().configure(configuration);
//             engine().run();
//         });
//     }

//     allocate_reactor(0);
//     _reactors[0] = &engine();
//     auto queue_idx = alloc_io_queue(0);


//     reactors_registered.wait();
//     smp::_qs = new smp_message_queue* [smp::count];
//     for(unsigned i = 0; i < smp::count; i++) {
//         smp::_qs[i] = reinterpret_cast<smp_message_queue*>(operator new[] (sizeof(smp_message_queue) * smp::count));
//         for (unsigned j = 0; j < smp::count; ++j) {
//             new (&smp::_qs[i][j]) smp_message_queue(_reactors[j], _reactors[i]);
//         }
//     }
//     smp_queues_constructed.wait();
//     start_all_queues();
//     assign_io_queue(0, queue_idx);
//     inited.wait();

//     engine().configure(configuration);
//     engine()._lowres_clock = std::make_unique<lowres_clock>();
// }

// bool smp::poll_queues() {
//     size_t got = 0;
//     for (unsigned i = 0; i < count; i++) {
//         if (engine().cpu_id() != i) {
//             auto& rxq = _qs[engine().cpu_id()][i];
//             rxq.flush_response_batch();
//             got += rxq.has_unflushed_responses();
//             got += rxq.process_incoming();
//             auto& txq = _qs[i][engine()._id];
//             txq.flush_request_batch();
//             got += txq.process_completions();
//         }
//     }
//     return got != 0;
// }

// bool smp::pure_poll_queues() {
//     for (unsigned i = 0; i < count; i++) {
//         if (engine().cpu_id() != i) {
//             auto& rxq = _qs[engine().cpu_id()][i];
//             rxq.flush_response_batch();
//             auto& txq = _qs[i][engine()._id];
//             txq.flush_request_batch();
//             if (rxq.pure_poll_rx() || txq.pure_poll_tx() || rxq.has_unflushed_responses()) {
//                 return true;
//             }
//         }
//     }
//     return false;
// }

// boost::program_options::options_description
// smp::get_options_description()
// {
//     namespace bpo = boost::program_options;
//     bpo::options_description opts("SMP options");
//     opts.add_options()
//         ("smp,c", bpo::value<unsigned>(), "number of threads (default: one per CPU)")
//         ("cpuset", bpo::value<cpuset_bpo_wrapper>(), "CPUs to use (in cpuset(7) format; default: all))")
//         ("memory,m", bpo::value<std::string>(), "memory to use, in bytes (ex: 4G) (default: all)")
//         ("reserve-memory", bpo::value<std::string>(), "memory reserved to OS (if --memory not specified)")
//         ("hugepages", bpo::value<std::string>(), "path to accessible hugetlbfs mount (typically /dev/hugepages/something)")
//         ("lock-memory", bpo::value<bool>(), "lock all memory (prevents swapping)")
//         ("thread-affinity", bpo::value<bool>()->default_value(true), "pin threads to their cpus (disable for overprovisioning)")
//         ("num-io-queues", bpo::value<unsigned>(), "Number of IO queues. Each IO unit will be responsible for a fraction of the IO requests. Defaults to the number of threads")
//         ("max-io-requests", bpo::value<unsigned>(), "Maximum amount of concurrent requests to be sent to the disk. Defaults to 128 times the number of IO queues")
//         ;
//     return opts;
// }
// unsigned smp::count = 1;
// bool smp::_using_dpdk;
// void smp::start_all_queues()
// {
//     for (unsigned c = 0; c < count; c++) {
//         if (c != engine().cpu_id()) {
//             _qs[c][engine().cpu_id()].start(c);
//         }
//     }
// }
// void smp::join_all()
// {
//     for (auto&& t: smp::_threads) {
//         t.join();
//     }
// }

// void* posix_thread::start_routine(void* arg) noexcept {
//     auto pfunc = reinterpret_cast<std::function<void ()>*>(arg);
//     (*pfunc)();
//     return nullptr;
// }

// posix_thread::posix_thread(std::function<void ()> func)
//     : posix_thread(attr{}, std::move(func)) {
// }

// posix_thread::posix_thread(attr a, std::function<void ()> func)
//     : _func(std::make_unique<std::function<void ()>>(std::move(func))) {
//     pthread_attr_t pa;
//     auto r = pthread_attr_init(&pa);
//     if (r) {
//         throw std::system_error(r, std::system_category());
//     }
//     auto stack_size = a._stack_size.size;
//     if (!stack_size) {
//         stack_size = 2 << 20;
//     }
//     // allocate guard area as well
//     _stack = mmap_anonymous(nullptr, stack_size + (4 << 20),
//         PROT_NONE, MAP_PRIVATE | MAP_NORESERVE);
//     auto stack_start = align_up(_stack.get() + 1, 2 << 20);
//     mmap_area real_stack = mmap_anonymous(stack_start, stack_size,
//         PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_FIXED | MAP_STACK);
//     real_stack.release(); // protected by @_stack
//     ::madvise(stack_start, stack_size, MADV_HUGEPAGE);
//     r = pthread_attr_setstack(&pa, stack_start, stack_size);
//     if (r) {
//         throw std::system_error(r, std::system_category());
//     }
//     r = pthread_create(&_pthread, &pa,
//                 &posix_thread::start_routine, _func.get());
//     if (r) {
//         throw std::system_error(r, std::system_category());
//     }
// }

// posix_thread::posix_thread(posix_thread&& x)
//     : _func(std::move(x._func)), _pthread(x._pthread), _valid(x._valid)
//     , _stack(std::move(x._stack)) {
//     x._valid = false;
// }

// posix_thread::~posix_thread() {
//     assert(!_valid);
// }

// void posix_thread::join() {
//     assert(_valid);
//     pthread_join(_pthread, NULL);
//     _valid = false;
// }

