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

struct ShannonProbabilityCharPair {
    char character;
    double probability;
};

struct ShannonDictionaryPair {
    std::vector<bool> code;
    char character;
};

std::vector<ShannonProbabilityCharPair> getProbabilityOfSymbols(std::string& text);
std::vector<ShannonDictionaryPair> getOptimalDictionary(
    const std::vector<ShannonProbabilityCharPair>& probabilities
);
std::unordered_map<char, std::vector<bool>> buildCodeMap(
    const std::vector<ShannonDictionaryPair>& codes
);
std::vector<uint8_t> shannonEncode(
    std::string& text, 
    std::vector<ShannonDictionaryPair>& codes
);
std::string shannonDecode(
    const std::vector<ShannonDictionaryPair>& codes,
    const std::vector<uint8_t>& encodedData
);

void writeDictionaryFile(std::vector<ShannonDictionaryPair>& codes);
std::vector<ShannonDictionaryPair> readDictionaryFile();

TEST(Shannon, EncodeDecode) {
    std::string text = "abracadabra";
    auto probabilities = getProbabilityOfSymbols(text);
    std::vector<ShannonDictionaryPair> codes = getOptimalDictionary(probabilities);
    std::vector<uint8_t> buffer = shannonEncode(text, codes);
    std::string decoded = shannonDecode(codes, buffer);
    EXPECT_EQ(decoded, text);
}

TEST(Shannon, EmptyString) {
    std::string text = "";
    auto probabilities = getProbabilityOfSymbols(text);
    std::vector<ShannonDictionaryPair> codes = getOptimalDictionary(probabilities);
    std::vector<uint8_t> buffer = shannonEncode(text, codes);
    std::string decoded = shannonDecode(codes, buffer);
    EXPECT_EQ(decoded, text);
}

TEST(Shannon, SingleChar) {
    std::string text = "aaaaaa";
    auto probabilities = getProbabilityOfSymbols(text);
    std::vector<ShannonDictionaryPair> codes = getOptimalDictionary(probabilities);
    std::vector<uint8_t> buffer = shannonEncode(text, codes);
    std::string decoded = shannonDecode(codes, buffer);
    EXPECT_EQ(decoded, text);
}

TEST(Shannon, TwoChars) {
    std::string text = "abababab";
    auto probabilities = getProbabilityOfSymbols(text);
    std::vector<ShannonDictionaryPair> codes = getOptimalDictionary(probabilities);
    std::vector<uint8_t> buffer = shannonEncode(text, codes);
    std::string decoded = shannonDecode(codes, buffer);
    EXPECT_EQ(decoded, text);
}

TEST(Shannon, RepeatedPattern) {
    std::string text = "abcabcabcabcabcabc";
    auto probabilities = getProbabilityOfSymbols(text);
    std::vector<ShannonDictionaryPair> codes = getOptimalDictionary(probabilities);
    std::vector<uint8_t> buffer = shannonEncode(text, codes);
    std::string decoded = shannonDecode(codes, buffer);
    EXPECT_EQ(decoded, text);
}

TEST(Shannon, SpacesAndSpecials) {
    std::string text = "a b!c? a b!c?";
    auto probabilities = getProbabilityOfSymbols(text);
    std::vector<ShannonDictionaryPair> codes = getOptimalDictionary(probabilities);
    std::vector<uint8_t> buffer = shannonEncode(text, codes);
    std::string decoded = shannonDecode(codes, buffer);
    EXPECT_EQ(decoded, text);
}

TEST(Shannon, Cyrillic) {
    std::string text = "привет мир";
    auto probabilities = getProbabilityOfSymbols(text);
    std::vector<ShannonDictionaryPair> codes = getOptimalDictionary(probabilities);
    std::vector<uint8_t> buffer = shannonEncode(text, codes);
    std::string decoded = shannonDecode(codes, buffer);
    EXPECT_EQ(decoded, text);
}

TEST(Shannon, LongString) {
    std::string text(1000, 'x');
    for (int i = 0; i < 1000; i += 2) text[i] = 'y';
    auto probabilities = getProbabilityOfSymbols(text);
    std::vector<ShannonDictionaryPair> codes = getOptimalDictionary(probabilities);
    std::vector<uint8_t> buffer = shannonEncode(text, codes);
    std::string decoded = shannonDecode(codes, buffer);
    EXPECT_EQ(decoded, text);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
