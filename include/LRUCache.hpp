#include <thread>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <type_traits>

#if defined(__clang__) || defined(__GNUC__)
#define CPP_STANDARD __cplusplus
#elif defined(_MSC_VER)
#define CPP_STANDARD _MSVC_LANG
#endif

namespace std {
#if CPP_STANDARD <= 201703L
    template< class T >
    struct remove_cvref {
        typedef std::remove_cv_t<std::remove_reference_t<T>> type;
    };

    template< class T >
    using remove_cvref_t = typename remove_cvref<T>::type;
#endif
}

// STRUCT TEMPLATE is_smart_pointer
template<typename T>
struct is_smart_pointer_helper : public std::false_type {};

template<typename T>
struct is_smart_pointer_helper<std::shared_ptr<T>> : public std::true_type {};

template<typename T>
struct is_smart_pointer_helper<std::shared_ptr<const T>> : public std::true_type {};

template<typename T>
struct is_smart_pointer_helper<std::unique_ptr<T>> : public std::true_type {};

template<typename T>
struct is_smart_pointer_helper<std::unique_ptr<const T>> : public std::true_type {};

template<typename T>
struct is_smart_pointer_helper<std::weak_ptr<T>> : public std::true_type {};

template<typename T>
struct is_smart_pointer_helper<std::weak_ptr<const T>> : public std::true_type {};

template<typename T>
struct is_smart_pointer : public is_smart_pointer_helper<T> {};

template<typename T>
constexpr bool is_smart_pointer_v = typename is_smart_pointer<T>::value;

// STRUCT TEMPLATE remove_smart_pointer
template<typename _Ty>
struct remove_smart_pointer {
    using type = _Ty;
    using _Const_thru_ref_type = const _Ty;
};

template<typename _Ty>
struct remove_smart_pointer<std::shared_ptr<_Ty>> {
    using type = _Ty;
    using _Const_thru_ref_type = const _Ty&;
};

template<typename _Ty>
struct remove_smart_pointer<std::unique_ptr<_Ty>> {
    using type = _Ty;
    using _Const_thru_ref_type = const _Ty&;
};

template<typename _Ty>
struct remove_smart_pointer<std::weak_ptr<_Ty>> {
    using type = _Ty;
    using _Const_thru_ref_type = const _Ty&;
};

template<typename _Ty>
using remove_smart_pointer_t = typename remove_smart_pointer<_Ty>::type;

// STRUCT TEMPLATE remove_cvrefptr
template<typename _Ty>
struct remove_cvrefptr {
    using type = remove_smart_pointer_t<
        std::remove_pointer_t<
        std::remove_cvref_t<_Ty>>>;
};

template<typename _Ty>
using remove_cvrefptr_t = typename remove_cvrefptr<_Ty>::type;

// MACRO HAS_MEMBER_FUNC(member)
#define HAS_MEMBER_FUNC(member) \
template<typename T, typename... Args> \
struct has_member_func_##member \
{ \
    private: \
        template<typename U> static auto Check(int) -> decltype(std::declval<remove_cvrefptr_t<U>>().member(std::declval<Args>()...), std::true_type()); \
        template<typename U> static std::false_type Check(...); \
    public: \
        static constexpr bool value = std::is_same<decltype(Check<T>(0)), std::true_type>::value; \
};\
template<typename T, typename... Args> \
constexpr bool has_member_func_##member##_v = has_member_func_##member<T, Args...>::value;

// MACRO HAS_MEMBER_VAR(member)
#define HAS_MEMBER_VAR(member) \
template<typename T> \
struct has_member_var_##member \
{ \
    private: \
        template<typename U> static auto Check(int) -> decltype(std::declval<remove_cvrefptr_t<U>>().member, std::true_type()); \
        template<typename U> static std::false_type Check(...);\
    public: \
        static constexpr bool value = std::is_same<decltype(Check<T>(0)), std::true_type>::value; \
};\
template<typename T> \
constexpr bool has_member_var_##member##_v = has_member_var_##member<T>::value;

///////////////////////////////////////////////////////////////////////////////////////////////////
// the datastruct of least recently unit
template<typename T, typename U, typename Hasher = std::hash<T>>
class LRUCache_ {
public:
    struct Node;
    using MapType = std::unordered_map<T, Node*, Hasher>;

    struct Node {
        Node(U data) : data(data), prev(nullptr), next(nullptr) {}

        Node() : prev(nullptr), next(nullptr) {}

        U data;
        Node* prev;
        Node* next;
        typename MapType::iterator map_it;
    };

    inline explicit LRUCache_(int max_size) noexcept : maxSize_(max_size), size_(0)
    {
        listHead_ = new Node();
        listTail_ = new Node();
        listHead_->next = listTail_;
        listTail_->prev = listHead_;
    }

    inline ~LRUCache_() noexcept
    {
        EvictAll();
        delete listHead_;
        delete listTail_;
        listHead_ = nullptr;
        listTail_ = nullptr;
    }

    LRUCache_(const LRUCache_&) = delete;

    void operator=(const LRUCache_&) = delete;

    bool Put(T key, U value)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        size_t newDataSize = GetMemorySize(value);
        if (newDataSize > maxSize_) {
            return false;
        }

        if (!hashMap_.count(key)) {
            Node* newNode = new Node(value);
            auto it = hashMap_.insert(typename MapType::value_type(key, newNode));
            if (it.second == false) {
                delete newNode;
                return false;
            }
            newNode->map_it = it.first;

            AddToHead(newNode);
            size_ += newDataSize;

            while (size_ > maxSize_) {
                Node* removed = RemoveTail();
                size_t removedDataSize = GetMemorySize(removed->data);

                hashMap_.erase(removed->map_it);
                delete removed;
                size_ -= removedDataSize;
            }
        }
        else {
            Node* node = hashMap_[key];
            node->data = value;
            MoveToHead(node);
        }

        return true;
    }

    std::pair<U, bool> Get(T key)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = hashMap_.find(key);
        if (it == hashMap_.end()) {
            return std::pair<U, bool>(U(), false);
        }

        Node* node = it->second;

        MoveToHead(node);

        return std::pair<U, bool>(node->data, true);
    }

    int Size() const
    {
        return size_;
    }

    int MaxSize() const
    {
        return maxSize_;
    }

protected:
    void AddToHead(Node* node)
    {
        node->prev = listHead_;
        node->next = listHead_->next;
        listHead_->next->prev = node;
        listHead_->next = node;
    }

    void RemoveNode(Node* node)
    {
        node->prev->next = node->next;
        node->next->prev = node->prev;
    }

    void MoveToHead(Node* node)
    {
        RemoveNode(node);
        AddToHead(node);
    }

    Node* RemoveTail()
    {
        Node* node = listTail_->prev;
        RemoveNode(node);
        return node;
    }

    size_t GetMemorySize(const U& data) const
    {
        if constexpr (has_member_func_GetMemorySize_v<U> && (is_smart_pointer<U>::value || std::is_pointer_v<U>)) {
            return data->GetMemorySize();
        }
        else if constexpr (has_member_func_GetMemorySize_v<U>) {
            return data.GetMemorySize();
        }
        else {
            return 1;
        }
    }

    void EvictAll() noexcept
    {
        Node* cur = listHead_->next;
        while (cur != listTail_) {
            hashMap_.erase(cur->map_it);
            Node* save = cur;
            cur = cur->next;
            delete save;
        }
        size_ = 0;
    }

private:
    MapType hashMap_;
    Node* listHead_;
    Node* listTail_;
    int size_;
    int maxSize_;
    std::mutex mutex_;
};