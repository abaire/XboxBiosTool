// Bios.h: handling An Xbox BIOS.

/* Copyright(C) 2024 tommojphillips
 *
 * This program is free software : you can redistribute it and /or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.If not, see < https://www.gnu.org/licenses/>.
 */

// Author: tommojphillips
// GitHub: https:\\github.com\tommojphillips

#ifndef XB_BIOS_H
#define XB_BIOS_H

#include <stdint.h>

// user incl
#include "Mcpx.h"
#include "bldr.h"
#include "rsa.h"
#include "sha1.h"

#define MIN_BIOS_SIZE 0x40000     // Min bios file/rom size in bytes
#define MAX_BIOS_SIZE 0x100000    // Max bios file/rom size in bytes
#define KD_DELAY_FLAG 0x80000000  // Kernel Delay Flag

#define ROM_DIGEST_SIZE 0x100  // rom digest size in bytes

#define PRELDR_BLOCK_SIZE 0x2A00  // preldr block size in bytes
#define PRELDR_PARAMS_SIZE 0x80   // preldr params size in bytes
#define PRELDR_SIZE                      \
  (PRELDR_BLOCK_SIZE - ROM_DIGEST_SIZE - \
   PRELDR_PARAMS_SIZE)          // preldr size in bytes
#define PRELDR_NONCE_SIZE 0x10  // preldr nonce size in bytes
#define PRELDR_REAL_BASE                              \
  (0xFFFFFFFF - MCPX_BLOCK_SIZE - PRELDR_BLOCK_SIZE + \
   1)  // preldr rom base address		; FFFF.D400
#define PRELDR_REAL_END \
  (PRELDR_REAL_BASE +   \
   PRELDR_SIZE)  // preldr rom end address		; FFFF.FC80

#define BLDR_BLOCK_SIZE 0x6000  // 2BL block size in bytes
#define BLDR_RELOC 0x00400000   // 2BL relocation base address
#define BLDR_BASE 0x00090000    // 2BL boot base address
#define BLDR_REAL_BASE \
  (0xFFFFFFFF - MCPX_BLOCK_SIZE - BLDR_BLOCK_SIZE + 1)  // 2BL rom base address

#define BOOT_SIGNATURE 2018801994U  // J y T x

// BIOS load status codes
#define BIOS_LOAD_STATUS_SUCCESS 0       // success; the bios is loaded.
#define BIOS_LOAD_STATUS_INVALID_BLDR 1  // success; but the bldr is invalid.
#define BIOS_LOAD_STATUS_FAILED 2        // ERROR

// Preldr status codes
#define PRELDR_STATUS_BLDR_DECRYPTED \
  0  // found and was used to load and decrypt the 2bl.
#define PRELDR_STATUS_FOUND 1  // found but was not used to load the 2bl.
#define PRELDR_STATUS_NOT_FOUND \
  2  // not found. old bios (mcpx v1.0) or not a valid bios.
#define PRELDR_STATUS_ERROR 3  // ERROR

#define PRELDR_TEA_ATTACK_ENTRY_POINT 0x007fd588

// xbox public key structure
typedef struct _XB_PUBLIC_KEY {
  RSA_HEADER header;     // rsa header structure
  uint8_t modulus[264];  // pointer to the modulus.
} XB_PUBLIC_KEY;

// Preldr structure
typedef struct {
  uint8_t* data;
  PRELDR_PARAMS* params;
  PRELDR_PTR_BLOCK* ptr_block;
  PRELDR_FUNC_BLOCK* func_block;
  XB_PUBLIC_KEY* public_key;
  uint8_t bldr_key[SHA1_DIGEST_LEN];
  uint32_t jmp_offset;
  int status;
} PRELDR;

// 2BL structure
typedef struct {
  uint8_t* data;
  BLDR_ENTRY* entry;
  uint8_t* bfm_key;
  BLDR_KEYS* keys;
  BOOT_PARAMS* boot_params;
  BOOT_LDR_PARAM* ldr_params;
  bool encryption_state;
} BLDR;

// Kernel structure
typedef struct {
  uint8_t* compressed_kernel_ptr;
  uint8_t* uncompressed_data_ptr;
  uint8_t* img;
  uint32_t img_size;
  bool encryption_state;
} KERNEL;

// Bios load parameters
typedef struct BIOS_LOAD_PARAMS {
  uint32_t romsize;
  uint8_t* bldr_key;
  uint8_t* kernel_key;
  MCPX* mcpx;
  bool enc_bldr;
  bool enc_kernel;
  bool restore_boot_params;
} BIOS_LOAD_PARAMS;

// Bios build parmeters
typedef struct BIOS_BUILD_PARAMS {
  uint8_t* init_tbl;
  uint8_t* preldr;
  uint8_t* bldr;
  uint8_t* compressed_kernel;
  uint8_t* kernel_data;
  uint8_t* eeprom_key;
  uint8_t* cert_key;
  uint32_t preldr_size;
  uint32_t bldrSize;
  uint32_t init_tbl_size;
  uint32_t kernel_size;
  uint32_t kernel_data_size;
  bool bfm;
  bool hackinittbl;
  bool hacksignature;
  bool nobootparams;
  bool zero_kernel_key;
  bool fix2bldigest;
} BIOS_BUILD_PARAMS;

// Bios
class Bios {
 public:
  uint8_t* data;
  uint32_t size;
  BLDR bldr;
  PRELDR preldr;
  KERNEL kernel;
  INIT_TBL* init_tbl;
  uint8_t* rom_digest;
  int available_space;
  int bios_status;

  BIOS_LOAD_PARAMS params;

  Bios() { resetValues(); };

  ~Bios() { unload(); };

  // unload the bios. reset values and free memory.
  void unload();

  // load bios from memory.
  int load(uint8_t* buff, const uint32_t binsize,
           const BIOS_LOAD_PARAMS* bios_params);

  // build bios.
  int build(BIOS_BUILD_PARAMS* build_params, uint32_t binsize,
            BIOS_LOAD_PARAMS* bios_params);

  // initialize bios and calculate offsets and pointers.
  int init(uint8_t* buff, const uint32_t binsize,
           const BIOS_LOAD_PARAMS* bios_params);

  // calculate initial offsets for the bios. based on params.
  // sets up bldr and preldr structs, initTbl and dataTbl pointers.
  // Should be invoked when the bios is loaded.
  void getOffsets();

  // calculate offsets and pointers. base on bios data.
  // sets up 2bl, krnl and uncompressed_kernel_data pointers.
  // Should be invoked when the 2bl is valid. (not encrypted).
  void getOffsets2();

  // validate the 2BL boot param sizes and romsize.
  int validateBldrBootParams();

  // preldr create the bldr key
  void preldrCreateKey(uint8_t* sbkey, uint8_t* key);

  // preldr symmetric encryption and decryption for the 2BL.
  void preldrSymmetricEncDecBldr(const uint8_t* key, const uint32_t len);

  // validate the preldr and decrypt the 2bl.
  // sets up the preldr struct and decrypts the 2bl.
  void preldrValidateAndDecryptBldr();

  // symmetric encryption and decryption for the 2BL.
  void symmetricEncDecBldr(const uint8_t* key, const uint32_t len);

  // symmetric encryption and decryption for the kernel.
  void symmetricEncDecKernel();

  // decompress the kernel image from the bios.
  // stores results in decompressedKrnl pointer and decompressedKrnlSize.
  // returns 0 if successful,
  int decompressKrnl();

  // preldr decrypt preldr public key.
  int preldrDecryptPublicKey();

 private:
  // reset bios; reset values.
  void resetValues();
};

void bios_init_preldr(PRELDR* preldr);
void bios_init_bldr(BLDR* bldr);
void bios_init_kernel(KERNEL* kernel);
void bios_init_params(BIOS_LOAD_PARAMS* params);
void bios_init_build_params(BIOS_BUILD_PARAMS* params);
void bios_free_build_params(BIOS_BUILD_PARAMS* params);
int bios_check_size(const uint32_t size);
int bios_replicate_data(uint32_t from, uint32_t to, uint8_t* buffer,
                        uint32_t buffersize);

#endif  // !XB_BIOS_H
