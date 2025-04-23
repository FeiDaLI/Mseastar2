// #pragma once

// #include <chrono>
// #include <functional>

// #include "../util/shared_ptr.hh"
// #include "future_all4.hh"


// /// \file

// /// Returns a future which completes after a specified time has elapsed.
// ///
// /// \param dur minimum amount of time before the returned future becomes
// ///            ready.
// /// \return A \ref future which becomes ready when the sleep duration elapses.
// template <typename Clock = steady_clock_type, typename Rep, typename Period>
// future<> sleep(std::chrono::duration<Rep, Period> dur) {
//     struct sleeper {
//         promise<> done;
//         timer<Clock> tmr;
//         sleeper(std::chrono::duration<Rep, Period> dur)
//             : tmr([this] { done.set_value(); })
//         {
//             tmr.arm(dur);
//         }
//     };
//     sleeper *s = new sleeper(dur);
//     future<> fut = s->done.get_future();
//     return fut.then([s] { delete s; });
// }

// /// exception that is thrown when application is in process of been stopped
// class sleep_aborted : public std::exception {
// public:
//     /// Reports the exception reason.
//     virtual const char* what() const noexcept {
//         return "Sleep is aborted";
//     }
// };

// /// Returns a future which completes after a specified time has elapsed
// /// or throws \ref sleep_aborted exception if application is aborted
// ///
// /// \param dur minimum amount of time before the returned future becomes
// ///            ready.
// /// \return A \ref future which becomes ready when the sleep duration elapses.
// template <typename Rep, typename Period>
// future<> sleep_abortable(std::chrono::duration<Rep, Period> dur) {
//     return engine().wait_for_stop(dur).then([] {
//         throw sleep_aborted();
//     }).handle_exception([] (std::exception_ptr ep) {
//         try {
//             std::rethrow_exception(ep);
//         } catch(condition_variable_timed_out&) {};
//     });
// }
