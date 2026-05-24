/*
 * Copyright (C) 2026 Dahus
 * SPDX-License-Identifier: MIT
 */
#pragma once
#define NOMINMAX

#include <cstdint>

 // Integer types
using Int8 = std::int8_t;
using Int16 = std::int16_t;
using Int32 = std::int32_t;
using Int64 = std::int64_t;
using UInt8 = std::uint8_t;
using UInt16 = std::uint16_t;
using UInt32 = std::uint32_t;
using UInt64 = std::uint64_t;
using IntPtr = std::intptr_t;
using UIntPtr = std::uintptr_t;

#include <memory>
using std::unique_ptr;
using std::shared_ptr;
using std::weak_ptr;
using std::make_shared;

#include <string>
using std::string;

#include <vector>
using std::vector;
typedef std::vector<UInt8> ByteVector;

#include <map>
using std::map;

#include <unordered_map>
using std::unordered_map;

#include <deque>
using std::deque;

#include <queue>
using std::queue;

#include <list>
using std::list;

#include <array>
using std::array;
using std::begin;
using std::end;

#include <algorithm>
using std::for_each;
