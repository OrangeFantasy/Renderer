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

template <typename _Ty, typename _AllocTy = std::allocator<_Ty>>
class TArray
{
    template <typename _OtherTy, typename _OtherAllocTy>
    friend class TArray;

    using _SzTy = int32_t;
    using _Iterator = typename std::vector<_Ty, _AllocTy>::iterator;
    using _ConstIterator = typename std::vector<_Ty, _AllocTy>::const_iterator;

public:
    TArray() : Data(std::vector<_Ty, _AllocTy>(0)) { }
    TArray(_SzTy Count) : Data(std::vector<_Ty, _AllocTy>(Count)) { }
    TArray(TInitializerList<_Ty> InitList) : Data(std::vector<_Ty, _AllocTy>(InitList)) { }

    TArray(const TArray& Other) : Data(Other.Data) { }
    TArray(TArray&& Other) noexcept : Data(std::move(Other.Data)) { }

    TArray(_Ty* First, _Ty* Last) : Data(std::vector<_Ty, _AllocTy>(First, Last)) { }
    TArray(_Ty* First, _SzTy Count) : Data(std::vector<_Ty, _AllocTy>(First, First + Count)) { }

    template <typename _Vty = _Ty, typename = std::enable_if_t<std::is_constructible_v<_Ty, _Vty>>>
    explicit TArray(const TArray<_Vty, _AllocTy>& Other) : Data(Other.Data.begin(), Other.Data.end()) { }

    virtual ~TArray() { }

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

    template <typename _Vty = _Ty, typename = std::enable_if_t<std::is_constructible_v<_Ty, _Vty>>>
    TArray& operator=(const TArray<_Vty, _AllocTy>& Other)
    {
        Data = std::vector<_Ty, _AllocTy>(Other.Data.begin(), Other.Data.end());
        return *this;
    }

    inline _Ty& operator[](_SzTy Index) { return Data.at(Index); }
    inline const _Ty& operator[](_SzTy Index) const { return Data.at(Index); }

public:
    inline _Ty* GetData() noexcept { return Data.data(); }
    inline const _Ty* GetData() const noexcept { return Data.data(); }

    inline _SzTy Num() const { return static_cast<_SzTy>(Data.size()); }
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

    template <typename _Vty = _Ty, typename = std::enable_if_t<std::is_constructible_v<_Ty, _Vty>>>
    _SzTy Add(_Vty&& Item)
    {
        Data.push_back(std::forward<_Vty>(Item));
        return Num();
    }

    template <typename _Vty = _Ty, typename = std::enable_if_t<std::is_constructible_v<_Ty, _Vty>>>
    _SzTy AddUnique(_Vty&& Item)
    {
        _SzTy Index = -1;
        if (Find(Item, Index))
        {
            return Index;
        }
        return Add(std::forward<_Vty>(Item));
    }

    _SzTy AddZeroed(_SzTy Count = 1)
    {
        const _SzTy OldNum = Num();
        Resize(OldNum + Count);
        AMemory::Memzero(GetData() + OldNum * sizeof(_Ty), Count * sizeof(_Ty));
        return Num();
    }

    void RemoveAt(_SzTy Index)
    {
        std::swap(*(Data.begin() + Index), *(Data.end() - 1));
        Data.pop_back();
    }

    _SzTy RemoveFirstOf(const _Ty& Item)
    {
        _SzTy Index = -1;
        Find(Item, Index);
        RemoveAt(Index);
        return Index;
    }

    inline void Resize(_SzTy NewSize) { Data.resize(NewSize); }
    inline void Resize(_SzTy NewSize, const _Ty& Value) { Data.resize(NewSize, Value); }

    inline void Sort()
    {
        if constexpr (std::is_same_v<_Ty, const AnsiChar*>)
        {
            std::sort(Data.begin(), Data.end(), 
                [](const char* Lhs, const char* Rhs) { return std::strcmp(Lhs, Rhs) < 0; });
        }
        else
        {
            std::sort(First, Last);
        }
    }

    inline bool Find(const _Ty& Item) const
    {
        return Find(Data.begin(), Data.end(), Item) != Data.end();
    }

    inline bool Find(const _Ty& Item, _SzTy& Index) const
    {
        _ConstIterator Iter = Find(Data.begin(), Data.end(), Item);
        Index = static_cast<_SzTy>((Iter != Data.end()) ? std::distance(Data.begin(), Iter) : -1);
        return Iter != Data.end();
    }

    // Range-for.
    _Iterator begin() { return Data.begin(); }
    _Iterator end() { return Data.end(); }
    _ConstIterator begin() const { return Data.begin(); }
    _ConstIterator end() const { return Data.end(); }

private:
    template <typename _It, typename _Vty>
    static _ConstIterator Find(const _It First, const _It Last, const _Vty& Value)
    {
        if constexpr (std::is_same_v<_Vty, const AnsiChar*>)
        {
            return std::find_if(First, Last, 
                [&Value](const AnsiChar* Str) { return std::strcmp(Str, Value) == 0; });
        }
        else
        {
            return std::find(First, Last, Value);
        }
    }

private:
    std::vector<_Ty, _AllocTy> Data;
};

template <typename _Kty, typename _Vty>
class TMap
{
    template <typename _OtherKty, typename _OtherVty>
    friend class TMap;

    using _SzTy = int32_t;
    using _Iterator = typename std::unordered_map<_Kty, _Vty>::iterator;
    using _ConstIterator = typename std::unordered_map<_Kty, _Vty>::const_iterator;

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

    inline void Clear() { Data.clear(); }

    inline TPair<_Iterator, bool> Add(_Kty Key, _Vty Val) { return Data.emplace(Key, Val); }

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
        _Iterator Iter = Data.find(Key);
        if (Iter != Data.end())
        {
            return &Iter->second;
        }
        return nullptr;
    }

    inline const _Vty* Find(_Kty Key) const
    {
        _ConstIterator Iter = Data.find(Key);
        if (Iter != Data.end())
        {
            return &Iter->second;
        }
        return nullptr;
    }

    // Range-for.
    _Iterator begin() { return Data.begin(); }
    _Iterator end() { return Data.end(); }
    _ConstIterator begin() const { return Data.begin(); }
    _ConstIterator end() const { return Data.end(); }

private:
    std::unordered_map<_Kty, _Vty> Data;
};
