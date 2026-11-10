#pragma once

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

#include "allocator.hpp"
#include "detail/allocated.h"
#include "detail/construct.h"
#include "Iterator.h"

#ifndef NULL
#define NULL  0
#endif

#ifndef RED
#define RED   0
#endif

#ifndef BLACK
#define BLACK 1
#endif
