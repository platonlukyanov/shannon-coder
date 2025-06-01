
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

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
