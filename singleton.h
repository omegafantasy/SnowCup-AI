#ifndef SINGLETON_H
#define SINGLETON_H

template <typename T>
class Singleton {
protected:
    Singleton() = default;
    ~Singleton() = default;
    Singleton(const Singleton&) = delete;
    Singleton(Singleton&&) = delete;

public:
    static T& instance() {
        static T inst;
        return inst;
    }
};


#endif // SINGLETON_H