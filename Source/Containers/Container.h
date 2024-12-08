#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <type_traits>
#include <algorithm>

#include "Core/BasicTypes.h"

template <typename _Ty>
using TInitializerList = std::initializer_list<_Ty>;

template <typename _Ty1, typename _Ty2>
using TPair = std::pair<_Ty1, _Ty2>;

template <typename _Ty>
using TSet = std::unordered_set<_Ty>;

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

    inline void Clear() { Data.clear(); }

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
        Resize(OldNum + Count);
        AMemory::Memzero(GetData() + OldNum * sizeof(_Ty), Count * sizeof(_Ty));
        return Num();
    }

    inline SizeType AddUnique(const _Ty& Item) { return AddUnique_Impl(Item); }
    inline SizeType AddUnique(_Ty&& Item) { return AddUnique_Impl(std::move(Item)); }

    void RemoveAt(SizeType Index)
    {
        std::swap(*(Data.begin() + Index), *(Data.end() - 1));
        Data.pop_back();
    }

    void RemoveAt_Stable(SizeType Index) { Data.erase(Data.begin() + Index); }

    SizeType RemoveFirst(const _Ty& Item)
    {
        SizeType Index = -1;
        Find(Item, Index);
        RemoveAt(Index);
        return Index;
    }
    SizeType RemoveFirst_Stable(const _Ty& Item)
    {
        SizeType Index = -1;
        Find(Item, Index);
        RemoveAt_Stable(Index);
        return Index;
    }

    inline void Resize(SizeType NewSize) { Data.resize(NewSize); }
    inline void Resize(SizeType NewSize, const _Ty& Value) { Data.resize(NewSize, Value); }

    inline bool Find(const _Ty& Item, SizeType& Index) const { return Find_Impl<_Ty>(Item, Index); }
    inline bool Find(const _Ty& Item) const { return Find_Impl<_Ty>(Item); }

    inline void Sort() { Sort_Impl<_Ty>(); }

    // Range-for.
    using Iterator = _Ty*;
    using ConstIterator = const _Ty*;

    Iterator begin() { return GetData(); }
    Iterator end() { return GetData() + Num(); }
    ConstIterator begin() const { return GetData(); }
    ConstIterator end() const { return GetData() + Num(); }

private:
    template <typename _Vty>
    SizeType AddUnique_Impl(_Vty&& Arg)
    {
        SizeType Index = -1;
        if (Find(Arg, Index))
        {
            return Index;
        }
        return Add(std::forward<_Vty>(Arg));
    }

    template <typename _Vty>
    inline void Sort_Impl()
    {
        std::sort(Data.begin(), Data.end());
    }

    template <>
    inline void Sort_Impl<const AnsiChar*>()
    {
        std::sort(Data.begin(), Data.end(), [](const AnsiChar* Lhs, const AnsiChar* Rhs) { return std::strcmp(Lhs, Rhs) < 0; });
    }

    template <typename _Vty>
    bool Find_Impl(const _Ty& Item) const
    {
        return std::find(Data.begin(), Data.end(), Item) != Data.end();
    }

    template <typename _Vty>
    bool Find_Impl(const _Ty& Item, SizeType& Index) const
    {
        if (auto Pos = std::find(Data.begin(), Data.end(), Item); Pos != Data.end())
        {
            Index = static_cast<SizeType>(std::distance(Data.begin(), Pos));
            return true;
        }

        Index = -1;
        return false;
    }

    template <>
    bool Find_Impl<const AnsiChar*>(const _Ty& Item) const
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

    template <>
    bool Find_Impl<const AnsiChar*>(const _Ty& Item, SizeType& Index) const
    {
        for (Index = 0; Index < Num(); ++Index)
        {
            if (std::strcmp(Data[Index], Item) == 0)
            {
                return true;
            }
        }

        Index = -1;
        return false;
    }

private:
    std::vector<_Ty, _Alloc> Data;
};

template <typename _Kty, typename _Vty>
class TMap
{
    template <typename _OtherKty, typename _OtherVty>
    friend class TMap;

    using SizeType = int32_t;
    using Iterator = typename std::unordered_map<_Kty, _Vty>::iterator;
    using ConstIterator = typename std::unordered_map<_Kty, _Vty>::const_iterator;

public:
    TMap() = default;
    TMap(const TMap& Other) : Data(Other.Data) {}
    TMap(TMap&& Other) : Data(std::move(Other.Data)) {}

    TMap& operator=(const TMap& Other)
    {
        if (this != &Other)
        {
            Data = Other.Data;
        }
        return *this;
    }
    TMap& operator=(TMap&& Other) noexcept
    {
        Data = std::move(Other.Data);
        return *this;
    }

    inline _Vty& operator[](_Kty Key) { return Data[Key]; }
    inline const _Vty& operator[](_Kty Key) const { return Data[Key]; }

    inline TPair<Iterator, bool> Add(_Kty Key, _Vty Val) { return Data.emplace(Key, Val); }

    inline _Vty* Remove(_Kty Key)
    {
        _Vty* Val = Find(Key);
        if (Val)
        {
            Data.erase(Key);
        }
        return Val;
    }

    inline _Vty* Find(_Kty Key)
    {
        Iterator Iter = Data.find(Key);
        if (Iter != Data.end())
        {
            return &Iter->second;
        }
        return nullptr;
    }

    inline const _Vty* Find(_Kty Key) const
    {
        ConstIterator Iter = Data.find(Key);
        if (Iter != Data.end())
        {
            return &Iter->second;
        }
        return nullptr;
    }

private:
    std::unordered_map<_Kty, _Vty> Data;
};
