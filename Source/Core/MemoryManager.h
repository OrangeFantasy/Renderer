#pragma once

#include <type_traits>
#include <cstring>

struct AMemory
{
    template <typename T, typename = typename std::enable_if<!std::is_pointer<T>::value>>
    static inline void Memzero(T& Src)
    {
        std::memset(&Src, 0, sizeof(T));
    }

    template <typename T, typename = typename std::enable_if<std::is_pointer<T>::value>>
    static inline void Memzero(T* Ptr, size_t Size)
    {
        std::memset(Ptr, 0, Size);
    }

    template <typename T, typename = typename std::enable_if<!std::is_pointer<T>::value>>
    static inline void Memcpy(T& Dest, const T& Src)
    {
        std::memcpy(&Dest, &Src, sizeof(T));
    }

    static inline void* Memcpy(void* Dest, const void* Src, size_t Count) 
    {
        return std::memcpy(Dest, Src, Count);
    }

    static inline int32_t Memcmp(const void* Buf1, const void* Buf2, size_t Size)
    { 
        return static_cast<int32_t>(std::memcmp(Buf1, Buf2, Size));
    }
};

template <typename _Ty>
class TRefCountPtr
{
    using ReferenceType = _Ty*;

public:
    inline TRefCountPtr() : Ptr(nullptr), RefCount(nullptr) {}

    TRefCountPtr(_Ty* InPtr) noexcept : Ptr(InPtr), RefCount(nullptr)
    {
        if (Ptr)
        {
            RefCount = new size_t(1);
        }
    }

    TRefCountPtr(const TRefCountPtr& Copy) : Ptr(Copy.Ptr), RefCount(Copy.RefCount)
    {
        if (Ptr)
        {
            AddRefCount();
        }
    }

    template <typename CopyReferencedType>
    explicit TRefCountPtr(const TRefCountPtr<CopyReferencedType>& Copy) : Ptr(static_cast<_Ty*>(Copy.Get())), RefCount(Copy.RefCount)
    {
        if (Ptr)
        {
            AddRefCount();
        }
    }

    inline TRefCountPtr(TRefCountPtr&& Move) noexcept : Ptr(Move.Ptr), RefCount(Move.RefCount)
    {
        Move.Ptr = nullptr;
        Move.RefCount = nullptr;
    }

    template <typename MoveReferencedType>
    explicit TRefCountPtr(TRefCountPtr<MoveReferencedType>&& Move) : Ptr(static_cast<_Ty*>(Move.Get())), RefCount(Move.RefCount)
    {
        Move.Ptr = nullptr;
        Move.RefCount = nullptr;
    }

    ~TRefCountPtr()
    {
        if (Ptr)
        {
            DecreaseRefCount();
        }
    }

    TRefCountPtr& operator=(_Ty* InPtr)
    {
        if (Ptr)
        {
            DecreaseRefCount();
        }

        Ptr = InPtr;
        if (Ptr)
        {
            RefCount = new size_t(1);
        }
        return *this;
    }

    inline TRefCountPtr& operator=(const TRefCountPtr& Copy) { return *this = Copy.Ptr; }

    template <typename CopyReferencedType>
    inline TRefCountPtr& operator=(const TRefCountPtr<CopyReferencedType>& Copy)
    {
        return *this = Copy.Get();
    }

    TRefCountPtr& operator=(TRefCountPtr&& Move) noexcept
    {
        if (this != &Move)
        {
            if (Ptr)
            {
                DecreaseRefCount();
            }
            Ptr = Move.Ptr;
            RefCount = Move.RefCount;
        }
        return *this;
    }

    inline _Ty* operator->() const { return Ptr; }

    inline operator _Ty*() const { return Ptr; }

    inline _Ty* Get() const { return Ptr; }

    inline bool IsValid() const { return Ptr != nullptr; }

    size_t GetRefCount() { return *RefCount; }

    inline void Swap(TRefCountPtr& InPtr) // this does not change the reference count, and so is faster
    {
        _Ty* OldReference = Ptr;
        size_t* OldRefCount = RefCount;

        Ptr = InPtr.Ptr;
        RefCount = InPtr.RefCount;

        InPtr.Ptr = OldReference;
        InPtr.RefCount = OldRefCount;
    }

private:
    void AddRefCount() { ++(*RefCount); }
    void DecreaseRefCount()
    {
        if (--(*RefCount) == 0)
        {
            delete Ptr;
            Ptr = nullptr;
            delete RefCount;
            RefCount = nullptr;
        }
    }

private:
    _Ty* Ptr;
    size_t* RefCount;

    template <typename OtherType>
    friend class TRefCountPtr;
};

template <typename _Ty>
inline bool operator==(const TRefCountPtr<_Ty>& A, const TRefCountPtr<_Ty>& B)
{
    return A.Get() == B.Get();
}

template <typename _Ty>
inline bool operator==(const TRefCountPtr<_Ty>& A, _Ty* B)
{
    return A.Get() == B;
}

template <typename _Ty>
inline bool operator==(_Ty* A, const TRefCountPtr<_Ty>& B)
{
    return A == B.Get();
}
