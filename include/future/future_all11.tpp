pollable_fd
reactor::posix_listen(socket_address sa, listen_options opts) {
    file_desc fd = file_desc::socket(sa.u.sa.sa_family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, int(opts.proto));
    if (opts.reuse_address) {
        fd.setsockopt(SOL_SOCKET, SO_REUSEADDR, 1);
    }
    if (_reuseport)
        fd.setsockopt(SOL_SOCKET, SO_REUSEPORT, 1);

    fd.bind(sa.u.sa, sizeof(sa.u.sas));
    fd.listen(100);
    return pollable_fd(std::move(fd));
}


lw_shared_ptr<pollable_fd>
reactor::make_pollable_fd(socket_address sa, transport proto) {
    file_desc fd = file_desc::socket(sa.u.sa.sa_family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, int(proto));
    return make_lw_shared<pollable_fd>(pollable_fd(std::move(fd)));
}

future<>
reactor::posix_connect(lw_shared_ptr<pollable_fd> pfd, socket_address sa, socket_address local) {
    pfd->get_file_desc().bind(local.u.sa, sizeof(sa.u.sas));
    pfd->get_file_desc().connect(sa.u.sa, sizeof(sa.u.sas));
    return pfd->writeable().then([pfd]() mutable {
        auto err = pfd->get_file_desc().getsockopt<int>(SOL_SOCKET, SO_ERROR);
        if (err != 0) {
            throw std::system_error(err, std::system_category());
        }
        return make_ready_future<>();
    });
}

server_socket
reactor::listen(socket_address sa, listen_options opt) {
    return server_socket(_network_stack->listen(sa, opt));
}

future<connected_socket>
reactor::connect(socket_address sa) {
    return _network_stack->connect(sa);
}

future<connected_socket>
reactor::connect(socket_address sa, socket_address local, transport proto) {
    return _network_stack->connect(sa, local, proto);
}




inline
future<pollable_fd, socket_address>
reactor::accept(pollable_fd_state& listenfd) {
    return readable(listenfd).then([&listenfd] () mutable {
        socket_address sa;
        socklen_t sl = sizeof(&sa.u.sas);
        file_desc fd = listenfd.fd.accept(sa.u.sa, sl, SOCK_NONBLOCK | SOCK_CLOEXEC);
        pollable_fd pfd(std::move(fd), pollable_fd::speculation(EPOLLOUT));
        return make_ready_future<pollable_fd, socket_address>(std::move(pfd), std::move(sa));
    });
}

inline
future<size_t>
reactor::read_some(pollable_fd_state& fd, void* buffer, size_t len) {
    return readable(fd).then([this, &fd, buffer, len] () mutable {
        auto r = fd.fd.read(buffer, len);
        if (!r) {
            return read_some(fd, buffer, len);
        }
        if (size_t(*r) == len) {
            fd.speculate_epoll(EPOLLIN);
        }
        return make_ready_future<size_t>(*r);
    });
}

inline
future<size_t>
reactor::read_some(pollable_fd_state& fd, const std::vector<iovec>& iov) {
    return readable(fd).then([this, &fd, iov = iov] () mutable {
        ::msghdr mh = {};
        mh.msg_iov = &iov[0];
        mh.msg_iovlen = iov.size();
        auto r = fd.fd.recvmsg(&mh, 0);
        if (!r) {
            return read_some(fd, iov);
        }
        if (size_t(*r) == iovec_len(iov)) {
            fd.speculate_epoll(EPOLLIN);
        }
        return make_ready_future<size_t>(*r);
    });
}

inline
future<size_t>
reactor::write_some(pollable_fd_state& fd, const void* buffer, size_t len) {
    return writeable(fd).then([this, &fd, buffer, len] () mutable {
        auto r = fd.fd.send(buffer, len, MSG_NOSIGNAL);
        if (!r) {
            return write_some(fd, buffer, len);
        }
        if (size_t(*r) == len) {
            fd.speculate_epoll(EPOLLOUT);
        }
        return make_ready_future<size_t>(*r);
    });
}

inline
future<>
reactor::write_all_part(pollable_fd_state& fd, const void* buffer, size_t len, size_t completed) {
    if (completed == len) {
        return make_ready_future<>();
    } else {
        return write_some(fd, static_cast<const char*>(buffer) + completed, len - completed).then(
                [&fd, buffer, len, completed, this] (size_t part) mutable {
            return write_all_part(fd, buffer, len, completed + part);
        });
    }
}

inline
future<>
reactor::write_all(pollable_fd_state& fd, const void* buffer, size_t len) {
    assert(len);
    return write_all_part(fd, buffer, len, 0);
}


template <transport Transport>
future<connected_socket, socket_address> posix_ap_server_socket_impl<Transport>::accept() {
    auto conni = get_conn_q().find(_sa.as_posix_sockaddr_in());
    if (conni != get_conn_q().end()) {
        connection c = std::move(conni->second);
        get_conn_q().erase(conni);
        try {
            std::unique_ptr<connected_socket_impl> csi(
                    new posix_connected_socket_impl<Transport>(make_lw_shared(std::move(c.fd))));
            return make_ready_future<connected_socket, socket_address>(connected_socket(std::move(csi)), std::move(c.addr));
        } catch (...) {
            return make_exception_future<connected_socket, socket_address>(std::current_exception());
        }
    } else {
        try {
            auto i = get_sockets().emplace(std::piecewise_construct, std::make_tuple(_sa.as_posix_sockaddr_in()), std::make_tuple());
            assert(i.second);
            return i.first->second.get_future();
        } catch (...) {
            return make_exception_future<connected_socket, socket_address>(std::current_exception());
        }
    }
}

template <transport Transport>
void
posix_server_socket_impl<Transport>::abort_accept() {
    _lfd.abort_reader(std::make_exception_ptr(std::system_error(ECONNABORTED, std::system_category())));
}


template <transport Transport>
void
posix_ap_server_socket_impl<Transport>::abort_accept() {
    get_conn_q().erase(_sa.as_posix_sockaddr_in());
    auto i = get_sockets().find(_sa.as_posix_sockaddr_in());
    if (i != get_sockets().end()) {
        i->second.set_exception(std::system_error(ECONNABORTED, std::system_category()));
        get_sockets().erase(i);
    }
}



template <transport Transport>
future<connected_socket, socket_address>
posix_reuseport_server_socket_impl<Transport>::accept() {
    return _lfd.accept().then([] (pollable_fd fd, socket_address sa) {
        std::unique_ptr<connected_socket_impl> csi(
                new posix_connected_socket_impl<Transport>(make_lw_shared(std::move(fd))));
        return make_ready_future<connected_socket, socket_address>(
            connected_socket(std::move(csi)), sa);
    });
}




template <transport Transport>
void
posix_reuseport_server_socket_impl<Transport>::abort_accept() {
    _lfd.abort_reader(std::make_exception_ptr(std::system_error(ECONNABORTED, std::system_category())));
}




template <transport Transport>
void  posix_ap_server_socket_impl<Transport>::move_connected_socket(socket_address sa, pollable_fd fd, socket_address addr) {
    auto i = get_sockets().find(sa.as_posix_sockaddr_in());
    if (i != get_sockets().end()) {
        try {
            std::unique_ptr<connected_socket_impl> csi(new posix_connected_socket_impl<Transport>(make_lw_shared(std::move(fd))));
            i->second.set_value(connected_socket(std::move(csi)), std::move(addr));
        } catch (...) {
            i->second.set_exception(std::current_exception());
        }
        get_sockets().erase(i);
    } else {
        get_conn_q().emplace(std::piecewise_construct, std::make_tuple(sa.as_posix_sockaddr_in()), std::make_tuple(std::move(fd), std::move(addr)));
    }
}



future<temporary_buffer<char>>
posix_data_source_impl::get() {
    return _fd->read_some(_buf.get_write(), _buf_size).then([this] (size_t size) {
        _buf.trim(size);
        auto ret = std::move(_buf);
        _buf = temporary_buffer<char>(_buf_size);
        return make_ready_future<temporary_buffer<char>>(std::move(ret));
    });
}

future<> posix_data_source_impl::close() {
    _fd->shutdown(SHUT_RD);
    return make_ready_future<>();
}
template <typename T, typename E, typename EnableFunc>
void reactor::complete_timers(T& timers, E& expired_timers, EnableFunc&& enable_fn) {
    expired_timers = timers.expire(timers.now()); // 获取过期的定时器
    std::cout << "Expired " << expired_timers.size() << " timers" << std::endl;
    // 处理所有过期定时器
    for (auto* timer_ptr : expired_timers) {
        if (timer_ptr) {
            // std::cout << "Marking timer " << " as expired" << std::endl;
            timer_ptr->_expired = true;
        }
    }
    // arm表示定时器是否有一个过期时间.
    while (!expired_timers.empty()) {
        auto* timer_ptr = expired_timers.front();
        expired_timers.pop_front();
        if (timer_ptr) {
            // std::cout << "Processing timer "<<std::endl;
            timer_ptr->_queued = false;
            if (timer_ptr->_armed) {
                timer_ptr->_armed = false;
                if (timer_ptr->_period) {
                    // std::cout << "Re-adding periodic timer " << timer_ptr->_timerid << std::endl;
                    timer_ptr->readd_periodic();//周期定时器

                }
                try {
                    // std::cout << "Executing timer callback for " << std::endl;
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

template<typename Func>
future<> smp::invoke_on_all(Func&& func) {
        static_assert(std::is_same<future<>, typename futurize<std::result_of_t<Func()>>::type>::value, "bad Func signature");
        return parallel_for_each(all_cpus(), [&func] (unsigned id) {
            return smp::submit_to(id, Func(func));
        });
}
template <typename... T>
inline
void
stream<T...>::close() {
    _done.set_value();
}

template <typename... T>
template <typename E>
inline
void
stream<T...>::set_exception(E ex) {
    _done.set_exception(ex);
}

template <typename... T>
inline
subscription<T...>::subscription(stream<T...>* s)
        : _stream(s) {
    assert(!_stream->_sub);
    _stream->_sub = this;
}

template <typename... T>
inline
void
subscription<T...>::start(std::function<future<> (T...)> next) {
    _next = std::move(next);
    _stream->_ready.set_value();
}

template <typename... T>
inline
subscription<T...>::~subscription() {
    if (_stream) {
        _stream->_sub = nullptr;
    }
}

template <typename... T>
inline
subscription<T...>::subscription(subscription&& x)
    : _stream(x._stream), _next(std::move(x._next)) {
    x._stream = nullptr;
    if (_stream) {
        _stream->_sub = this;
    }
}

template <typename... T>
inline
future<>
subscription<T...>::done() {
    return _stream->_done.get_future();
}

template <typename... T>
inline
stream<T...>::~stream() {
    if (_sub) {
        _sub->_stream = nullptr;
    }
}

template <typename... T>
inline
subscription<T...>
stream<T...>::listen() {
    return subscription<T...>(this);
}

template <typename... T>
inline
subscription<T...>
stream<T...>::listen(next_fn next) {
    auto sub = subscription<T...>(this);
    sub.start(std::move(next));
    return sub;
}

template <typename... T>
inline
future<>
stream<T...>::started() {
    return _ready.get_future();
}

template <typename... T>
inline
future<>
stream<T...>::produce(T... data) {
    auto ret = futurize<void>::apply(_sub->_next, std::move(data)...);
    if (ret.available() && !ret.failed()) {
        // Native network stack depends on stream::produce() returning
        // a ready future to push packets along without dropping.  As
        // a temporary workaround, special case a ready, unfailed future
        // and return it immediately, so that then_wrapped(), below,
        // doesn't convert a ready future to an unready one.
        return ret;
    }
    return ret.then_wrapped([this] (auto&& f) {
        try {
            f.get();
        } catch (...) {
            _done.set_exception(std::current_exception());
            // FIXME: tell the producer to stop producing
            throw;
        }
    });
}