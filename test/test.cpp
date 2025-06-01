#include <gtest/gtest.h>

class BitWriter {
public:
    void writeBit(bool bit);
    void writeBits(const std::vector<bool>& bits);
    std::vector<uint8_t> getBuffer();
};

class BitReader {
public:
    BitReader(const std::vector<uint8_t>& buf);
    bool readBit();
};

struct ShannonProbabilityBytePair {
    uint8_t byte;
    double probability;
};

struct ShannonDictionaryPair {
    std::vector<bool> code;
    uint8_t byte;
};

std::vector<ShannonProbabilityBytePair> getProbabilityOfBytes(const std::vector<uint8_t>& data);
std::vector<ShannonDictionaryPair> getOptimalDictionary(
    const std::vector<ShannonProbabilityBytePair>& probabilities
);
std::unordered_map<uint8_t, std::vector<bool>> buildCodeMap(
    const std::vector<ShannonDictionaryPair>& codes
);
std::vector<uint8_t> shannonEncode(
    const std::vector<uint8_t>& data, 
    std::vector<ShannonDictionaryPair>& codes
);
std::vector<uint8_t> shannonDecode(
    const std::vector<ShannonDictionaryPair>& codes,
    const std::vector<uint8_t>& encodedData
);

void writeDictionaryFile(std::vector<ShannonDictionaryPair>& codes);
std::vector<ShannonDictionaryPair> readDictionaryFile();

TEST(Shannon, EncodeDecode) {
    std::string text = "abracadabra";
    std::vector<uint8_t> data(text.begin(), text.end());
    auto probabilities = getProbabilityOfBytes(data);
    std::vector<ShannonDictionaryPair> codes = getOptimalDictionary(probabilities);
    std::vector<uint8_t> buffer = shannonEncode(data, codes);
    std::vector<uint8_t> decoded = shannonDecode(codes, buffer);
    std::string result(decoded.begin(), decoded.end());
    EXPECT_EQ(result, text);
}

TEST(Shannon, EmptyString) {
    std::string text = "";
    std::vector<uint8_t> data(text.begin(), text.end());
    auto probabilities = getProbabilityOfBytes(data);
    std::vector<ShannonDictionaryPair> codes = getOptimalDictionary(probabilities);
    std::vector<uint8_t> buffer = shannonEncode(data, codes);
    std::vector<uint8_t> decoded = shannonDecode(codes, buffer);
    std::string result(decoded.begin(), decoded.end());
    EXPECT_EQ(result, text);
}

TEST(Shannon, SingleChar) {
    std::string text = "aaaaaa";
    std::vector<uint8_t> data(text.begin(), text.end());
    auto probabilities = getProbabilityOfBytes(data);
    std::vector<ShannonDictionaryPair> codes = getOptimalDictionary(probabilities);
    std::vector<uint8_t> buffer = shannonEncode(data, codes);
    std::vector<uint8_t> decoded = shannonDecode(codes, buffer);
    std::string result(decoded.begin(), decoded.end());
    EXPECT_EQ(result, text);
}

TEST(Shannon, TwoChars) {
    std::string text = "abababab";
    std::vector<uint8_t> data(text.begin(), text.end());
    auto probabilities = getProbabilityOfBytes(data);
    std::vector<ShannonDictionaryPair> codes = getOptimalDictionary(probabilities);
    std::vector<uint8_t> buffer = shannonEncode(data, codes);
    std::vector<uint8_t> decoded = shannonDecode(codes, buffer);
    std::string result(decoded.begin(), decoded.end());
    EXPECT_EQ(result, text);
}

TEST(Shannon, RepeatedPattern) {
    std::string text = "abcabcabcabcabcabc";
    std::vector<uint8_t> data(text.begin(), text.end());
    auto probabilities = getProbabilityOfBytes(data);
    std::vector<ShannonDictionaryPair> codes = getOptimalDictionary(probabilities);
    std::vector<uint8_t> buffer = shannonEncode(data, codes);
    std::vector<uint8_t> decoded = shannonDecode(codes, buffer);
    std::string result(decoded.begin(), decoded.end());
    EXPECT_EQ(result, text);
}

TEST(Shannon, SpacesAndSpecials) {
    std::string text = "a b!c? a b!c?";
    std::vector<uint8_t> data(text.begin(), text.end());
    auto probabilities = getProbabilityOfBytes(data);
    std::vector<ShannonDictionaryPair> codes = getOptimalDictionary(probabilities);
    std::vector<uint8_t> buffer = shannonEncode(data, codes);
    std::vector<uint8_t> decoded = shannonDecode(codes, buffer);
    std::string result(decoded.begin(), decoded.end());
    EXPECT_EQ(result, text);
}

TEST(Shannon, Cyrillic) {
    std::string text = "привет мир";
    std::vector<uint8_t> data(text.begin(), text.end());
    auto probabilities = getProbabilityOfBytes(data);
    std::vector<ShannonDictionaryPair> codes = getOptimalDictionary(probabilities);
    std::vector<uint8_t> buffer = shannonEncode(data, codes);
    std::vector<uint8_t> decoded = shannonDecode(codes, buffer);
    std::string result(decoded.begin(), decoded.end());
    EXPECT_EQ(result, text);
}

TEST(Shannon, LongString) {
    std::string text(1000, 'x');
    for (int i = 0; i < 1000; i += 2) text[i] = 'y';
    std::vector<uint8_t> data(text.begin(), text.end());
    auto probabilities = getProbabilityOfBytes(data);
    std::vector<ShannonDictionaryPair> codes = getOptimalDictionary(probabilities);
    std::vector<uint8_t> buffer = shannonEncode(data, codes);
    std::vector<uint8_t> decoded = shannonDecode(codes, buffer);
    std::string result(decoded.begin(), decoded.end());
    EXPECT_EQ(result, text);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
