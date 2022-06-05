#pragma once

#include <cstdint>

namespace core {

class System;

namespace decode {

class Decoder {
public:
  Decoder(System *system);
  ~Decoder() = default;

  void start(uint32_t inst);

  void decode_sme_encodings(uint32_t inst);
  void decode_unallocated(uint32_t inst);
  void decode_sve_encodings(uint32_t inst);

  /*
           loads/stores
  */
  void decode_loads_and_stores(uint32_t inst);
  void decode_ldst_register(uint32_t inst);
  void decode_ldst_reg_unscaled_imm(uint32_t inst);
  void decode_ldst_reg_imm_post_indexed(uint32_t inst);
  void decode_ldst_reg_unpriviledged(uint32_t inst);
  void decode_ldst_reg_imm_pre_indexed(uint32_t inst);
  void decode_ldst_atomic_memory_op(uint32_t inst);
  void decode_ldst_reg_pac(uint32_t inst);
  void decode_ldst_reg_reg_offset(uint32_t inst);

  void decode_data_processing_imm(uint32_t inst);
  void decode_data_processing_reg(uint32_t inst);
  void decode_data_processing_float(uint32_t inst);
  void decode_branches(uint32_t inst);

  /*
           Data Processing Immediate
  */
  void decode_pc_rel(uint32_t inst);
  void decode_add_sub_imm(uint32_t inst);
  void decode_add_sub_imm_with_tags(uint32_t inst);
  void decode_logical_imm(uint32_t inst);
  void decode_move_wide_imm(uint32_t inst);
  void decode_bitfield(uint32_t inst);
  void decode_extract(uint32_t inst);

private:
  System *system_;
};

typedef void (Decoder::*decode_func)(uint32_t inst);

} // namespace decode
} // namespace core
