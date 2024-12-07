#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <type_traits>
#include <algorithm>

#include "Core/MemoryManager.h"
#include "Core/CoreTypes.h"

template <typename _Ty>
using TInitializerList = std::initializer_list<_Ty>;

template <typename _Kty, typename Ty>
using TMap = std::unordered_map<_Kty, Ty>;

template <typename _Ty>
using TSet = std::unordered_set<_Ty>;

#if TARRAY_RANGED_FOR_CHECKS
template <typename _Ty, typename _STy>
struct TCheckedPointerIterator
{
    explicit TCheckedPointerIterator(_Ty* InPtr, _STy InNum) : Ptr(InPtr), InitialNum(InNum), CurrentNum(InNum) {}

    _Ty& operator*() const { return *Ptr; }

    TCheckedPointerIterator& operator++()
    {
        ++Ptr;
        return *this;
    }

private:
    _Ty* Ptr;
    const _STy CurrentNum;
    _STy InitialNum;

    friend bool operator!=(const TCheckedPointerIterator& Lhs, const TCheckedPointerIterator& Rhs)
    {
        if (Lhs.CurrentNum != Lhs.InitialNum)
        {
            std::cerr << "Array has changed during ranged-for iteration! \n";
        }
        return Lhs.Ptr != Rhs.Ptr;
    }
};
#endif

template <typename _Ty, typename _Alloc = std::allocator<_Ty>>
class TArray
{
    template <typename _OtherTy, typename _Alloc>
    friend class TArray;

    using SizeType = int32_t;

public:
    TArray() : Data(std::vector<_Ty, _Alloc>(0)) {}
    TArray(SizeType Count) : Data(std::vector<_Ty, _Alloc>(Count)) {}
    TArray(TInitializerList<_Ty> InitList) : Data(std::vector<_Ty, _Alloc>(InitList)) {}

    TArray(const TArray& Other) : Data(Other.Data) {}
    TArray(TArray&& Other) noexcept : Data(std::move(Other.Data)) {}

    TArray(_Ty* First, _Ty* Last) : Data(std::vector<_Ty, _Alloc>(First, Last)) {}
    TArray(_Ty* First, SizeType Count) : Data(std::vector<_Ty, _Alloc>(First, First + Count)) {}

    template <typename Other_Ty, typename _Alloc, std::enable_if_t<std::is_constructible<_Ty, Other_Ty>::value>* = nullptr>
    explicit TArray(const TArray<Other_Ty, _Alloc>& Other) : Data(Other.Data.begin(), Other.Data.end())
    {
    }

    virtual ~TArray() {}

    TArray& operator=(TInitializerList<_Ty> InitList)
    {
        Data = InitList;
        return *this;
    }

    TArray& operator=(const TArray& Other)
    {
        if (this != &Other)
        {
            Data = Other.Data;
        }
        return *this;
    }

    TArray& operator=(TArray&& Other) noexcept
    {
        Data = std::move(Other.Data);
        return *this;
    }

    template <typename _OtherTy, typename _Alloc, std::enable_if_t<std::is_constructible<_Ty, _OtherTy>::value>* = nullptr>
    TArray& operator=(const TArray<_OtherTy, _Alloc>& Other)
    {
        Data = std::vector<_Ty, _Alloc>(Other.Data.begin(), Other.Data.end());
        return *this;
    }

    inline _Ty& operator[](SizeType Index) { return Data[Index]; }
    inline const _Ty& operator[](SizeType Index) const { return Data[Index]; }

    inline _Ty* GetData() noexcept { return Data.data(); }
    inline const _Ty* GetData() const noexcept { return Data.data(); }

    inline SizeType Num() const { return static_cast<SizeType>(Data.size()); }

    inline bool IsEmpty() const { return Data.empty(); }

    bool Contains(const _Ty& Item)
    {
        for (const _Ty* __restrict Data = GetData(), * __restrict End = Data + Num(); Data != End; ++Data)
        {
            if (*Data == Item)
            {
                return true;
            }
        }
        return false;
    }

    SizeType Add(const _Ty& Item)
    {
        Data.push_back(Item);
        return Num();
    }
    SizeType Add(_Ty&& Item)
    {
        Data.push_back(std::move(Item));
        return Num();
    }

    template <typename _OtherTy, typename = std::enable_if_t<std::is_constructible<_Ty, _OtherTy>::value>::type>
    SizeType Add(const _OtherTy& Item)
    {
        Data.push_back(Item);
        return Num();
    }

    template <typename _OtherTy, typename = std::enable_if_t<std::is_constructible<_Ty, _OtherTy>::value>::type>
    SizeType Add(_OtherTy&& Item)
    {
        Data.push_back(std::move(Item));
        return Num();
    }

    SizeType AddZeroed(SizeType Count = 1)
    {
        const SizeType OldNum = Num();
        Resize(Num() + Count);
        AMemory::Memzero(GetData() + OldNum * sizeof(_Ty), Count * sizeof(_Ty));
        return OldNum;
    }

    SizeType AddUnique(const _Ty& Item) { return AddUniqueImplement(Item); }

    SizeType AddUnique(_Ty&& Item) { return AddUniqueImplement(std::move(Item)); }

    void RemoveAt(SizeType Index) { Data.erase(Data.begin() + Index); }

    void RemoveAtSwap(SizeType Index)
    {
        std::swap(*(Data.begin() + Index), *(Data.end() - 1));
        Data.pop_back();
    }

    SizeType RemoveSingle(const _Ty& Item)
    {
        SizeType Index = -1;
        Find(Item, Index);
        RemoveAt(Index);
        return Index;
    }

    SizeType RemoveSingleSwap(const _Ty& Item)
    {
        SizeType Index = -1;
        Find(Item, Index);
        RemoveAtSwap(Index);
        return Index;
    }

    inline void Resize(SizeType NewSize) { Data.resize(NewSize); }
    inline void Resize(SizeType NewSize, const _Ty& Value) { Data.resize(NewSize, Value); }

    inline void Empty(SizeType NewSize)
    {
        Clear();
        Resize(NewSize);
    }

    inline void Clear() { Data.clear(); }

    // For string with type of const AnsiChar*.
    bool Find(const _Ty& Item, SizeType& Index) const
    {
        if constexpr (std::is_same<_Ty, const AnsiChar*>::value)
        {
            for (Index = 0; Index < Num(); ++Index)
            {
                if (std::strcmp(Data[Index], Item) == 0)
                {
                    return true;
                }
            }
        }
        else
        {
            if (auto Pos = std::find(Data.begin(), Data.end(), Item); Pos != Data.end())
            {
                Index = static_cast<SizeType>(std::distance(Data.begin(), Pos));
                return true;
            }
        }

        Index = -1;
        return false;
    }

    bool Find(const _Ty& Item) const
    {
        if constexpr (std::is_same<_Ty, const AnsiChar*>::value)
        {
            for (int32_t Index = 0; Index < Num(); ++Index)
            {
                if (std::strcmp(Data[Index], Item) == 0)
                {
                    return true;
                }
            }
            return false;
        }
        else
        {
            return std::find(Data.begin(), Data.end(), Item) != Data.end();
        }
    }

    inline void Sort()
    {
        if constexpr (std::is_same<_Ty, const AnsiChar*>::value)
        {
            std::sort(Data.begin(), Data.end(), [](const _Ty& Lhs, const _Ty& Rhs) { return std::strcmp(Lhs, Rhs) < 0; });
        }
        else
        {
            std::sort(Data.begin(), Data.end());
        }
    }

    // Range-for.
#if TARRAY_RANGED_FOR_CHECKS
    using IteratorType = TCheckedPointerIterator<_Ty, SizeType>;
    using ConstIteratorType = TCheckedPointerIterator<const _Ty, SizeType>;
#else
    using IteratorType = _Ty*;
    using ConstIteratorType = const _Ty*;
#endif

#if TARRAY_RANGED_FOR_CHECKS
    IteratorType begin() { return IteratorType(GetData(), Num()); }
    IteratorType end() { return IteratorType(GetData() + Num(), Num()); }
    ConstIteratorType begin() const { return ConstIteratorType(GetData(), Num()); }
    ConstIteratorType end() const { return ConstIteratorType(GetData() + Num(), Num()); }
#else
    IteratorType begin() { return GetData(); }
    IteratorType end() { return GetData() + Num(); }
    ConstIteratorType begin() const { return GetData(); }
    ConstIteratorType end() const { return GetData() + Num(); }
#endif

private:
    template <typename _ArgTy>
    SizeType AddUniqueImplement(_ArgTy&& Arg)
    {
        SizeType Index = -1;
        if (Find(Arg, Index))
        {
            return Index;
        }
        return Add(std::forward<_ArgTy>(Arg));
    }

private:
    std::vector<_Ty, _Alloc> Data;
};
