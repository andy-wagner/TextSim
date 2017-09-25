#ifndef TEXTSIM_BIT_VECTOR_HPP
#define TEXTSIM_BIT_VECTOR_HPP

#include "common.hpp"
#include "tool.hpp"

namespace textsim{
class bit_vector_handler{

 #define WORDSIZE 64

 private:
  // First element of data records num_of_bits
  std::vector<uint64_t> &data;
  uint64_t _buffer;
  uint64_t _num_of_buffered_bits; // Should only record remaining bits not aligned to WORDSIZE

 public:

  bit_vector_handler(std::vector<uint64_t> &intermediate):data(intermediate){
    // Valid data container should at least have two eles.
    if(this->data.size() < 2){
      this->data.push_back(0);
      this->_buffer = 0;
      this->_num_of_buffered_bits = 0;
    }else{
      // DO not forget, when buffer is full, this operation will lead to 0.
      this->_num_of_buffered_bits = this->data[0] % WORDSIZE == 0 ? WORDSIZE : this->data[0] % WORDSIZE;
      this->_buffer = this->data.back();
      this->data.pop_back();
    }
    std::cout<<"init check buf"<<std::endl;
    this->look_buf();
  }

  ~bit_vector_handler(){
    std::cout<<"deconstructor check buf"<<std::endl;
    this->look_buf();
    if(this->_num_of_buffered_bits != 0){
      this->flush();
    }
    this->trim();
  };

  inline uint64_t read_bits(size_t len){
    ASSERT(len <= this->data[0],"cannot read bits more than storage");
    this->data[0] -= len;
    if(len > _num_of_buffered_bits){
      size_t res = 0;
      size_t num_lowbits = _num_of_buffered_bits;
      size_t num_highbits = len - num_lowbits;
      res |= (this->_buffer >> (WORDSIZE - this->_num_of_buffered_bits));
      this->_buffer = this->data.back();
      this->_num_of_buffered_bits = WORDSIZE - num_highbits;
      return res | ((((1 << num_highbits) - 1) & this->_buffer) << num_lowbits);
    }else{
      this->_num_of_buffered_bits -= len;
      size_t mask = (1 << len) - 1;
      return (this->_buffer >> (WORDSIZE - this->_num_of_buffered_bits - len)) & mask;
    }
  }

  inline bool read_bit(){
    ASSERT(0 != this->data[0],"empty storage!");
    --this->data[0];              // Decrease all bits
    if(this->_num_of_buffered_bits == 0 || this->_num_of_buffered_bits > 64){
      this->_buffer = this->data.back();
      this->data.pop_back();
      this->_num_of_buffered_bits = WORDSIZE - 1;
      return this->_buffer & 1;
    }else{
      return (this->_buffer) & (1ul << (WORDSIZE - this->_num_of_buffered_bits--));
    }
  }

  inline uint64_t read_bits_reverse(){

  }

  inline bool is_empty(){
    return this->data[0] == 0;
  }

  template <bool bit_content>
  inline void write_bit(){
    // bit size increment
    ++this->data[0];
    // If current buffer is full
    if(_num_of_buffered_bits == WORDSIZE){
      this->flush();
      this->_num_of_buffered_bits = 1;
      this->_buffer |= ((uint64_t)bit_content << (WORDSIZE - 1));
    }else{
      this->_buffer |= ((uint64_t)bit_content << (WORDSIZE - 1  - this->_num_of_buffered_bits));
      ++this->_num_of_buffered_bits;
    }
  }

  inline void write_bits(uint64_t val,size_t num_of_bits){
    // Cannot handle bit width larger than buffer size at one operation.
    ASSERT(num_of_bits <= 64,"unable to handle data more than 64 bits at once");
    this->data[0] += num_of_bits;
    // If buffer will be full
    if(num_of_bits > WORDSIZE - this->_num_of_buffered_bits){
      size_t num_highbits = WORDSIZE - this->_num_of_buffered_bits;
      size_t num_lowbits = num_of_bits - num_highbits;
      this->_buffer |= val >> num_lowbits;
      this->flush();
      size_t mask = (1 << num_lowbits) - 1;
      this->_buffer |= ((val & mask)<<(WORDSIZE - num_lowbits));
      this->_num_of_buffered_bits += num_lowbits ;
    } else {
      this->_buffer |= val << (WORDSIZE - this->_num_of_buffered_bits - num_of_bits);
      this->_num_of_buffered_bits += num_of_bits;
    }
  };

  inline void write_bits_reverse(uint64_t,size_t num_of_bits){
    // TODO: add reverse write
  }

 private:
  inline void flush(){
    this->data.push_back(this->_buffer);
    this->_buffer = 0ul;
    this->_num_of_buffered_bits = 0;
  }

  inline void read_buffer(){
    if(this->_num_of_buffered_bits != 0){
      this->_buffer = (this->data.back() >> (WORDSIZE - _num_of_buffered_bits));
      this->data.pop_back();
    }
  }

 public:
  inline void trim(){
    this->data.shrink_to_fit();
  }

  inline void clear(){
    this->data.resize(1);
    this->data[0] = 0;
    this->_num_of_buffered_bits = 0;
  }

 public:
  template <size_t offset>
  inline void look_data(){
    ASSERT(offset < this->data.size(),"required offset larger than data size");
    std::cout<<"now you have "<<this->data[0]<<" \tbits,\t"<<this->_num_of_buffered_bits<<" buffered bits"<<std::endl;
    std::bitset<64> demo(this->data[offset]);
    std::cout<<"mem["<<offset<<"]:\t"<<demo<<std::endl;
  }

  inline void look_buf(){
    std::cout<<"buf bits:\t"<<this->_num_of_buffered_bits<<std::endl;
//    textsim::logger::show_uint64t_binary(this->_buffer,"buf:\t");
    std::cout<<"buf:\t";
    size_t demo = this->_buffer >> (WORDSIZE - this->_num_of_buffered_bits);
    std::vector<bool> temp;
    size_t highbitsnum = this->_num_of_buffered_bits;
    while(highbitsnum > 0 && highbitsnum < 64){
      temp.push_back(demo & 1);
      demo >>= 1;
      highbitsnum--;
    }
      for (size_t i = temp.size() - 1; i <= 64; i--) {
        std::cout << temp[i];
      }
    std::cout<<" ";

    size_t numlowbits = (WORDSIZE - this->_num_of_buffered_bits);
    size_t mask = (1 << (WORDSIZE - this->_num_of_buffered_bits)) - 1;
    size_t lowbits = mask & this->_buffer;
    std::vector<bool> temp1;
    while(numlowbits > 0 && numlowbits < 64){
      temp1.push_back(lowbits & 1);
      lowbits >>= 1;
      numlowbits--;
    }
    for(size_t i = temp1.size() - 1; i <= 64; i--){
      std::cout<<temp1[i];
    }
    std::cout<<std::endl;
  }

  inline void check_buffer(size_t expectedbuffer){
    ASSERT(expectedbuffer == this->_buffer,"buffer not equal to expectation");
  }
};
};
#endif //TEXTSIM_BIT_VECTOR_HPP