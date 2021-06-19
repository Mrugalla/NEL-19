#pragma once
#include <JuceHeader.h>

/*
* pointers to arbitrary underlying objects
*/
struct VectorAnything {
    VectorAnything() :
        data()
    {}
    template<typename T>
    void add(const T&& args) { data.push_back(std::make_shared<T>(args)); }
    template<typename T>
    T* get(const int idx) const noexcept {
        auto newShredPtr = data[idx];
        return static_cast<T*>(newShredPtr.get());
    }
    const size_t size() const noexcept { return data.size(); }
    void dbgRefCount() const noexcept {
        juce::String str;
        for (const auto& d : data) str += juce::String(d.use_count()) + ", ";
        DBG(str);
    }
protected:
    std::vector<std::shared_ptr<void>> data;
    /*
    * try to rewrite this with different smart pointers and/or std::any some time
    * or with juce::Var?
    */
};

/*
* releasePool for any object
*/
struct ReleasePool :
    public juce::Timer
{
    ReleasePool() :
        pool(),
        mutex()
    {}
    template<typename T>
    void add(const std::shared_ptr<T>& ptr) {
        const juce::ScopedLock lock(mutex);
        if (ptr == nullptr) return;
        for (const auto& p : pool) if (p.get() == ptr.get()) return;
        pool.emplace_back(ptr);
        if (!isTimerRunning())
            startTimer(10000);
    }
    template<typename T>
    void remove(const std::shared_ptr<T>& ptr) {
        for (auto p = 0; p < pool.size(); ++p)
            if (pool[p] == ptr) {
                pool.erase(pool.begin() + p);
                return;
            }
    }
    void timerCallback() override { release(); }
    void release() {
        const juce::ScopedLock lock(mutex);
        pool.erase(
            std::remove_if(
                pool.begin(), pool.end(), [](auto& object) { return object.use_count() < 2; }),
            pool.end());
    }
    void dbg() const {
        juce::String str("RP Size: ");
        str += juce::String(pool.size()) + " :: ";
        for (const auto& p : pool)
            str += juce::String(p.use_count()) + ", ";
        DBG(str);
    }

    static ReleasePool theReleasePool;
private:
    std::vector<std::shared_ptr<void>> pool;
    juce::CriticalSection mutex;
};

/*
* reference counted pointer to an object that uses a static release pool
* to ensure thread- and realtime-safety
*/
template<class Type>
struct ThreadSafePtr {
    ThreadSafePtr(const Type&& args) :
        curPtr(std::make_shared<Type>(args)),
        updatedPtr(curPtr),
        spinLock()
    { ReleasePool::theReleasePool.add(curPtr); }
    ~ThreadSafePtr() {
        curPtr.reset(); updatedPtr.reset();
        ReleasePool::theReleasePool.release();
    }
    std::shared_ptr<Type> getCopyOfUpdatedPtr() {
        spinLock.enter();
        auto copiedPtr = std::make_shared<Type>(*updatedPtr.get());
        spinLock.exit();
        return copiedPtr;
    }
    void replaceUpdatedPtrWith(const std::shared_ptr<Type>& newPtr) {
        spinLock.enter();
        updatedPtr = newPtr;
        ReleasePool::theReleasePool.add(updatedPtr);
        spinLock.exit();
    }
    std::shared_ptr<Type> getUpdatedPtr() noexcept {
        return updatedPtr;
    }
    std::shared_ptr<Type> updateAndLoadCurrentPtr() noexcept {
        if (curPtr != updatedPtr)
            if (spinLock.tryEnter()) {
                curPtr = updatedPtr;
                spinLock.exit();
            }
        return curPtr;
    }
    const std::shared_ptr<Type>& operator->() const noexcept { return curPtr; }
    void dbgReferenceCount(juce::String start = "") const noexcept {
        DBG(start);
        ReleasePool::theReleasePool.dbg();
        DBG("current: " << curPtr.use_count() << " :: " << "updated: " << updatedPtr.use_count());
        DBG("THEY ARE " << (curPtr == updatedPtr ? "THE SAME" : "DIFFERENT") << "\n");
    }
protected:
    std::shared_ptr<Type> curPtr;
    std::shared_ptr<Type> updatedPtr;
    juce::SpinLock spinLock;
};