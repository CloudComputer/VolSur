#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>


class DynamicBitset
{
public:
  // Default constructor (empty bitset)
  DynamicBitset()
    : bit_count(0)
  {
  }

  // Constructor with specified size (default all 0)
  explicit DynamicBitset(size_t size, bool value = false)
    : bit_count(size)
  {
    size_t blocks = blocks_needed(size);
    data.resize(blocks, value ? ~Block(0) : Block(0));

    // Handle initialization of extra bits in last block
    if (value && size > 0)
    {
      size_t last_block_bits = size % bits_per_block;
      if (last_block_bits != 0)
      {
        data.back() &= (Block(1) << last_block_bits) - 1;
      }
    }
  }

  // Set all bits to 1
  void set()
  {
    std::fill(data.begin(), data.end(), ~Block(0));
    if (bit_count > 0)
    {
      trim_last_block();
    }
  }

  // Set specified position to 1 (default) or specified value
  void set(size_t pos, bool value = true)
  {
    if (pos >= bit_count)
    {
      throw std::out_of_range("DynamicBitset::set position out of range");
    }

    size_t block_idx = pos / bits_per_block;
    size_t bit_idx = pos % bits_per_block;
    Block mask = Block(1) << bit_idx;

    if (value)
    {
      data[block_idx] |= mask;
    }
    else
    {
      data[block_idx] &= ~mask;
    }
  }

  // Reset all bits to 0
  void reset() { std::fill(data.begin(), data.end(), Block(0)); }

  // Reset specified position to 0
  void reset(size_t pos) { set(pos, false); }

  // Flip all bits
  void flip()
  {
    for (auto& block : data)
    {
      block = ~block;
    }
    trim_last_block();
  }

  // Flip specified bit
  void flip(size_t pos)
  {
    if (pos >= bit_count)
    {
      throw std::out_of_range("DynamicBitset::flip position out of range");
    }

    size_t block_idx = pos / bits_per_block;
    size_t bit_idx = pos % bits_per_block;
    Block mask = Block(1) << bit_idx;
    data[block_idx] ^= mask;
  }

  // Test value of specified bit
  bool test(size_t pos) const
  {
    if (pos >= bit_count)
    {
      throw std::out_of_range("DynamicBitset::test position out of range");
    }

    size_t block_idx = pos / bits_per_block;
    size_t bit_idx = pos % bits_per_block;
    return (data[block_idx] >> bit_idx) & 1;
  }

  // Return bitset size
  size_t size() const { return bit_count; }

  // Count number of set bits (1s)
  size_t count() const
  {
    size_t total = 0;
    for (Block block : data)
    {
      total += popcount(block);
    }
    return total;
  }

  // Resize bitset
  void resize(size_t new_size, bool value = false)
  {
    size_t old_blocks = blocks_needed(bit_count);
    size_t new_blocks = blocks_needed(new_size);

    // Handle size change
    bit_count = new_size;
    data.resize(new_blocks, value ? ~Block(0) : Block(0));

    // Handle newly grown bits
    if (new_size > bit_count && value)
    {
      if (old_blocks > 0)
      {
        size_t last_old_block_bits = bit_count % bits_per_block;
        if (last_old_block_bits > 0)
        {
          Block mask = (Block(1) << last_old_block_bits) - 1;
          data[old_blocks - 1] |= ~mask;
        }
      }
    }

    // Trim extra bits in last block
    trim_last_block();
  }

  // Bitwise AND operation
  DynamicBitset& operator&=(const DynamicBitset& other)
  {
    if (bit_count != other.bit_count)
    {
      throw std::invalid_argument("Bitsets must be same size for &=");
    }

    size_t common_blocks = blocks_needed(bit_count);
    for (size_t i = 0; i < common_blocks; ++i)
    {
      data[i] &= other.data[i];
    }
    return *this;
  }

  // Bitwise OR operation
  DynamicBitset& operator|=(const DynamicBitset& other)
  {
    if (bit_count != other.bit_count)
    {
      throw std::invalid_argument("Bitsets must be same size for |=");
    }

    size_t common_blocks = blocks_needed(bit_count);
    for (size_t i = 0; i < common_blocks; ++i)
    {
      data[i] |= other.data[i];
    }
    return *this;
  }

  // Bitwise XOR operation
  DynamicBitset& operator^=(const DynamicBitset& other)
  {
    if (bit_count != other.bit_count)
    {
      throw std::invalid_argument("Bitsets must be same size for ^=");
    }

    size_t common_blocks = blocks_needed(bit_count);
    for (size_t i = 0; i < common_blocks; ++i)
    {
      data[i] ^= other.data[i];
    }
    return *this;
  }

private:
  using Block = uint64_t;
  static constexpr size_t bits_per_block = std::numeric_limits<Block>::digits;
  std::vector<Block> data;
  size_t bit_count; // total number of bits

  // Calculate number of Blocks needed
  static size_t blocks_needed(size_t bits) { return (bits + bits_per_block - 1) / bits_per_block; }

  // Trim extra bits in last block
  void trim_last_block()
  {
    if (bit_count == 0)
      return;

    size_t last_block_bits = bit_count % bits_per_block;
    if (last_block_bits != 0)
    {
      Block mask = (Block(1) << last_block_bits) - 1;
      data.back() &= mask;
    }
  }

  static size_t popcount(Block x)
  {
    size_t count = 0;
    while (x)
    {
      count++;
      x &= x - 1; // Clear lowest bit
    }
    return count;
  }
};

// Non-member bitwise operators
DynamicBitset operator&(const DynamicBitset& lhs, const DynamicBitset& rhs)
{
  DynamicBitset result = lhs;
  result &= rhs;
  return result;
}

DynamicBitset operator|(const DynamicBitset& lhs, const DynamicBitset& rhs)
{
  DynamicBitset result = lhs;
  result |= rhs;
  return result;
}

DynamicBitset operator^(const DynamicBitset& lhs, const DynamicBitset& rhs)
{
  DynamicBitset result = lhs;
  result ^= rhs;
  return result;
}

DynamicBitset operator~(const DynamicBitset& bitset)
{
  DynamicBitset result = bitset;
  result.flip();
  return result;
}

