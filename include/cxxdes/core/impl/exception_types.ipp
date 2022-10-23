struct interrupted_exception: std::exception {
    interrupted_exception(std::string what = "interrupted."):
        what_{std::move(what)} {
    }

    const char *what() const noexcept override {
        return what_.c_str();
    }

private:
    std::string what_;
};

struct stopped_exception: std::exception {
    const char *what() const noexcept override {
        return "stopped.";
    }
};
