#ifndef CXX_DES_SIMULATION_HPP_INCLUDED
#define CXX_DES_SIMULATION_HPP_INCLUDED

namespace cxx_des {

template <typename T>
concept startable = requires(T t, environment env) {
    { t.start(env) };
};

template <typename Derived>
struct simulation {
    environment env;

    template <startable S>
    auto &start(S &&s) {
        return s.start(env);
    }

    template <typename T, typename ...Args>
    auto create(Args && ...args) {
        return T{env, std::forward<Args>(args) ...};
    }

    void start_main() {
        ((Derived &) *this).co_main().start(env);
    }

    auto now() const {
        return env.now();
    }

    void run() {
        start_main();
        while (env.step()) ;
    }

    void run_until(time_type t) {
        start_main();
        while (now() <= t && env.step());
    }

    void run_for(time_type t) {
        run_until(now() + t);
    }
};

#define CXX_DES_SIMULATION(name) struct name : simulation < name >

}

#endif /* CXX_DES_SIMULATION_HPP_INCLUDED */
