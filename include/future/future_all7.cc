// #include "future_all7.hh"

// void reactor::enable_timer(steady_clock_type::time_point when) {
//     itimerspec its;
//     its.it_interval = {};
//     its.it_value = to_timespec(when);
//     auto ret = timer_settime(_steady_clock_timer, TIMER_ABSTIME, &its, NULL);
//     // throw_system_error_on(ret == -1);
// }


// /*  信号触发​:默认情况下，定时器到期会发送一个信号（如 SIGALRM）到进程。进程可以通过信号处理函数来处理定时器到期事件. */

// void reactor::add_timer(steady_timer *tmr) {
//     std::cout<<"reactor add timer"<<std::endl;
//     if (queue_timer(tmr)) {
//         enable_timer(_timers.get_next_timeout());
//     }
// }
// void reactor::add_timer(lowres_timer* tmr) {
//     if (queue_timer(tmr)) {
//         _lowres_next_timeout = _lowres_timers.get_next_timeout();
//     }
// }

// void reactor::add_timer(manual_timer* tmr) {
//     queue_timer(tmr);
// }
// bool reactor::queue_timer(lowres_timer* tmr) {
//     return _lowres_timers.insert(*tmr);
// }

// bool reactor::queue_timer(manual_timer* tmr) {
//     return _manual_timers.insert(*tmr);
// }

// bool reactor::queue_timer(steady_timer* tmr) {
//     std::cout<<"reaactor queue timer"<<std::endl;
//     return _timers.insert(*tmr);
// }


// /*
//     del_timer什么时候调用?
// */
// void reactor::del_timer(steady_timer* tmr) {
//     if (tmr->_expired) {
//         _expired_timers.erase(tmr->expired_it);  // 直接使用保存的迭代器
//         tmr->_expired = false;
//     } else {
//         _timers.remove(*tmr);  // 通过 it 成员快速删除
//     }
// }

// // 同理修改其他 del_timer 函数：
// void reactor::del_timer(lowres_timer* tmr) {
//     if (tmr->_expired) {
//         _expired_lowres_timers.erase(tmr->expired_it);
//         tmr->_expired = false;
//     } else {
//         _lowres_timers.remove(*tmr);
//     }
// }

// void reactor::del_timer(manual_timer* tmr) {
//     if (tmr->_expired) {
//         _expired_manual_timers.erase(tmr->expired_it);
//         tmr->_expired = false;
//     } else {
//         _manual_timers.remove(*tmr);
//     }
// }




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

//     for (unsigned i = 0; i < nr_cpus; ++i) {
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



// #include <iostream>


// namespace memory {

// static thread_local int abort_on_alloc_failure_suppressed = 0;

// disable_abort_on_alloc_failure_temporarily::disable_abort_on_alloc_failure_temporarily() {
//     ++abort_on_alloc_failure_suppressed;
// }

// disable_abort_on_alloc_failure_temporarily::~disable_abort_on_alloc_failure_temporarily() noexcept {
//     --abort_on_alloc_failure_suppressed;
// }

// }

// #ifndef DEFAULT_ALLOCATOR

// #include "../util/bitops.hh"
// #include "../util/align.hh"
// #include "../fd/posix.hh"
// #include "../util/shared_ptr.hh"
// #include <new>
// #include <cstdint>
// #include <algorithm>
// #include <limits>
// #include <cassert>
// #include <atomic>
// #include <mutex>
// #include <experimental/optional>
// #include <functional>
// #include <cstring>
// #include <sys/uio.h>  // For writev
// #include <boost/intrusive/list.hpp>
// #include <sys/mman.h>
// #include "../util/defer.hh"
// #include "../util/backtrace.hh"
// #include <unordered_set>
// #ifdef HAVE_NUMA
// #include <numaif.h>
// #endif

// struct allocation_site {
//     mutable size_t count = 0; // number of live objects allocated at backtrace.
//     mutable size_t size = 0; // amount of bytes in live objects allocated at backtrace.
//     mutable const allocation_site* next = nullptr;
//     saved_backtrace backtrace;

//     bool operator==(const allocation_site& o) const {
//         return backtrace == o.backtrace;
//     }

//     bool operator!=(const allocation_site& o) const {
//         return !(*this == o);
//     }
// };

// namespace std {

// template<>
// struct hash<::allocation_site> {
//     size_t operator()(const ::allocation_site& bi) const {
//         return std::hash<saved_backtrace>()(bi.backtrace);
//     }
// };

// }

// using allocation_site_ptr = const allocation_site*;

// namespace memory {

// static allocation_site_ptr get_allocation_site() __attribute__((unused));

// static std::atomic<bool> abort_on_allocation_failure{false};

// void enable_abort_on_allocation_failure() {
//     abort_on_allocation_failure.store(true, std::memory_order_seq_cst);
// }

// static void on_allocation_failure(size_t size) {
//     if (!abort_on_alloc_failure_suppressed
//             && abort_on_allocation_failure.load(std::memory_order_relaxed)) {
//         abort();
//     }
// }

// static constexpr unsigned cpu_id_shift = 36; // FIXME: make dynamic
// static constexpr unsigned max_cpus = 256;
// static constexpr size_t cache_line_size = 64;

// using pageidx = uint32_t;

// struct page;
// class page_list;

// static std::atomic<bool> live_cpus[max_cpus];

// static thread_local uint64_t g_allocs;
// static thread_local uint64_t g_frees;
// static thread_local uint64_t g_cross_cpu_frees;
// static thread_local uint64_t g_reclaims;

// using std::experimental::optional;

// using allocate_system_memory_fn
//         = std::function<mmap_area (optional<void*> where, size_t how_much)>;

// namespace bi = boost::intrusive;

// inline
// unsigned object_cpu_id(const void* ptr) {
//     return (reinterpret_cast<uintptr_t>(ptr) >> cpu_id_shift) & 0xff;
// }

// class page_list_link {
//     uint32_t _prev;
//     uint32_t _next;
//     friend class page_list;
// };

// static char* mem_base() {
//     static char* known;
//     static std::once_flag flag;
//     std::call_once(flag, [] {
//         size_t alloc = size_t(1) << 44;
//         auto r = ::mmap(NULL, 2 * alloc,
//                     PROT_NONE,
//                     MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE,
//                     -1, 0);
//         if (r == MAP_FAILED) {
//             abort();
//         }
//         ::madvise(r, 2 * alloc, MADV_DONTDUMP);
//         auto cr = reinterpret_cast<char*>(r);
//         known = align_up(cr, alloc);
//         ::munmap(cr, known - cr);
//         ::munmap(known + alloc, cr + 2 * alloc - (known + alloc));
//     });
//     return known;
// }

// constexpr bool is_page_aligned(size_t size) {
//     return (size & (page_size - 1)) == 0;
// }

// constexpr size_t next_page_aligned(size_t size) {
//     return (size + (page_size - 1)) & ~(page_size - 1);
// }

// class small_pool;

// struct free_object {
//     free_object* next;
// };

// struct page {
//     bool free;
//     uint8_t offset_in_span;
//     uint16_t nr_small_alloc;
//     uint32_t span_size; // in pages, if we're the head or the tail
//     page_list_link link;
//     small_pool* pool;  // if used in a small_pool
//     free_object* freelist;
// #ifdef SEASTAR_HEAPPROF
//     allocation_site_ptr alloc_site; // for objects whose size is multiple of page size, valid for head only
// #endif
// };

// class page_list {
//     uint32_t _front = 0;
//     uint32_t _back = 0;
// public:
//     page& front(page* ary) { return ary[_front]; }
//     page& back(page* ary) { return ary[_back]; }
//     bool empty() const { return !_front; }
//     void erase(page* ary, page& span) {
//         if (span.link._next) {
//             ary[span.link._next].link._prev = span.link._prev;
//         } else {
//             _back = span.link._prev;
//         }
//         if (span.link._prev) {
//             ary[span.link._prev].link._next = span.link._next;
//         } else {
//             _front = span.link._next;
//         }
//     }
//     void push_front(page* ary, page& span) {
//         auto idx = &span - ary;
//         if (_front) {
//             ary[_front].link._prev = idx;
//         } else {
//             _back = idx;
//         }
//         span.link._next = _front;
//         span.link._prev = 0;
//         _front = idx;
//     }
//     void pop_front(page* ary) {
//         if (ary[_front].link._next) {
//             ary[ary[_front].link._next].link._prev = 0;
//         } else {
//             _back = 0;
//         }
//         _front = ary[_front].link._next;
//     }
//     page* find(uint32_t n_pages, page* ary) {
//         auto n = _front;
//         while (n && ary[n].span_size < n_pages) {
//             n = ary[n].link._next;
//         }
//         if (!n) {
//             return nullptr;
//         }
//         return &ary[n];
//     }
// };

// class small_pool {
//     unsigned _object_size;
//     unsigned _span_size;
//     free_object* _free = nullptr;
//     size_t _free_count = 0;
//     unsigned _min_free;
//     unsigned _max_free;
//     unsigned _spans_in_use = 0;
//     page_list _span_list;
//     static constexpr unsigned idx_frac_bits = 2;
// private:
//     size_t span_bytes() const { return _span_size * page_size; }
// public:
//     explicit small_pool(unsigned object_size) noexcept;
//     ~small_pool();
//     void* allocate();
//     void deallocate(void* object);
//     unsigned object_size() const { return _object_size; }
//     bool objects_page_aligned() const { return is_page_aligned(_object_size); }
//     static constexpr unsigned size_to_idx(unsigned size);
//     static constexpr unsigned idx_to_size(unsigned idx);
//     allocation_site_ptr& alloc_site_holder(void* ptr);
// private:
//     void add_more_objects();
//     void trim_free_list();
//     float waste();
// };

// // index 0b0001'1100 -> size (1 << 4) + 0b11 << (4 - 2)

// constexpr unsigned
// small_pool::idx_to_size(unsigned idx) {
//     return (((1 << idx_frac_bits) | (idx & ((1 << idx_frac_bits) - 1)))
//               << (idx >> idx_frac_bits))
//                   >> idx_frac_bits;
// }

// constexpr unsigned
// small_pool::size_to_idx(unsigned size) {
//     return ((log2floor(size) << idx_frac_bits) - ((1 << idx_frac_bits) - 1))
//             + ((size - 1) >> (log2floor(size) - idx_frac_bits));
// }

// class small_pool_array {
// public:
//     static constexpr unsigned nr_small_pools = small_pool::size_to_idx(4 * page_size) + 1;
// private:
//     union u {
//         small_pool a[nr_small_pools];
//         u() {
//             for (unsigned i = 0; i < nr_small_pools; ++i) {
//                 new (&a[i]) small_pool(small_pool::idx_to_size(i));
//             }
//         }
//         ~u() {
//             // cannot really call destructor, since other
//             // objects may be freed after we are gone.
//         }
//     } _u;
// public:
//     small_pool& operator[](unsigned idx) { return _u.a[idx]; }
// };

// static constexpr size_t max_small_allocation
//     = small_pool::idx_to_size(small_pool_array::nr_small_pools - 1);

// constexpr size_t object_size_with_alloc_site(size_t size) {
// #ifdef SEASTAR_HEAPPROF
//     // For page-aligned sizes, allocation_site* lives in page::alloc_site, not with the object.
//     static_assert(is_page_aligned(max_small_allocation), "assuming that max_small_allocation is page aligned so that we"
//             " don't need to add allocation_site_ptr to objects of size close to it");
//     size_t next_page_aligned_size = next_page_aligned(size);
//     if (next_page_aligned_size - size > sizeof(allocation_site_ptr)) {
//         size += sizeof(allocation_site_ptr);
//     } else {
//         return next_page_aligned_size;
//     }
// #endif
//     return size;
// }

// #ifdef SEASTAR_HEAPPROF
// // Ensure that object_size_with_alloc_site() does not exceed max_small_allocation
// static_assert(object_size_with_alloc_site(max_small_allocation) == max_small_allocation, "");
// static_assert(object_size_with_alloc_site(max_small_allocation - 1) == max_small_allocation, "");
// static_assert(object_size_with_alloc_site(max_small_allocation - sizeof(allocation_site_ptr) + 1) == max_small_allocation, "");
// static_assert(object_size_with_alloc_site(max_small_allocation - sizeof(allocation_site_ptr)) == max_small_allocation, "");
// static_assert(object_size_with_alloc_site(max_small_allocation - sizeof(allocation_site_ptr) - 1) == max_small_allocation - 1, "");
// static_assert(object_size_with_alloc_site(max_small_allocation - sizeof(allocation_site_ptr) - 2) == max_small_allocation - 2, "");
// #endif

// struct cross_cpu_free_item {
//     cross_cpu_free_item* next;
// };

// struct cpu_pages {
//     uint32_t min_free_pages = 20000000 / page_size;
//     char* memory;
//     page* pages;
//     uint32_t nr_pages;
//     uint32_t nr_free_pages;
//     uint32_t current_min_free_pages = 0;
//     unsigned cpu_id = -1U;
//     std::function<void (std::function<void ()>)> reclaim_hook;
//     std::vector<reclaimer*> reclaimers;
//     static constexpr unsigned nr_span_lists = 32;
//     union pla {
//         pla() {
//             for (auto&& e : free_spans) {
//                 new (&e) page_list;
//             }
//         }
//         ~pla() {
//             // no destructor -- might be freeing after we die
//         }
//         page_list free_spans[nr_span_lists];  // contains spans with span_size >= 2^idx
//     } fsu;
//     small_pool_array small_pools;
//     alignas(cache_line_size) std::atomic<cross_cpu_free_item*> xcpu_freelist;
//     alignas(cache_line_size) std::vector<physical_address> virt_to_phys_map;
//     static std::atomic<unsigned> cpu_id_gen;
//     static cpu_pages* all_cpus[max_cpus];
//     union asu {
//         using alloc_sites_type = std::unordered_set<allocation_site>;
//         asu() {
//             new (&alloc_sites) alloc_sites_type();
//         }
//         ~asu() {} // alloc_sites live forever
//         alloc_sites_type alloc_sites;
//     } asu;
//     allocation_site_ptr alloc_site_list_head = nullptr; // For easy traversal of asu.alloc_sites from scylla-gdb.py
//     bool collect_backtrace = false;
//     char* mem() { return memory; }

//     void link(page_list& list, page* span);
//     void unlink(page_list& list, page* span);
//     struct trim {
//         unsigned offset;
//         unsigned nr_pages;
//     };
//     void maybe_reclaim();
//     template <typename Trimmer>
//     void* allocate_large_and_trim(unsigned nr_pages, Trimmer trimmer);
//     void* allocate_large(unsigned nr_pages);
//     void* allocate_large_aligned(unsigned align_pages, unsigned nr_pages);
//     page* find_and_unlink_span(unsigned nr_pages);
//     page* find_and_unlink_span_reclaiming(unsigned n_pages);
//     void free_large(void* ptr);
//     void free_span(pageidx start, uint32_t nr_pages);
//     void free_span_no_merge(pageidx start, uint32_t nr_pages);
//     void* allocate_small(unsigned size);
//     void free(void* ptr);
//     void free(void* ptr, size_t size);
//     bool try_cross_cpu_free(void* ptr);
//     void shrink(void* ptr, size_t new_size);
//     void free_cross_cpu(unsigned cpu_id, void* ptr);
//     bool drain_cross_cpu_freelist();
//     size_t object_size(void* ptr);
//     page* to_page(void* p) {
//         return &pages[(reinterpret_cast<char*>(p) - mem()) / page_size];
//     }

//     bool is_initialized() const;
//     bool initialize();
//     reclaiming_result run_reclaimers(reclaimer_scope);
//     void schedule_reclaim();
//     void set_reclaim_hook(std::function<void (std::function<void ()>)> hook);
//     void set_min_free_pages(size_t pages);
//     void resize(size_t new_size, allocate_system_memory_fn alloc_sys_mem);
//     void do_resize(size_t new_size, allocate_system_memory_fn alloc_sys_mem);
//     void replace_memory_backing(allocate_system_memory_fn alloc_sys_mem);
//     void init_virt_to_phys_map();
//     memory::memory_layout memory_layout();
//     translation translate(const void* addr, size_t size);
//     ~cpu_pages();
// };

// static thread_local cpu_pages cpu_mem;
// std::atomic<unsigned> cpu_pages::cpu_id_gen;
// cpu_pages* cpu_pages::all_cpus[max_cpus];

// void set_heap_profiling_enabled(bool enable) {
//     bool is_enabled = cpu_mem.collect_backtrace;
//     if (enable) {
//         if (!is_enabled) {
//             abort();
//         }
//     } else {
//         if (is_enabled) {
//             abort();
//         }
//     }
//     cpu_mem.collect_backtrace = enable;
// }

// // Free spans are store in the largest index i such that nr_pages >= 1 << i.
// static inline
// unsigned index_of(unsigned pages) {
//     return std::numeric_limits<unsigned>::digits - count_leading_zeros(pages) - 1;
// }

// // Smallest index i such that all spans stored in the index are >= pages.
// static inline
// unsigned index_of_conservative(unsigned pages) {
//     if (pages == 1) {
//         return 0;
//     }
//     return std::numeric_limits<unsigned>::digits - count_leading_zeros(pages - 1);
// }

// void
// cpu_pages::unlink(page_list& list, page* span) {
//     list.erase(pages, *span);
// }

// void
// cpu_pages::link(page_list& list, page* span) {
//     list.push_front(pages, *span);
// }

// void cpu_pages::free_span_no_merge(uint32_t span_start, uint32_t nr_pages) {
//     assert(nr_pages);
//     nr_free_pages += nr_pages;
//     auto span = &pages[span_start];
//     auto span_end = &pages[span_start + nr_pages - 1];
//     span->free = span_end->free = true;
//     span->span_size = span_end->span_size = nr_pages;
//     auto idx = index_of(nr_pages);
//     link(fsu.free_spans[idx], span);
// }

// void cpu_pages::free_span(uint32_t span_start, uint32_t nr_pages) {
//     page* before = &pages[span_start - 1];
//     if (before->free) {
//         auto b_size = before->span_size;
//         assert(b_size);
//         span_start -= b_size;
//         nr_pages += b_size;
//         nr_free_pages -= b_size;
//         unlink(fsu.free_spans[index_of(b_size)], before - (b_size - 1));
//     }
//     page* after = &pages[span_start + nr_pages];
//     if (after->free) {
//         auto a_size = after->span_size;
//         assert(a_size);
//         nr_pages += a_size;
//         nr_free_pages -= a_size;
//         unlink(fsu.free_spans[index_of(a_size)], after);
//     }
//     free_span_no_merge(span_start, nr_pages);
// }

// page*
// cpu_pages::find_and_unlink_span(unsigned n_pages) {
//     auto idx = index_of_conservative(n_pages);
//     auto orig_idx = idx;
//     if (n_pages >= (2u << idx)) {
//         throw std::bad_alloc();
//     }
//     while (idx < nr_span_lists && fsu.free_spans[idx].empty()) {
//         ++idx;
//     }
//     if (idx == nr_span_lists) {
//         if (initialize()) {
//             return find_and_unlink_span(n_pages);
//         }
//         // Can smaller list possibly hold object?
//         idx = index_of(n_pages);
//         if (idx == orig_idx) {   // was exact power of two
//             return nullptr;
//         }
//     }
//     auto& list = fsu.free_spans[idx];
//     page* span = list.find(n_pages, pages);
//     if (!span) {
//         return nullptr;
//     }
//     unlink(list, span);
//     return span;
// }

// page*
// cpu_pages::find_and_unlink_span_reclaiming(unsigned n_pages) {
//     while (true) {
//         auto span = find_and_unlink_span(n_pages);
//         if (span) {
//             return span;
//         }
//         if (run_reclaimers(reclaimer_scope::sync) == reclaiming_result::reclaimed_nothing) {
//             return nullptr;
//         }
//     }
// }

// void cpu_pages::maybe_reclaim() {
//     if (nr_free_pages < current_min_free_pages) {
//         drain_cross_cpu_freelist();
//         run_reclaimers(reclaimer_scope::sync);
//         if (nr_free_pages < current_min_free_pages) {
//             schedule_reclaim();
//         }
//     }
// }

// template <typename Trimmer>
// void*
// cpu_pages::allocate_large_and_trim(unsigned n_pages, Trimmer trimmer) {
//     // Avoid exercising the reclaimers for requests we'll not be able to satisfy
//     // nr_pages might be zero during startup, so check for that too
//     if (nr_pages && n_pages >= nr_pages) {
//         return nullptr;
//     }
//     page* span = find_and_unlink_span_reclaiming(n_pages);
//     if (!span) {
//         return nullptr;
//     }
//     auto span_size = span->span_size;
//     auto span_idx = span - pages;
//     nr_free_pages -= span->span_size;
//     trim t = trimmer(span_idx, nr_pages);
//     if (t.offset) {
//         free_span_no_merge(span_idx, t.offset);
//         span_idx += t.offset;
//         span_size -= t.offset;
//         span = &pages[span_idx];

//     }
//     if (t.nr_pages < span_size) {
//         free_span_no_merge(span_idx + t.nr_pages, span_size - t.nr_pages);
//     }
//     auto span_end = &pages[span_idx + t.nr_pages - 1];
//     span->free = span_end->free = false;
//     span->span_size = span_end->span_size = t.nr_pages;
//     span->pool = nullptr;
//     maybe_reclaim();
//     return mem() + span_idx * page_size;
// }

// void*
// cpu_pages::allocate_large(unsigned n_pages) {
//     return allocate_large_and_trim(n_pages, [n_pages] (unsigned idx, unsigned n) {
//         return trim{0, std::min(n, n_pages)};
//     });
// }

// void*
// cpu_pages::allocate_large_aligned(unsigned align_pages, unsigned n_pages) {
//     return allocate_large_and_trim(n_pages + align_pages - 1, [=] (unsigned idx, unsigned n) {
//         return trim{align_up(idx, align_pages) - idx, n_pages};
//     });
// }

// #ifdef SEASTAR_HEAPPROF

// class disable_backtrace_temporarily {
//     bool _old;
// public:
//     disable_backtrace_temporarily() {
//         _old = cpu_mem.collect_backtrace;
//         cpu_mem.collect_backtrace = false;
//     }
//     ~disable_backtrace_temporarily() {
//         cpu_mem.collect_backtrace = _old;
//     }
// };

// #else

// struct disable_backtrace_temporarily {
//     ~disable_backtrace_temporarily() {}
// };

// #endif

// static
// saved_backtrace get_backtrace() noexcept {
//     disable_backtrace_temporarily dbt;
//     return current_backtrace();
// }

// static
// allocation_site_ptr get_allocation_site() {
//     if (!cpu_mem.is_initialized() || !cpu_mem.collect_backtrace) {
//         return nullptr;
//     }
//     disable_backtrace_temporarily dbt;
//     allocation_site new_alloc_site;
//     new_alloc_site.backtrace = get_backtrace();
//     auto insert_result = cpu_mem.asu.alloc_sites.insert(std::move(new_alloc_site));
//     allocation_site_ptr alloc_site = &*insert_result.first;
//     if (insert_result.second) {
//         alloc_site->next = cpu_mem.alloc_site_list_head;
//         cpu_mem.alloc_site_list_head = alloc_site;
//     }
//     return alloc_site;
// }

// #ifdef SEASTAR_HEAPPROF

// allocation_site_ptr&
// small_pool::alloc_site_holder(void* ptr) {
//     if (objects_page_aligned()) {
//         return cpu_mem.to_page(ptr)->alloc_site;
//     } else {
//         return *reinterpret_cast<allocation_site_ptr*>(reinterpret_cast<char*>(ptr) + _object_size - sizeof(allocation_site_ptr));
//     }
// }

// #endif

// void*
// cpu_pages::allocate_small(unsigned size) {
//     auto idx = small_pool::size_to_idx(size);
//     auto& pool = small_pools[idx];
//     assert(size <= pool.object_size());
//     auto ptr = pool.allocate();
// #ifdef SEASTAR_HEAPPROF
//     if (!ptr) {
//         return nullptr;
//     }
//     allocation_site_ptr alloc_site = get_allocation_site();
//     if (alloc_site) {
//         ++alloc_site->count;
//         alloc_site->size += pool.object_size();
//     }
//     new (&pool.alloc_site_holder(ptr)) allocation_site_ptr{alloc_site};
// #endif
//     return ptr;
// }

// void cpu_pages::free_large(void* ptr) {
//     pageidx idx = (reinterpret_cast<char*>(ptr) - mem()) / page_size;
//     page* span = &pages[idx];
// #ifdef SEASTAR_HEAPPROF
//     auto alloc_site = span->alloc_site;
//     if (alloc_site) {
//         --alloc_site->count;
//         alloc_site->size -= span->span_size * page_size;
//     }
// #endif
//     free_span(idx, span->span_size);
// }

// size_t cpu_pages::object_size(void* ptr) {
//     pageidx idx = (reinterpret_cast<char*>(ptr) - mem()) / page_size;
//     page* span = &pages[idx];
//     if (span->pool) {
//         auto s = span->pool->object_size();
// #ifdef SEASTAR_HEAPPROF
//         // We must not allow the object to be extended onto the allocation_site_ptr field.
//         if (!span->pool->objects_page_aligned()) {
//             s -= sizeof(allocation_site_ptr);
//         }
// #endif
//         return s;
//     } else {
//         return size_t(span->span_size) * page_size;
//     }
// }

// void cpu_pages::free_cross_cpu(unsigned cpu_id, void* ptr) {
//     if (!live_cpus[cpu_id].load(std::memory_order_relaxed)) {
//         // Thread was destroyed; leak object
//         // should only happen for boost unit-tests.
//         return;
//     }
//     auto p = reinterpret_cast<cross_cpu_free_item*>(ptr);
//     auto& list = all_cpus[cpu_id]->xcpu_freelist;
//     auto old = list.load(std::memory_order_relaxed);
//     do {
//         p->next = old;
//     } while (!list.compare_exchange_weak(old, p, std::memory_order_release, std::memory_order_relaxed));
//     ++g_cross_cpu_frees;
// }

// bool cpu_pages::drain_cross_cpu_freelist() {
//     if (!xcpu_freelist.load(std::memory_order_relaxed)) {
//         return false;
//     }
//     auto p = xcpu_freelist.exchange(nullptr, std::memory_order_acquire);
//     while (p) {
//         auto n = p->next;
//         ++g_frees;
//         free(p);
//         p = n;
//     }
//     return true;
// }

// void cpu_pages::free(void* ptr) {
//     page* span = to_page(ptr);
//     if (span->pool) {
//         small_pool& pool = *span->pool;
// #ifdef SEASTAR_HEAPPROF
//         allocation_site_ptr alloc_site = pool.alloc_site_holder(ptr);
//         if (alloc_site) {
//             --alloc_site->count;
//             alloc_site->size -= pool.object_size();
//         }
// #endif
//         pool.deallocate(ptr);
//     } else {
//         free_large(ptr);
//     }
// }

// void cpu_pages::free(void* ptr, size_t size) {
//     // match action on allocate() so hit the right pool
//     if (size <= sizeof(free_object)) {
//         size = sizeof(free_object);
//     }
//     if (size <= max_small_allocation) {
//         size = object_size_with_alloc_site(size);
//         auto pool = &small_pools[small_pool::size_to_idx(size)];
// #ifdef SEASTAR_HEAPPROF
//         allocation_site_ptr alloc_site = pool->alloc_site_holder(ptr);
//         if (alloc_site) {
//             --alloc_site->count;
//             alloc_site->size -= pool->object_size();
//         }
// #endif
//         pool->deallocate(ptr);
//     } else {
//         free_large(ptr);
//     }
// }

// bool
// cpu_pages::try_cross_cpu_free(void* ptr) {
//     auto obj_cpu = object_cpu_id(ptr);
//     if (obj_cpu != cpu_id) {
//         free_cross_cpu(obj_cpu, ptr);
//         return true;
//     }
//     return false;
// }

// void cpu_pages::shrink(void* ptr, size_t new_size) {
//     auto obj_cpu = object_cpu_id(ptr);
//     assert(obj_cpu == cpu_id);
//     page* span = to_page(ptr);
//     if (span->pool) {
//         return;
//     }
//     size_t new_size_pages = align_up(new_size, page_size) / page_size;
//     auto old_size_pages = span->span_size;
//     assert(old_size_pages >= new_size_pages);
//     if (new_size_pages == old_size_pages) {
//         return;
//     }
// #ifdef SEASTAR_HEAPPROF
//     auto alloc_site = span->alloc_site;
//     if (alloc_site) {
//         alloc_site->size -= span->span_size * page_size;
//         alloc_site->size += new_size_pages * page_size;
//     }
// #endif
//     span->span_size = new_size_pages;
//     span[new_size_pages - 1].free = false;
//     span[new_size_pages - 1].span_size = new_size_pages;
//     pageidx idx = span - pages;
//     free_span(idx + new_size_pages, old_size_pages - new_size_pages);
// }

// cpu_pages::~cpu_pages() {
//     live_cpus[cpu_id].store(false, std::memory_order_relaxed);
// }

// bool cpu_pages::is_initialized() const {
//     return bool(nr_pages);
// }

// bool cpu_pages::initialize() {
//     if (is_initialized()) {
//         return false;
//     }
//     cpu_id = cpu_id_gen.fetch_add(1, std::memory_order_relaxed);
//     assert(cpu_id < max_cpus);
//     all_cpus[cpu_id] = this;
//     auto base = mem_base() + (size_t(cpu_id) << cpu_id_shift);
//     auto size = 32 << 20;  // Small size for bootstrap
//     auto r = ::mmap(base, size,
//             PROT_READ | PROT_WRITE,
//             MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED,
//             -1, 0);
//     if (r == MAP_FAILED) {
//         abort();
//     }
//     ::madvise(base, size, MADV_HUGEPAGE);
//     pages = reinterpret_cast<page*>(base);
//     memory = base;
//     nr_pages = size / page_size;
//     // we reserve the end page so we don't have to special case
//     // the last span.
//     auto reserved = align_up(sizeof(page) * (nr_pages + 1), page_size) / page_size;
//     for (pageidx i = 0; i < reserved; ++i) {
//         pages[i].free = false;
//     }
//     pages[nr_pages].free = false;
//     free_span_no_merge(reserved, nr_pages - reserved);
//     live_cpus[cpu_id].store(true, std::memory_order_relaxed);
//     return true;
// }

// mmap_area
// allocate_anonymous_memory(optional<void*> where, size_t how_much) {
//     return mmap_anonymous(where.value_or(nullptr),
//             how_much,
//             PROT_READ | PROT_WRITE,
//             MAP_PRIVATE | (where ? MAP_FIXED : 0));
// }

// mmap_area
// allocate_hugetlbfs_memory(file_desc& fd, optional<void*> where, size_t how_much) {
//     auto pos = fd.size();
//     fd.truncate(pos + how_much);
//     auto ret = fd.map(
//             how_much,
//             PROT_READ | PROT_WRITE,
//             MAP_SHARED | MAP_POPULATE | (where ? MAP_FIXED : 0),
//             pos,
//             where.value_or(nullptr));
//     return ret;
// }

// void cpu_pages::replace_memory_backing(allocate_system_memory_fn alloc_sys_mem) {
//     // We would like to use ::mremap() to atomically replace the old anonymous
//     // memory with hugetlbfs backed memory, but mremap() does not support hugetlbfs
//     // (for no reason at all).  So we must copy the anonymous memory to some other
//     // place, map hugetlbfs in place, and copy it back, without modifying it during
//     // the operation.
//     auto bytes = nr_pages * page_size;
//     auto old_mem = mem();
//     auto relocated_old_mem = mmap_anonymous(nullptr, bytes, PROT_READ|PROT_WRITE, MAP_PRIVATE);
//     std::memcpy(relocated_old_mem.get(), old_mem, bytes);
//     alloc_sys_mem({old_mem}, bytes).release();
//     std::memcpy(old_mem, relocated_old_mem.get(), bytes);
// }

// void cpu_pages::init_virt_to_phys_map() {
//     auto nr_entries = nr_pages / (huge_page_size / page_size);
//     virt_to_phys_map.resize(nr_entries);
//     auto fd = file_desc::open("/proc/self/pagemap", O_RDONLY | O_CLOEXEC);
//     for (size_t i = 0; i != nr_entries; ++i) {
//         uint64_t entry = 0;
//         auto phys = std::numeric_limits<physical_address>::max();
//         auto pfn = reinterpret_cast<uintptr_t>(mem() + i * huge_page_size) / page_size;
//         fd.pread(&entry, 8, pfn * 8);
//         assert(entry & 0x8000'0000'0000'0000);
//         phys = (entry & 0x007f'ffff'ffff'ffff) << page_bits;
//         virt_to_phys_map[i] = phys;
//     }
// }

// translation
// cpu_pages::translate(const void* addr, size_t size) {
//     auto a = reinterpret_cast<uintptr_t>(addr) - reinterpret_cast<uintptr_t>(mem());
//     auto pfn = a / huge_page_size;
//     if (pfn >= virt_to_phys_map.size()) {
//         return {};
//     }
//     auto phys = virt_to_phys_map[pfn];
//     if (phys == std::numeric_limits<physical_address>::max()) {
//         return {};
//     }
//     auto translation_size = align_up(a + 1, huge_page_size) - a;
//     size = std::min(size, translation_size);
//     phys += a & (huge_page_size - 1);
//     return translation{phys, size};
// }

// void cpu_pages::do_resize(size_t new_size, allocate_system_memory_fn alloc_sys_mem) {
//     auto new_pages = new_size / page_size;
//     if (new_pages <= nr_pages) {
//         return;
//     }
//     auto old_size = nr_pages * page_size;
//     auto mmap_start = memory + old_size;
//     auto mmap_size = new_size - old_size;
//     auto mem = alloc_sys_mem({mmap_start}, mmap_size);
//     mem.release();
//     ::madvise(mmap_start, mmap_size, MADV_HUGEPAGE);
//     // one past last page structure is a sentinel
//     auto new_page_array_pages = align_up(sizeof(page[new_pages + 1]), page_size) / page_size;
//     auto new_page_array
//         = reinterpret_cast<page*>(allocate_large(new_page_array_pages));
//     if (!new_page_array) {
//         throw std::bad_alloc();
//     }
//     std::copy(pages, pages + nr_pages, new_page_array);
//     // mark new one-past-last page as taken to avoid boundary conditions
//     new_page_array[new_pages].free = false;
//     auto old_pages = reinterpret_cast<char*>(pages);
//     auto old_nr_pages = nr_pages;
//     auto old_pages_size = align_up(sizeof(page[nr_pages + 1]), page_size);
//     pages = new_page_array;
//     nr_pages = new_pages;
//     auto old_pages_start = (old_pages - memory) / page_size;
//     if (old_pages_start == 0) {
//         // keep page 0 allocated
//         old_pages_start = 1;
//         old_pages_size -= page_size;
//     }
//     free_span(old_pages_start, old_pages_size / page_size);
//     free_span(old_nr_pages, new_pages - old_nr_pages);
// }

// void cpu_pages::resize(size_t new_size, allocate_system_memory_fn alloc_memory) {
//     new_size = align_down(new_size, huge_page_size);
//     while (nr_pages * page_size < new_size) {
//         // don't reallocate all at once, since there might not
//         // be enough free memory available to relocate the pages array
//         auto tmp_size = std::min(new_size, 4 * nr_pages * page_size);
//         do_resize(tmp_size, alloc_memory);
//     }
// }

// reclaiming_result cpu_pages::run_reclaimers(reclaimer_scope scope) {
//     auto target = std::max(nr_free_pages + 1, min_free_pages);
//     reclaiming_result result = reclaiming_result::reclaimed_nothing;
//     while (nr_free_pages < target) {
//         bool made_progress = false;
//         ++g_reclaims;
//         for (auto&& r : reclaimers) {
//             if (r->scope() >= scope) {
//                 made_progress |= r->do_reclaim() == reclaiming_result::reclaimed_something;
//             }
//         }
//         if (!made_progress) {
//             return result;
//         }
//         result = reclaiming_result::reclaimed_something;
//     }
//     return result;
// }

// void cpu_pages::schedule_reclaim() {
//     current_min_free_pages = 0;
//     reclaim_hook([this] {
//         if (nr_free_pages < min_free_pages) {
//             try {
//                 run_reclaimers(reclaimer_scope::async);
//             } catch (...) {
//                 current_min_free_pages = min_free_pages;
//                 throw;
//             }
//         }
//         current_min_free_pages = min_free_pages;
//     });
// }

// memory::memory_layout cpu_pages::memory_layout() {
//     assert(is_initialized());
//     return {
//         reinterpret_cast<uintptr_t>(memory),
//         reinterpret_cast<uintptr_t>(memory) + nr_pages * page_size
//     };
// }

// void cpu_pages::set_reclaim_hook(std::function<void (std::function<void ()>)> hook) {
//     reclaim_hook = hook;
//     current_min_free_pages = min_free_pages;
// }

// void cpu_pages::set_min_free_pages(size_t pages) {
//     if (pages > std::numeric_limits<decltype(min_free_pages)>::max()) {
//         throw std::runtime_error("Number of pages too large");
//     }
//     min_free_pages = pages;
//     maybe_reclaim();
// }

// small_pool::small_pool(unsigned object_size) noexcept
//     : _object_size(object_size), _span_size(1) {
//     while (_object_size > span_bytes()
//             || (_span_size < 32 && waste() > 0.05)
//             || (span_bytes() / object_size < 32)) {
//         _span_size *= 2;
//     }
//     _max_free = std::max<unsigned>(100, span_bytes() * 2 / _object_size);
//     _min_free = _max_free / 2;
// }

// small_pool::~small_pool() {
//     _min_free = _max_free = 0;
//     trim_free_list();
// }

// // Should not throw in case of running out of memory to avoid infinite recursion,
// // becaue throwing std::bad_alloc requires allocation. __cxa_allocate_exception
// // falls back to the emergency pool in case malloc() returns nullptr.
// void*
// small_pool::allocate() {
//     if (!_free) {
//         add_more_objects();
//     }
//     if (!_free) {
//         return nullptr;
//     }
//     auto* obj = _free;
//     _free = _free->next;
//     --_free_count;
//     return obj;
// }

// void
// small_pool::deallocate(void* object) {
//     auto o = reinterpret_cast<free_object*>(object);
//     o->next = _free;
//     _free = o;
//     ++_free_count;
//     if (_free_count >= _max_free) {
//         trim_free_list();
//     }
// }

// void
// small_pool::add_more_objects() {
//     auto goal = (_min_free + _max_free) / 2;
//     while (!_span_list.empty() && _free_count < goal) {
//         page& span = _span_list.front(cpu_mem.pages);
//         _span_list.pop_front(cpu_mem.pages);
//         while (span.freelist) {
//             auto obj = span.freelist;
//             span.freelist = span.freelist->next;
//             obj->next = _free;
//             _free = obj;
//             ++_free_count;
//             ++span.nr_small_alloc;
//         }
//     }
//     while (_free_count < goal) {
//         disable_backtrace_temporarily dbt;
//         auto data = reinterpret_cast<char*>(cpu_mem.allocate_large(_span_size));
//         if (!data) {
//             return;
//         }
//         ++_spans_in_use;
//         auto span = cpu_mem.to_page(data);
//         for (unsigned i = 0; i < _span_size; ++i) {
//             span[i].offset_in_span = i;
//             span[i].pool = this;
//         }
//         span->nr_small_alloc = 0;
//         span->freelist = nullptr;
//         for (unsigned offset = 0; offset <= span_bytes() - _object_size; offset += _object_size) {
//             auto h = reinterpret_cast<free_object*>(data + offset);
//             h->next = _free;
//             _free = h;
//             ++_free_count;
//             ++span->nr_small_alloc;
//         }
//     }
// }

// void
// small_pool::trim_free_list() {
//     auto goal = (_min_free + _max_free) / 2;
//     while (_free && _free_count > goal) {
//         auto obj = _free;
//         _free = _free->next;
//         --_free_count;
//         page* span = cpu_mem.to_page(obj);
//         span -= span->offset_in_span;
//         if (!span->freelist) {
//             new (&span->link) page_list_link();
//             _span_list.push_front(cpu_mem.pages, *span);
//         }
//         obj->next = span->freelist;
//         span->freelist = obj;
//         if (--span->nr_small_alloc == 0) {
//             _span_list.erase(cpu_mem.pages, *span);
//             cpu_mem.free_span(span - cpu_mem.pages, span->span_size);
//             --_spans_in_use;
//         }
//     }
// }

// float small_pool::waste() {
//     return (span_bytes() % _object_size) / (1.0 * span_bytes());
// }

// void
// abort_on_underflow(size_t size) {
//     if (std::make_signed_t<size_t>(size) < 0) {
//         // probably a logic error, stop hard
//         abort();
//     }
// }

// void* allocate_large(size_t size) {
//     abort_on_underflow(size);
//     unsigned size_in_pages = (size + page_size - 1) >> page_bits;
//     std::cout<<"size:   "<<size<<"  size_in pages:  "<<size_in_pages<<std::endl;
//     if ((size_t(size_in_pages) << page_bits) < size) {
//         std::cout<<"allocate_large overflow"<<std::endl;
//         throw std::bad_alloc();
//     }
//     return cpu_mem.allocate_large(size_in_pages);

// }

// void* allocate_large_aligned(size_t align, size_t size) {
//     abort_on_underflow(size);
//     unsigned size_in_pages = (size + page_size - 1) >> page_bits;
//     unsigned align_in_pages = std::max(align, page_size) >> page_bits;
//     return cpu_mem.allocate_large_aligned(align_in_pages, size_in_pages);
// }

// void free_large(void* ptr) {
//     return cpu_mem.free_large(ptr);
// }

// size_t object_size(void* ptr) {
//     return cpu_pages::all_cpus[object_cpu_id(ptr)]->object_size(ptr);
// }

// void* allocate(size_t size) {
//     if (size <= sizeof(free_object)) {
//         size = sizeof(free_object);
//     }
//     void* ptr;
//     if (size <= max_small_allocation) {
//         size = object_size_with_alloc_site(size);
//         ptr = cpu_mem.allocate_small(size);
//     } else {
//         std::cout<<"huge alloc"<<std::endl;
//         ptr = allocate_large(size);
//     }
//     if (!ptr) {
//         on_allocation_failure(size);
//     }
//     ++g_allocs;
//     return ptr;
// }

// void* allocate_aligned(size_t align, size_t size) {
//     size = std::max(size, align);
//     if (size <= sizeof(free_object)) {
//         size = sizeof(free_object);
//     }
//     void* ptr;
//     if (size <= max_small_allocation && align <= page_size) {
//         // Our small allocator only guarantees alignment for power-of-two
//         // allocations which are not larger than a page.
//         size = 1 << log2ceil(object_size_with_alloc_site(size));
//         ptr = cpu_mem.allocate_small(size);
//     } else {
//         ptr = allocate_large_aligned(align, size);
//     }
//     if (!ptr) {
//         on_allocation_failure(size);
//     }
//     ++g_allocs;
//     return ptr;
// }

// void free(void* obj) {
//     if (cpu_mem.try_cross_cpu_free(obj)) {
//         return;
//     }
//     ++g_frees;
//     cpu_mem.free(obj);
// }

// void free(void* obj, size_t size) {
//     if (cpu_mem.try_cross_cpu_free(obj)) {
//         return;
//     }
//     ++g_frees;
//     cpu_mem.free(obj, size);
// }

// void shrink(void* obj, size_t new_size) {
//     ++g_frees;
//     ++g_allocs; // keep them balanced
//     cpu_mem.shrink(obj, new_size);
// }

// void set_reclaim_hook(std::function<void (std::function<void ()>)> hook) {
//     cpu_mem.set_reclaim_hook(hook);
// }

// reclaimer::reclaimer(reclaim_fn reclaim, reclaimer_scope scope)
//     : _reclaim(std::move(reclaim))
//     , _scope(scope) {
//     cpu_mem.reclaimers.push_back(this);
// }

// reclaimer::~reclaimer() {
//     auto& r = cpu_mem.reclaimers;
//     r.erase(std::find(r.begin(), r.end(), this));
// }

// void configure(std::vector<resource::memory> m,
//         optional<std::string> hugetlbfs_path) {
//     size_t total = 0;
//     for (auto&& x : m) {
//         total += x.bytes;
//     }
//     allocate_system_memory_fn sys_alloc = allocate_anonymous_memory;
//     if (hugetlbfs_path) {
//         // std::function is copyable, but file_desc is not, so we must use
//         // a shared_ptr to allow sys_alloc to be copied around
//         auto fdp = make_lw_shared<file_desc>(file_desc::temporary(*hugetlbfs_path));
//         sys_alloc = [fdp] (optional<void*> where, size_t how_much) {
//             return allocate_hugetlbfs_memory(*fdp, where, how_much);
//         };
//         cpu_mem.replace_memory_backing(sys_alloc);
//     }
//     cpu_mem.resize(total, sys_alloc);
//     size_t pos = 0;
//     for (auto&& x : m) {
// #ifdef HAVE_NUMA
//         unsigned long nodemask = 1UL << x.nodeid;
//         auto r = ::mbind(cpu_mem.mem() + pos, x.bytes,
//                         MPOL_PREFERRED,
//                         &nodemask, std::numeric_limits<unsigned long>::digits,
//                         MPOL_MF_MOVE);

//         if (r == -1) {
//             char err[1000] = {};
//             strerror_r(errno, err, sizeof(err));
//             std::cerr << "WARNING: unable to mbind shard memory; performance may suffer: "
//                     << err << std::endl;
//         }
// #endif
//         pos += x.bytes;
//     }
//     if (hugetlbfs_path) {
//         cpu_mem.init_virt_to_phys_map();
//     }
// }

// statistics stats() {
//     return statistics{g_allocs, g_frees, g_cross_cpu_frees,
//         cpu_mem.nr_pages * page_size, cpu_mem.nr_free_pages * page_size, g_reclaims};
// }

// bool drain_cross_cpu_freelist() {
//     return cpu_mem.drain_cross_cpu_freelist();
// }

// translation
// translate(const void* addr, size_t size) {
//     auto cpu_id = object_cpu_id(addr);
//     if (cpu_id >= max_cpus) {
//         return {};
//     }
//     auto cp = cpu_pages::all_cpus[cpu_id];
//     if (!cp) {
//         return {};
//     }
//     return cp->translate(addr, size);
// }

// memory_layout get_memory_layout() {
//     return cpu_mem.memory_layout();
// }

// size_t min_free_memory() {
//     return cpu_mem.min_free_pages * page_size;
// }

// void set_min_free_pages(size_t pages) {
//     cpu_mem.set_min_free_pages(pages);
// }

// } // namespace memory




// #endif // DEFAULT_ALLOCATOR
