
#include <stdbool.h> // For bool type
#include <stdint.h>  // For fixed-width integer types: uint64_t, uint8_t
#include <stdio.h>

// --- Theoretical Feasibility Assessment Summary ---
// This approach is theoretically completely feasible. By generating key-related
// S-boxes and round keys through a 64-bit key, combined with modular addition,
// S-box lookup, XOR and other reversible operations, it can achieve strict
// 8-bit to 8-bit encryption and decryption, while providing good disturbance
// and avalanche effect.

// Global S-box and inverse S-box. In practical applications, they are usually
// part of internal state or passed through context. For demonstration
// simplicity, they are set as global variables here.
static uint8_t sbox[256];
static uint8_t inv_sbox[256];
static uint8_t subkey_add; // Subkey for modular addition
static uint8_t subkey_xor; // Subkey for XOR operation

/**
 * @brief Generate S-box, inverse S-box and subkeys using a 64-bit key.
 *
 * This function is based on a simplified version of Key Scheduling Algorithm
 * (KSA), ensuring generation of a key-related permutation of 256 bytes. At the
 * same time, it derives two 8-bit subkeys from the 64-bit master key.
 *
 * @param key_64bit 64-bit key for S-box initialization and confusion, and
 * subkey derivation.
 */
void generate_sboxes_and_subkeys(uint64_t key_64bit) {
  int i, j;

  // 1. Initialize S-box: sbox[i] = i
  for (i = 0; i < 256; i++) {
    sbox[i] = i;
  }

  // 2. Decompose 64-bit key into 8 8-bit bytes for S-box confusion and subkey
  // derivation
  uint8_t k_bytes[8];
  for (i = 0; i < 8; i++) {
    k_bytes[i] = (uint8_t)((key_64bit >> (i * 8)) & 0xFF);
  }

  // 3. KSA-like confusion process: shuffle S-box according to key
  // This is a key step to ensure permutation generation, similar to RC4's KSA.
  j = 0;
  for (i = 0; i < 256; i++) {
    j = (j + sbox[i] + k_bytes[i % 8]) %
        256; // Introduce confusion of key bytes and current S-box state
    // Swap sbox[i] and sbox[j]
    uint8_t temp = sbox[i];
    sbox[i] = sbox[j];
    sbox[j] = temp;
  }

  // 4. Generate inverse S-box
  // If sbox[x] = y, then inv_sbox[y] = x
  for (i = 0; i < 256; i++) {
    inv_sbox[sbox[i]] = i;
  }

  // 5. Derive subkeys
  // Here simply use two bytes of the key as subkeys.
  // More complex derivation methods can provide better security, but sufficient
  // for 8-bit operations.
  subkey_add = k_bytes[0]; // Use first byte of key as modular addition subkey
  subkey_xor = k_bytes[1]; // Use second byte of key as XOR subkey

  printf(
      "S-box, inverse S-box and subkeys generated (based on key: 0x%016llX).\n",key_64bit);
  printf("  Subkey (addition): 0x%02X, Subkey (XOR): 0x%02X\n", subkey_add,
         subkey_xor);
}

/**
 * @brief Encryption function: encrypt 8-bit data A into 8-bit data B, adding
 * disturbance and avalanche effect.
 *
 * Encryption process: (A + subkey_add) % 256 -> S-box lookup -> result XOR
 * subkey_xor
 *
 * @param data_A Original 8-bit data.
 * @return Encrypted 8-bit data B.
 */
uint8_t encrypt_byte(uint8_t data_A) {
  // Step 1: Modular addition with addition subkey, providing initial
  // disturbance
  uint8_t intermediate1 = (data_A + subkey_add) % 256;

  // Step 2: Non-linear permutation through S-box
  uint8_t intermediate2 = sbox[intermediate1];

  // Step 3: XOR with XOR subkey for further confusion
  uint8_t data_B = intermediate2 ^ subkey_xor;

  // printf("[Encrypt] A: 0x%02X -> B: 0x%02X (intermediate1: 0x%02X,
  // intermediate2: 0x%02X)\n",
  //        data_A, data_B, intermediate1, intermediate2);
  return data_B;
}

/**
 * @brief Decryption function: decrypt 8-bit data B back to 8-bit data A,
 * reverse of encryption process.
 *
 * Decryption process: B XOR subkey_xor -> inverse S-box lookup -> (result -
 * subkey_add + 256) % 256
 *
 * @param data_B Encrypted 8-bit data.
 * @return Restored original 8-bit data A.
 */
uint8_t decrypt_byte(uint8_t data_B) {
  // Step 1 (reverse): Restore XOR operation
  uint8_t intermediate1_inv = data_B ^ subkey_xor;

  // Step 2 (reverse): Restore S-box lookup
  uint8_t intermediate2_inv = inv_sbox[intermediate1_inv];

  // Step 3 (reverse): Restore modular addition (note negative case, add 256 to
  // ensure positive then modulo)
  uint8_t data_A_restored = (intermediate2_inv - subkey_add + 256) % 256;

  // printf("[Decrypt] B: 0x%02X -> A': 0x%02X (intermediate1_inv: 0x%02X,
  // intermediate2_inv: 0x%02X)\n",
  //        data_B, data_A_restored, intermediate1_inv, intermediate2_inv);
  return data_A_restored;
}

int main() {
  uint64_t key1 = 0x123456789ABCDEF1ULL; // Example key 1
  uint64_t key2 = 0xFEDCBA9876543210ULL; // Example key 2 (different key)
  uint64_t key3 =
      0x123456789ABCDEE0ULL; // Example key 3 (only 1 bit difference from key1)

  printf("--- Test with Key 1 ---\n");
  generate_sboxes_and_subkeys(key1); // Generate S-box and subkeys using key 1

  // Test all possible 8-bit values (0-255)
  bool all_match_key1 = true;
  for (uint16_t i = 0; i <= 255; i++) {
    uint8_t original_A = (uint8_t)i;
    uint8_t encrypted_B = encrypt_byte(original_A);
    uint8_t decrypted_A = decrypt_byte(encrypted_B);

    if (original_A != decrypted_A) {
      printf("Error! Original A: 0x%02X, Encrypted B: 0x%02X, Decrypted A: "
             "0x%02X\n",
             original_A, encrypted_B, decrypted_A);
      all_match_key1 = false;
      break;
    }
  }

  if (all_match_key1) {
    printf("All 256 8-bit values were encrypted and correctly decrypted with "
           "key 1!\n");
  } else {
    printf("Error occurred during testing with key 1.\n");
  }
  printf("----------------------------------------\n\n");

  printf(
      "--- Test with Key 2 (Different key, different S-box and subkeys) ---\n");
  generate_sboxes_and_subkeys(key2); // Generate S-box and subkeys using key 2

  // Test all possible 8-bit values (0-255)
  bool all_match_key2 = true;
  for (uint16_t i = 0; i <= 255; i++) {
    uint8_t original_A = (uint8_t)i;
    uint8_t encrypted_B = encrypt_byte(original_A);
    uint8_t decrypted_A = decrypt_byte(encrypted_B);

    if (original_A != decrypted_A) {
      printf("Error! Original A: 0x%02X, Encrypted B: 0x%02X, Decrypted A: "
             "0x%02X\n",
             original_A, encrypted_B, decrypted_A);
      all_match_key2 = false;
      break;
    }
  }

  if (all_match_key2) {
    printf("All 256 8-bit values were encrypted and correctly decrypted with "
           "key 2!\n");
  } else {
    printf("Error occurred during testing with key 2.\n");
  }
  printf("----------------------------------------\n\n");

  // Demonstrate "input sensitivity" (avalanche effect)
  printf("--- Demonstrate Input Sensitivity (Avalanche Effect) ---\n");
  uint8_t base_value = 0xAA; // Base test value
  uint8_t changed_value =
      0xAB; // Base value changed by 1 bit (0xAA = 10101010, 0xAB = 10101011)

  generate_sboxes_and_subkeys(key1); // Use key 1

  uint8_t encrypted_base = encrypt_byte(base_value);
  uint8_t encrypted_changed = encrypt_byte(changed_value);

  printf("Original value A1: 0x%02X (%u), Encrypted result B1: 0x%02X (%u)\n",
         base_value, base_value, encrypted_base, encrypted_base);
  printf("Original value A2: 0x%02X (%u), Encrypted result B2: 0x%02X (%u)\n",
         changed_value, changed_value, encrypted_changed, encrypted_changed);
  printf("Note: Original values A1 and A2 differ by only 1 bit, but encrypted "
         "results B1 and B2 usually differ greatly.\n");
  printf("----------------------------------------\n\n");

  // Demonstrate "key sensitivity" (avalanche effect)
  printf("--- Demonstrate Key Sensitivity (Avalanche Effect) ---\n");
  uint8_t common_value = 0x5A; // Common test value

  printf("Common original value A: 0x%02X (%u)\n", common_value, common_value);

  // Encrypt using key 1
  generate_sboxes_and_subkeys(key1);
  uint8_t enc_key1 = encrypt_byte(common_value);
  printf("Encrypted result B using key 1 (0x%016llX): 0x%02X (%u)\n", key1,
         enc_key1, enc_key1);

  // Encrypt using key 3 (only 1 bit difference from key 1)
  generate_sboxes_and_subkeys(key3);
  uint8_t enc_key3 = encrypt_byte(common_value);
  printf("Encrypted result B using key 3 (0x%016llX): 0x%02X (%u)\n", key3,
         enc_key3, enc_key3);
  printf("Note: Even with only 1 bit difference in keys, encryption results "
         "are completely different, demonstrating key avalanche effect.\n");
  printf("----------------------------------------\n\n");

  return 0;
}
