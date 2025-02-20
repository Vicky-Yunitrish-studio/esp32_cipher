import struct
import binascii

class ChaCha20:
    def __init__(self):
        self.KEY_SIZE = 32
        self.NONCE_SIZE = 12
        self.key = bytearray(self.KEY_SIZE)
        self.nonce = bytearray(self.NONCE_SIZE)
        self.counter = 0

    def format_mac_address(self, mac_address):
        return ''.join(c for c in mac_address if c != ':')

    def init(self, mac_address, custom_key=None, key_length=None):
        self.counter = 0
        formatted_mac = self.format_mac_address(mac_address).upper()  # 全部轉大寫
        
        # Initialize nonce - 直接使用 MAC 地址字符的 ASCII 值
        self.nonce = bytearray(self.NONCE_SIZE)
        for i in range(min(self.NONCE_SIZE, len(formatted_mac))):
            self.nonce[i] = ord(formatted_mac[i])
        
        # Print debug info
        print(f"\nInitialization debug info:")
        print(f"Formatted MAC: {formatted_mac}")
        print(f"Nonce (ASCII values):", end=" ")
        for b in self.nonce:
            print(f"{b:02X}", end=" ")  # 顯示每個位元組的 ASCII 值
        print(f"\nNonce (hex): {self.nonce.hex()}")
        print(f"Nonce (as string): {self.nonce.decode('ascii', errors='ignore')}")
        
        # Set key
        self.key = custom_key
        print(f"Key (hex): {self.key.hex()}\n")

    def quarter_round(self, state, a, b, c, d):
        state[a] = (state[a] + state[b]) & 0xFFFFFFFF
        state[d] ^= state[a]
        state[d] = ((state[d] << 16) | (state[d] >> 16)) & 0xFFFFFFFF

        state[c] = (state[c] + state[d]) & 0xFFFFFFFF
        state[b] ^= state[c]
        state[b] = ((state[b] << 12) | (state[b] >> 20)) & 0xFFFFFFFF

        state[a] = (state[a] + state[b]) & 0xFFFFFFFF
        state[d] ^= state[a]
        state[d] = ((state[d] << 8) | (state[d] >> 24)) & 0xFFFFFFFF

        state[c] = (state[c] + state[d]) & 0xFFFFFFFF
        state[b] ^= state[c]
        state[b] = ((state[b] << 7) | (state[b] >> 25)) & 0xFFFFFFFF

    def chacha20_block(self, counter, nonce):
        # Print detailed debug info
        print(f"\nBlock generation details:")
        print(f"Counter: {counter}")
        print(f"Nonce (hex): {nonce.hex()}")
        print(f"Key (hex): {self.key.hex()}")
        
        # Ensure little-endian state setup
        state = [
            0x61707865, 0x3320646e, 0x79622d32, 0x6b206574,  # Constants
            *(int.from_bytes(self.key[i:i+4], 'little') for i in range(0, 32, 4)),  # Key
            counter,  # Counter
            *(int.from_bytes(nonce[i:i+4], 'little') for i in range(0, 12, 4))  # Nonce
        ]
        
        # Print initial state for debugging
        print("\nInitial state (hex):")
        for i in range(0, 16, 4):
            print(f"{state[i]:08x} {state[i+1]:08x} {state[i+2]:08x} {state[i+3]:08x}")

        working = state.copy()
        
        # 20 rounds (10 double rounds)
        for _ in range(10):
            # Column rounds
            self.quarter_round(working, 0, 4, 8, 12)
            self.quarter_round(working, 1, 5, 9, 13)
            self.quarter_round(working, 2, 6, 10, 14)
            self.quarter_round(working, 3, 7, 11, 15)
            
            # Diagonal rounds
            self.quarter_round(working, 0, 5, 10, 15)
            self.quarter_round(working, 1, 6, 11, 12)
            self.quarter_round(working, 2, 7, 8, 13)
            self.quarter_round(working, 3, 4, 9, 14)

        # Add working state to initial state
        working = [(a + b) & 0xFFFFFFFF for a, b in zip(working, state)]
        
        # Convert to bytes in little-endian order
        result = bytearray()
        for x in working:
            result.extend(x.to_bytes(4, 'little'))
        return result

    def encrypt(self, data):
        if isinstance(data, str):
            data = data.encode()
        
        data_len = len(data)
        output = bytearray(data_len)
        local_counter = self.counter
        
        # Generate keystream and encrypt
        for i in range(0, data_len, 64):
            keystream = self.chacha20_block(local_counter, self.nonce)
            local_counter += 1
            
            block_size = min(64, data_len - i)
            for j in range(block_size):
                output[i + j] = data[i + j] ^ keystream[j]
        
        # Update global counter
        self.counter = local_counter
        
        # Convert to hex string
        return binascii.hexlify(output).decode()

    def decrypt(self, encrypted_hex):
        try:
            print("\nDecryption debug info:")
            print(f"Counter: {self.counter}")
            print(f"Nonce: {self.nonce.hex()}")
            print(f"Input: {encrypted_hex}")
            
            # Convert hex string to bytes
            data_len = len(encrypted_hex) // 2
            input_data = binascii.unhexlify(encrypted_hex)
            output = bytearray(data_len)
            local_counter = self.counter
            
            # Decrypt using keystream
            for i in range(0, data_len, 64):
                keystream = self.chacha20_block(local_counter, self.nonce)
                print(f"Block {local_counter} keystream: {keystream.hex()[:32]}...")
                local_counter += 1
                
                block_size = min(64, data_len - i)
                for j in range(block_size):
                    output[i + j] = input_data[i + j] ^ keystream[j]
            
            # Update global counter
            self.counter = local_counter
            
            # Try to decode with error handling
            try:
                result = output.decode('utf-8')
                print(f"Decrypted (hex): {output.hex()}")
                print(f"Decrypted (utf-8): {result}")
                return result
            except UnicodeDecodeError:
                print(f"Raw decrypted bytes: {output.hex()}")
                return None
            
        except Exception as e:
            print(f"Decryption error: {str(e)}")
            return None

    def reset_counter(self):
        self.counter = 0

# 測試代碼
if __name__ == "__main__":
    # 初始化
    cryptor = ChaCha20()
    mac_address = "34:86:5D:B6:DF:94"
    key = binascii.unhexlify("DC3935B94F7069973B0AF683F2F219A1EF2408391254E632367944E7BDAA7B0F")
    
    # 初始化加密器
    cryptor.init(mac_address, key, len(key))
    
    # 測試消息
    test_message = "Test message from ESP32"
    print(f"Original: {test_message}")
    
    # 加密
    encrypted = cryptor.encrypt(test_message)
    print(f"Encrypted: {encrypted}")
    
    # 重置計數器
    cryptor.reset_counter()
    
    # 解密
    decrypted = cryptor.decrypt(encrypted)
    print(f"Decrypted: {decrypted}")
    
    # 驗證
    print(f"Verification: {'Success' if test_message == decrypted else 'Failed'}")
