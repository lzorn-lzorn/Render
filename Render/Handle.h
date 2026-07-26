#include <atomic>
#include <memory>
#include <functional>
#include <stdexcept>

inline constexpr uint64_t InvalidHandleID = 0;
inline std::atomic<uint64_t> GlobalHandleBExtID{ 1 };

template <typename ObjectType>
class Handle 
{	
public:
    using IDType = uint64_t;

    Handle() = default;

    explicit Handle(ObjectType* obj)
        : Object(obj), ID(GlobalHandleBExtID.fetch_add(1))
    {}

    explicit Handle(std::unique_ptr<ObjectType> obj)
        : Object(std::move(obj)), ID(GlobalHandleBExtID.fetch_add(1))
    {}

    Handle(Handle&& other) noexcept
        : Object(std::move(other.Object)), ID(other.ID)
    {
        other.ID = InvalidHandleID;   // 源 ID 置为无效
    }

    Handle& operator=(Handle&& other) noexcept {
        if (this != &other) {
            Object = std::move(other.Object);
            ID = other.ID;
            other.ID = InvalidHandleID;
        }
        return *this;
    }

    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;

    ~Handle() = default;

    IDType getID() const noexcept { return ID; }

    bool valid() const noexcept {
        return Object != nullptr && ID != InvalidHandleID;
    }

    explicit operator bool() const noexcept {
        return valid();
    }

    ObjectType* get() noexcept { return Object.get(); }
    const ObjectType* get() const noexcept { return Object.get(); }

	ObjectType* operator->() noexcept { return Object.get(); }
	const ObjectType* operator->() const noexcept { return Object.get(); }	
private:
    std::unique_ptr<ObjectType> Object;
    IDType ID = InvalidHandleID;
};