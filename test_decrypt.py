from chacha20 import ChaCha20
import binascii

# 測試數據
TEST_CASES = [
    {
        "topic": "duel_cipher32/test_group/34865DB6DF94/test",
        "original": "Test message from 34865DB6DF94 at 30783",
        "encrypted": "c77b35027edc9cc4173ba397c8ac32b97b29e5bed2855cc23422a841ddd1132361deb7080dce31",
        "counter": 1,
        "nonce": "333438363544423644463934"
    },
    {
        "topic": "duel_cipher32/test_group/34865DB6DF94/test",
        "original": "Test message from 34865DB6DF94 at 35861",
        "encrypted": "c77b35027edc9cc4173ba397c8ac32b97b29e5bed2855cc23422a841ddd1132361deb70d02c033",
        "counter": 1,
        "nonce": "333438363544423644463934"
    },
    {
        "topic": "duel_cipher32/test_group/34865DB6DF94/test",
        "original": "Test message from 34865DB6DF94 at 40948",
        "encrypted": "c77b35027edc9cc4173ba397c8ac32b97b29e5bed2855cc23422a841ddd1132361deb00803c23a",
        "counter": 1,
        "nonce": "333438363544423644463934"
    },
    {
        "topic": "duel_cipher32/test_group/34865DB6DF94/test",
        "original": "Test message from 34865DB6DF94 at 46026",
        "encrypted": "c77b35027edc9cc4173ba397c8ac32b97b29e5bed2855cc23422a841ddd1132361deb00e0ac434",
        "counter": 1,
        "nonce": "333438363544423644463934"
    }
]

def extract_mac_from_topic(topic):
    parts = topic.split('/')
    if len(parts) >= 4:
        return parts[2]
    return None

def test_decrypt():
    # 初始化解密器
    cryptor = ChaCha20()
    key = binascii.unhexlify("DC3935B94F7069973B0AF683F2F219A1EF2408391254E632367944E7BDAA7B0F")
    
    total_tests = len(TEST_CASES)
    passed_tests = 0
    
    print("Starting decryption tests...\n")
    
    for i, test in enumerate(TEST_CASES, 1):
        print(f"=== Test Case {i} ===")
        print(f"Topic: {test['topic']}")
        print(f"Original: {test['original']}")
        print(f"Encrypted: {test['encrypted']}")
        print(f"Expected Nonce: {test['nonce']}")
        
        # 從 topic 提取 MAC 地址
        device_mac = extract_mac_from_topic(test['topic'])
        
        # 初始化加密器
        cryptor.init(device_mac, key, len(key))
        
        # 重置計數器
        cryptor.reset_counter()
        
        # 解密
        decrypted = cryptor.decrypt(test['encrypted'])
        
        # 驗證結果
        if decrypted == test['original']:
            print(f"✓ Test passed!")
            passed_tests += 1
        else:
            print(f"✗ Test failed!")
            print(f"Expected: {test['original']}")
            print(f"Got: {decrypted}")
        print("=====================\n")
    
    # 輸出總結果
    print(f"Test Summary: {passed_tests}/{total_tests} tests passed")
    
if __name__ == "__main__":
    test_decrypt()
