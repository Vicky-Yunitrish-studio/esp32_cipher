from chacha20 import ChaCha20
import binascii

def forge_message(mac_address, timestamp):
    """創建一個符合格式的加密訊息"""
    # 初始化加密器
    cryptor = ChaCha20()
    key = binascii.unhexlify("DC3935B94F7069973B0AF683F2F219A1EF2408391254E632367944E7BDAA7B0F")
    cryptor.init(mac_address, key, len(key))
    
    # 構建原始訊息
    message = f"Test message from {mac_address} at {timestamp}"
    
    # 加密訊息
    cryptor.reset_counter()
    encrypted = cryptor.encrypt(message)
    
    # 測試解密
    cryptor.reset_counter()
    decrypted = cryptor.decrypt(encrypted)
    
    print("\n=== Forged Message ===")
    print(f"Original: {message}")
    print(f"Encrypted: {encrypted}")
    print(f"Decrypted: {decrypted}")
    print(f"Verification: {'Success' if message == decrypted else 'Failed'}")
    print("=====================")
    
    return {
        'topic': f"duel_cipher32/test_group/{mac_address}/test",
        'message': message,
        'encrypted': encrypted
    }

def main():
    # 使用與 ESP32 相同的 MAC 和時間戳
    mac_address = "34865DB6DF94"
    timestamps = ["30783", "35861", "40948", "46026"]
    
    print("\nStarting message forgery test...")
    print(f"Using MAC address: {mac_address}")
    print(f"Key: DC3935B94F7069973B0AF683F2F219A1EF2408391254E632367944E7BDAA7B0F")
    
    forged_messages = []
    for timestamp in timestamps:
        result = forge_message(mac_address, timestamp)
        forged_messages.append(result)
    
    # 輸出適合複製貼上的格式
    print("\n=== All Forged Messages ===")
    for msg in forged_messages:
        print(f"""
=== MQTT Debug Info ===
Topic: {msg['topic']}
Original: {msg['message']}
Encrypted: {msg['encrypted']}
Decrypted: {msg['message']}
=====================

Published message: {msg['message']}
""")

if __name__ == "__main__":
    main()
