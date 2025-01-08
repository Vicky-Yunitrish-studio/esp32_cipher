from Crypto.Cipher import AES
from Crypto.Hash import SHA256
import binascii

class Decryption:
    def __init__(self, group_name, device_mac):
        self.key_size = 32
        self.nonce_size = 16
        self.constant = "esp32_duel_cipher"
        self.group_name = group_name
        self.device_mac = device_mac
        self.key = self._derive_key()
        self.nonce = self._generate_nonce()
    
    def _derive_key(self):
        initial_key = self.constant.encode().ljust(32, b'\0')
        hash_obj = SHA256.new()
        hash_obj.update(initial_key)
        return hash_obj.digest()

    def _generate_nonce(self):
        seed = (self.group_name + self.device_mac).encode()
        hash_obj = SHA256.new()
        hash_obj.update(seed)
        return hash_obj.digest()[:16]

    def decrypt(self, encrypted_hex):
        try:
            encrypted_data = binascii.unhexlify(encrypted_hex)
            if len(encrypted_data) % 16 != 0:
                print("Invalid data length")
                return None

            cipher = AES.new(self.key, AES.MODE_ECB)
            decrypted_data = cipher.decrypt(encrypted_data)
            
            padding_len = decrypted_data[-1]
            if padding_len > 16:
                print("Invalid padding")
                return None
                
            unpadded_data = decrypted_data[:-padding_len]
            return unpadded_data.decode('utf-8')
            
        except Exception as e:
            print(f"Decryption error: {e}")
            return None
