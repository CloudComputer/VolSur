#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>


class DynamicBitset
{
public:
  // 默认构造函数（空 bitset）
  DynamicBitset()
    : bit_count(0)
  {
  }

  // 指定大小的构造函数（默认全0）
  explicit DynamicBitset(size_t size, bool value = false)
    : bit_count(size)
  {
    size_t blocks = blocks_needed(size);
    data.resize(blocks, value ? ~Block(0) : Block(0));

    // 处理最后一块中多余位的初始化
    if (value && size > 0)
    {
      size_t last_block_bits = size % bits_per_block;
      if (last_block_bits != 0)
      {
        data.back() &= (Block(1) << last_block_bits) - 1;
      }
    }
  }

  // 设置所有位为1
  void set()
  {
    std::fill(data.begin(), data.end(), ~Block(0));
    if (bit_count > 0)
    {
      trim_last_block();
    }
  }

  // 设置指定位置为1（默认）或指定值
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

  // 重置所有位为0
  void reset() { std::fill(data.begin(), data.end(), Block(0)); }

  // 重置指定位置为0
  void reset(size_t pos) { set(pos, false); }

  // 翻转所有位
  void flip()
  {
    for (auto& block : data)
    {
      block = ~block;
    }
    trim_last_block();
  }

  // 翻转指定位
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

  // 测试指定位的值
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

  // 返回bitset大小
  size_t size() const { return bit_count; }

  // 计算设置位（1）的数量
  size_t count() const
  {
    size_t total = 0;
    for (Block block : data)
    {
      total += popcount(block);
    }
    return total;
  }

  // 调整bitset大小
  void resize(size_t new_size, bool value = false)
  {
    size_t old_blocks = blocks_needed(bit_count);
    size_t new_blocks = blocks_needed(new_size);

    // 处理大小变化
    bit_count = new_size;
    data.resize(new_blocks, value ? ~Block(0) : Block(0));

    // 处理新增长的位
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

    // 清理最后一块的多余位
    trim_last_block();
  }

  // 按位与操作
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

  // 按位或操作
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

  // 按位异或操作
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
  size_t bit_count; // 位总数

  // 计算需要的Block数量
  static size_t blocks_needed(size_t bits) { return (bits + bits_per_block - 1) / bits_per_block; }

  // 清理最后一块中多余的位
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
      x &= x - 1; // 清除最低位
    }
    return count;
  }
};

// 非成员位操作函数
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

