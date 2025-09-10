#pragma once

#include "network_types.hpp"
#include <vector>
#include <memory>
#include <string>
#include <functional>
#include <unordered_map>
#include <array>

namespace ecscope::networking {

/**
 * @brief Encryption Algorithm Types
 */
enum class EncryptionAlgorithm : uint8_t {
    NONE = 0,
    AES_128_GCM = 1,
    AES_256_GCM = 2,
    CHACHA20_POLY1305 = 3,
    CUSTOM = 255
};

/**
 * @brief Key Exchange Method
 */
enum class KeyExchangeMethod : uint8_t {
    PRE_SHARED_KEY = 0,
    ECDH_X25519 = 1,
    ECDH_P256 = 2,
    RSA_2048 = 3,
    CUSTOM = 255
};

/**
 * @brief Encryption Statistics
 */
struct EncryptionStats {
    uint64_t bytes_encrypted = 0;
    uint64_t bytes_decrypted = 0;
    uint64_t encryption_operations = 0;
    uint64_t decryption_operations = 0;
    uint64_t key_exchanges = 0;
    uint64_t authentication_failures = 0;
    double average_encryption_time_us = 0.0;
    double average_decryption_time_us = 0.0;
    double encryption_throughput_mbps = 0.0;
    double decryption_throughput_mbps = 0.0;
    
    void update_throughput() {
        if (encryption_operations > 0 && average_encryption_time_us > 0) {
            double bytes_per_op = static_cast<double>(bytes_encrypted) / encryption_operations;
            encryption_throughput_mbps = (bytes_per_op / average_encryption_time_us) * 0.953674; // Convert to Mbps
        }
        if (decryption_operations > 0 && average_decryption_time_us > 0) {
            double bytes_per_op = static_cast<double>(bytes_decrypted) / decryption_operations;
            decryption_throughput_mbps = (bytes_per_op / average_decryption_time_us) * 0.953674; // Convert to Mbps
        }
    }
};

/**
 * @brief Cryptographic Key
 */
class CryptographicKey {
public:
    enum Type : uint8_t {
        SYMMETRIC = 0,
        PUBLIC = 1,
        PRIVATE = 2
    };
    
    CryptographicKey(Type type, std::vector<uint8_t> key_data);
    ~CryptographicKey();
    
    // Key access (const only for security)
    const uint8_t* data() const { return key_data_.data(); }
    size_t size() const { return key_data_.size(); }
    Type get_type() const { return type_; }
    
    // Key operations
    std::vector<uint8_t> derive_key(const std::vector<uint8_t>& context, size_t key_length) const;
    std::array<uint8_t, 32> get_key_hash() const;
    
    // Security
    void zero_key(); // Securely clear key data
    bool is_valid() const { return !key_data_.empty() && !zeroed_; }

private:
    Type type_;
    std::vector<uint8_t> key_data_;
    bool zeroed_ = false;
};

/**
 * @brief Base Encryptor Interface
 */
class Encryptor {
public:
    virtual ~Encryptor() = default;
    
    // Encryption operations
    virtual std::vector<uint8_t> encrypt(const std::vector<uint8_t>& plaintext,
                                        const std::vector<uint8_t>& additional_data = {}) = 0;
    virtual std::vector<uint8_t> encrypt(const void* plaintext, size_t plaintext_size,
                                        const void* additional_data = nullptr, size_t additional_data_size = 0) = 0;
    virtual bool encrypt(const void* plaintext, size_t plaintext_size,
                        void* ciphertext, size_t& ciphertext_size,
                        const void* additional_data = nullptr, size_t additional_data_size = 0) = 0;
    
    // Decryption operations
    virtual std::vector<uint8_t> decrypt(const std::vector<uint8_t>& ciphertext,
                                        const std::vector<uint8_t>& additional_data = {}) = 0;
    virtual std::vector<uint8_t> decrypt(const void* ciphertext, size_t ciphertext_size,
                                        const void* additional_data = nullptr, size_t additional_data_size = 0) = 0;
    virtual bool decrypt(const void* ciphertext, size_t ciphertext_size,
                        void* plaintext, size_t& plaintext_size,
                        const void* additional_data = nullptr, size_t additional_data_size = 0) = 0;
    
    // Key management
    virtual void set_key(std::shared_ptr<CryptographicKey> key) = 0;
    virtual bool has_valid_key() const = 0;
    virtual void rotate_key() = 0; // Generate new key/IV
    
    // Size estimation
    virtual size_t get_max_ciphertext_size(size_t plaintext_size) const = 0;
    virtual size_t get_max_plaintext_size(size_t ciphertext_size) const = 0;
    virtual size_t get_overhead_size() const = 0;
    
    // Algorithm info
    virtual EncryptionAlgorithm get_algorithm() const = 0;
    virtual std::string get_algorithm_name() const = 0;
    virtual size_t get_key_size() const = 0;
    virtual size_t get_iv_size() const = 0;
    
    // Statistics
    virtual EncryptionStats get_statistics() const = 0;
    virtual void reset_statistics() = 0;
};

/**
 * @brief No-op Encryptor (pass-through)
 */
class NullEncryptor : public Encryptor {
public:
    NullEncryptor() = default;
    ~NullEncryptor() override = default;
    
    std::vector<uint8_t> encrypt(const std::vector<uint8_t>& plaintext,
                               const std::vector<uint8_t>& additional_data = {}) override;
    std::vector<uint8_t> encrypt(const void* plaintext, size_t plaintext_size,
                               const void* additional_data = nullptr, size_t additional_data_size = 0) override;
    bool encrypt(const void* plaintext, size_t plaintext_size,
                void* ciphertext, size_t& ciphertext_size,
                const void* additional_data = nullptr, size_t additional_data_size = 0) override;
    
    std::vector<uint8_t> decrypt(const std::vector<uint8_t>& ciphertext,
                               const std::vector<uint8_t>& additional_data = {}) override;
    std::vector<uint8_t> decrypt(const void* ciphertext, size_t ciphertext_size,
                               const void* additional_data = nullptr, size_t additional_data_size = 0) override;
    bool decrypt(const void* ciphertext, size_t ciphertext_size,
                void* plaintext, size_t& plaintext_size,
                const void* additional_data = nullptr, size_t additional_data_size = 0) override;
    
    void set_key(std::shared_ptr<CryptographicKey> key) override {}
    bool has_valid_key() const override { return true; }
    void rotate_key() override {}
    
    size_t get_max_ciphertext_size(size_t plaintext_size) const override { return plaintext_size; }
    size_t get_max_plaintext_size(size_t ciphertext_size) const override { return ciphertext_size; }
    size_t get_overhead_size() const override { return 0; }
    
    EncryptionAlgorithm get_algorithm() const override { return EncryptionAlgorithm::NONE; }
    std::string get_algorithm_name() const override { return "None"; }
    size_t get_key_size() const override { return 0; }
    size_t get_iv_size() const override { return 0; }
    
    EncryptionStats get_statistics() const override;
    void reset_statistics() override;

private:
    mutable EncryptionStats statistics_;
    mutable std::mutex statistics_mutex_;
};

/**
 * @brief AES-GCM Encryptor Implementation
 */
class AESGCMEncryptor : public Encryptor {
public:
    explicit AESGCMEncryptor(size_t key_size_bits = 256); // 128 or 256
    ~AESGCMEncryptor() override;
    
    std::vector<uint8_t> encrypt(const std::vector<uint8_t>& plaintext,
                               const std::vector<uint8_t>& additional_data = {}) override;
    std::vector<uint8_t> encrypt(const void* plaintext, size_t plaintext_size,
                               const void* additional_data = nullptr, size_t additional_data_size = 0) override;
    bool encrypt(const void* plaintext, size_t plaintext_size,
                void* ciphertext, size_t& ciphertext_size,
                const void* additional_data = nullptr, size_t additional_data_size = 0) override;
    
    std::vector<uint8_t> decrypt(const std::vector<uint8_t>& ciphertext,
                               const std::vector<uint8_t>& additional_data = {}) override;
    std::vector<uint8_t> decrypt(const void* ciphertext, size_t ciphertext_size,
                               const void* additional_data = nullptr, size_t additional_data_size = 0) override;
    bool decrypt(const void* ciphertext, size_t ciphertext_size,
                void* plaintext, size_t& plaintext_size,
                const void* additional_data = nullptr, size_t additional_data_size = 0) override;
    
    void set_key(std::shared_ptr<CryptographicKey> key) override;
    bool has_valid_key() const override;
    void rotate_key() override;
    
    size_t get_max_ciphertext_size(size_t plaintext_size) const override;
    size_t get_max_plaintext_size(size_t ciphertext_size) const override;
    size_t get_overhead_size() const override;
    
    EncryptionAlgorithm get_algorithm() const override;
    std::string get_algorithm_name() const override;
    size_t get_key_size() const override { return key_size_bits_ / 8; }
    size_t get_iv_size() const override { return 12; } // 96 bits for GCM
    
    EncryptionStats get_statistics() const override;
    void reset_statistics() override;

private:
    struct AESContext;
    std::unique_ptr<AESContext> context_;
    size_t key_size_bits_;
    std::shared_ptr<CryptographicKey> key_;
    mutable EncryptionStats statistics_;
    mutable std::mutex statistics_mutex_;
    
    void initialize_context();
    void cleanup_context();
    std::vector<uint8_t> generate_iv() const;
    void update_encryption_stats(size_t plaintext_size, double time_microseconds) const;
    void update_decryption_stats(size_t ciphertext_size, double time_microseconds) const;
};

/**
 * @brief ChaCha20-Poly1305 Encryptor Implementation
 */
class ChaCha20Poly1305Encryptor : public Encryptor {
public:
    ChaCha20Poly1305Encryptor();
    ~ChaCha20Poly1305Encryptor() override;
    
    std::vector<uint8_t> encrypt(const std::vector<uint8_t>& plaintext,
                               const std::vector<uint8_t>& additional_data = {}) override;
    std::vector<uint8_t> encrypt(const void* plaintext, size_t plaintext_size,
                               const void* additional_data = nullptr, size_t additional_data_size = 0) override;
    bool encrypt(const void* plaintext, size_t plaintext_size,
                void* ciphertext, size_t& ciphertext_size,
                const void* additional_data = nullptr, size_t additional_data_size = 0) override;
    
    std::vector<uint8_t> decrypt(const std::vector<uint8_t>& ciphertext,
                               const std::vector<uint8_t>& additional_data = {}) override;
    std::vector<uint8_t> decrypt(const void* ciphertext, size_t ciphertext_size,
                               const void* additional_data = nullptr, size_t additional_data_size = 0) override;
    bool decrypt(const void* ciphertext, size_t ciphertext_size,
                void* plaintext, size_t& plaintext_size,
                const void* additional_data = nullptr, size_t additional_data_size = 0) override;
    
    void set_key(std::shared_ptr<CryptographicKey> key) override;
    bool has_valid_key() const override;
    void rotate_key() override;
    
    size_t get_max_ciphertext_size(size_t plaintext_size) const override;
    size_t get_max_plaintext_size(size_t ciphertext_size) const override;
    size_t get_overhead_size() const override;
    
    EncryptionAlgorithm get_algorithm() const override { return EncryptionAlgorithm::CHACHA20_POLY1305; }
    std::string get_algorithm_name() const override { return "ChaCha20-Poly1305"; }
    size_t get_key_size() const override { return 32; } // 256 bits
    size_t get_iv_size() const override { return 12; }  // 96 bits
    
    EncryptionStats get_statistics() const override;
    void reset_statistics() override;

private:
    struct ChaChaContext;
    std::unique_ptr<ChaChaContext> context_;
    std::shared_ptr<CryptographicKey> key_;
    mutable EncryptionStats statistics_;
    mutable std::mutex statistics_mutex_;
    
    void initialize_context();
    void cleanup_context();
    std::vector<uint8_t> generate_nonce() const;
    void update_encryption_stats(size_t plaintext_size, double time_microseconds) const;
    void update_decryption_stats(size_t ciphertext_size, double time_microseconds) const;
};

/**
 * @brief Key Exchange Interface
 */
class KeyExchange {
public:
    virtual ~KeyExchange() = default;
    
    // Key exchange operations
    virtual std::vector<uint8_t> generate_public_key() = 0;
    virtual std::shared_ptr<CryptographicKey> derive_shared_secret(const std::vector<uint8_t>& peer_public_key) = 0;
    
    // Pre-shared key support
    virtual void set_pre_shared_key(std::shared_ptr<CryptographicKey> psk) = 0;
    virtual bool has_pre_shared_key() const = 0;
    
    // Method info
    virtual KeyExchangeMethod get_method() const = 0;
    virtual std::string get_method_name() const = 0;
    virtual size_t get_public_key_size() const = 0;
    virtual size_t get_shared_secret_size() const = 0;
    
    // Security parameters
    virtual bool is_post_quantum_safe() const = 0;
    virtual size_t get_security_level_bits() const = 0;
    
    // Statistics
    virtual uint64_t get_key_exchange_count() const = 0;
    virtual void reset_statistics() = 0;
};

/**
 * @brief X25519 ECDH Key Exchange
 */
class X25519KeyExchange : public KeyExchange {
public:
    X25519KeyExchange();
    ~X25519KeyExchange() override;
    
    std::vector<uint8_t> generate_public_key() override;
    std::shared_ptr<CryptographicKey> derive_shared_secret(const std::vector<uint8_t>& peer_public_key) override;
    
    void set_pre_shared_key(std::shared_ptr<CryptographicKey> psk) override;
    bool has_pre_shared_key() const override;
    
    KeyExchangeMethod get_method() const override { return KeyExchangeMethod::ECDH_X25519; }
    std::string get_method_name() const override { return "X25519"; }
    size_t get_public_key_size() const override { return 32; }
    size_t get_shared_secret_size() const override { return 32; }
    
    bool is_post_quantum_safe() const override { return false; }
    size_t get_security_level_bits() const override { return 128; }
    
    uint64_t get_key_exchange_count() const override;
    void reset_statistics() override;

private:
    struct X25519Context;
    std::unique_ptr<X25519Context> context_;
    std::shared_ptr<CryptographicKey> pre_shared_key_;
    std::atomic<uint64_t> key_exchange_count_{0};
    
    void generate_key_pair();
};

/**
 * @brief Encryption Factory
 */
class EncryptionFactory {
public:
    static std::unique_ptr<Encryptor> create_encryptor(EncryptionAlgorithm algorithm);
    static std::unique_ptr<KeyExchange> create_key_exchange(KeyExchangeMethod method);
    
    static std::unique_ptr<Encryptor> create_null_encryptor();
    static std::unique_ptr<Encryptor> create_aes_gcm_encryptor(size_t key_size_bits = 256);
    static std::unique_ptr<Encryptor> create_chacha20_poly1305_encryptor();
    
    static std::unique_ptr<KeyExchange> create_x25519_key_exchange();
    
    // Registration for custom encryptors
    using EncryptorCreator = std::function<std::unique_ptr<Encryptor>()>;
    using KeyExchangeCreator = std::function<std::unique_ptr<KeyExchange>()>;
    
    static void register_encryptor(EncryptionAlgorithm algorithm, EncryptorCreator creator);
    static void register_key_exchange(KeyExchangeMethod method, KeyExchangeCreator creator);
    
    // Query available algorithms
    static std::vector<EncryptionAlgorithm> get_available_algorithms();
    static std::vector<KeyExchangeMethod> get_available_key_exchange_methods();
    static bool is_algorithm_available(EncryptionAlgorithm algorithm);
    static bool is_key_exchange_available(KeyExchangeMethod method);

private:
    static std::unordered_map<EncryptionAlgorithm, EncryptorCreator>& get_encryptor_registry();
    static std::unordered_map<KeyExchangeMethod, KeyExchangeCreator>& get_key_exchange_registry();
    static std::mutex& get_registry_mutex();
};

/**
 * @brief Secure Network Protocol
 * 
 * High-level protocol that combines key exchange, encryption, and authentication.
 */
class SecureNetworkProtocol {
public:
    struct SecurityConfig {
        EncryptionAlgorithm encryption_algorithm = EncryptionAlgorithm::AES_256_GCM;
        KeyExchangeMethod key_exchange_method = KeyExchangeMethod::ECDH_X25519;
        bool require_mutual_authentication = true;
        std::chrono::seconds key_rotation_interval{300}; // 5 minutes
        size_t max_unauthenticated_data = 1024; // Bytes
        bool enable_forward_secrecy = true;
    };
    
    enum State : uint8_t {
        DISCONNECTED = 0,
        HANDSHAKING = 1,
        AUTHENTICATED = 2,
        SECURE_CONNECTED = 3,
        ERROR_STATE = 4
    };
    
    explicit SecureNetworkProtocol(const SecurityConfig& config = {});
    ~SecureNetworkProtocol() = default;
    
    // Protocol state machine
    State get_state() const { return state_.load(); }
    bool is_secure() const { return get_state() == SECURE_CONNECTED; }
    
    // Handshake process
    std::vector<uint8_t> initiate_handshake();
    std::vector<uint8_t> process_handshake_message(const std::vector<uint8_t>& message);
    bool is_handshake_complete() const;
    
    // Secure data operations
    std::vector<uint8_t> encrypt_data(const std::vector<uint8_t>& plaintext);
    std::vector<uint8_t> decrypt_data(const std::vector<uint8_t>& ciphertext);
    
    // Key management
    void rotate_keys();
    bool should_rotate_keys() const;
    std::chrono::steady_clock::time_point get_last_key_rotation() const;
    
    // Configuration
    void set_config(const SecurityConfig& config) { config_ = config; }
    const SecurityConfig& get_config() const { return config_; }
    
    // Statistics
    struct SecurityStats {
        uint64_t handshakes_completed = 0;
        uint64_t key_rotations = 0;
        uint64_t authentication_failures = 0;
        EncryptionStats encryption_stats;
    };
    
    SecurityStats get_statistics() const;
    void reset_statistics();

private:
    SecurityConfig config_;
    std::atomic<State> state_{DISCONNECTED};
    
    // Cryptographic components
    std::unique_ptr<Encryptor> encryptor_;
    std::unique_ptr<KeyExchange> key_exchange_;
    
    // Session state
    std::shared_ptr<CryptographicKey> session_key_;
    std::chrono::steady_clock::time_point last_key_rotation_;
    uint64_t handshake_nonce_ = 0;
    
    // Statistics
    mutable SecurityStats statistics_;
    mutable std::mutex statistics_mutex_;
    
    // Handshake state
    enum HandshakeState {
        HANDSHAKE_INIT = 0,
        HANDSHAKE_KEY_EXCHANGE = 1,
        HANDSHAKE_AUTHENTICATE = 2,
        HANDSHAKE_COMPLETE = 3
    };
    
    HandshakeState handshake_state_ = HANDSHAKE_INIT;
    std::vector<uint8_t> local_public_key_;
    std::vector<uint8_t> peer_public_key_;
    
    // Helper functions
    void transition_to_state(State new_state);
    std::vector<uint8_t> create_handshake_init_message();
    std::vector<uint8_t> process_handshake_init_message(const std::vector<uint8_t>& message);
    std::vector<uint8_t> create_handshake_response_message();
    bool process_handshake_response_message(const std::vector<uint8_t>& message);
    bool derive_session_key();
    std::vector<uint8_t> generate_nonce() const;
};

} // namespace ecscope::networking