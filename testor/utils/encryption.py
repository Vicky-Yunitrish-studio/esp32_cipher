from Crypto.Cipher import AES
from Crypto.Util.Padding import pad, unpad
import binascii

class Encryption:
    def __init__(self):
        # ESP32 使用的密鑰和 IV
        self.key = b'0123456789abcdef0123456789abcdef'  # 32 bytes key
        self.iv = b'0123456789abcdef'  # 16 bytes IV

    def encrypt(self, data):
        """Encrypt data using AES-CBC"""
        try:
            # Convert string to bytes and pad
            data_bytes = str(data).encode('utf-8')
            padded_data = pad(data_bytes, AES.block_size)
            
            # Create cipher and encrypt
            cipher = AES.new(self.key, AES.MODE_CBC, self.iv)
            encrypted_data = cipher.encrypt(padded_data)
            
            # Convert to hex string
            return binascii.hexlify(encrypted_data).decode('utf-8')
        except Exception as e:
            raise Exception(f"Encryption error: {str(e)}")

    def decrypt(self, encrypted_text):
        try:
            # Convert hex string to bytes
            encrypted_bytes = bytes.fromhex(encrypted_text)
            
            # Create cipher and decrypt
            cipher = AES.new(self.key, AES.MODE_CBC, self.iv)
            decrypted = unpad(cipher.decrypt(encrypted_bytes), AES.block_size)
            return decrypted.decode('utf-8')
        except Exception as e:
            raise Exception(f"Decryption error: {str(e)}")
