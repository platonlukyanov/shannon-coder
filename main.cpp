#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <set>
#include <fstream>

#ifdef TESTING
#endif

class BitWriter {
    std::vector<uint8_t> buffer;
    uint8_t currentByte = 0;
    int bitsFilled = 0;
    uint32_t totalBits = 0;

public:
    void writeBit(bool bit) {
        currentByte <<= 1;
        if (bit) currentByte |= 1;
        bitsFilled++;
        totalBits++;
        if (bitsFilled == 8) {
            buffer.push_back(currentByte);
            currentByte = 0;
            bitsFilled = 0;
        }
    }

    void writeBits(const std::vector<bool>& bits) {
        for (bool bit : bits) {
            writeBit(bit);
        }
    }

    std::vector<uint8_t> getBuffer() {
        std::vector<uint8_t> result;
        result.push_back(static_cast<uint8_t>(totalBits & 0xFF));
        result.push_back(static_cast<uint8_t>((totalBits >> 8) & 0xFF));
        result.push_back(static_cast<uint8_t>((totalBits >> 16) & 0xFF));
        result.push_back(static_cast<uint8_t>((totalBits >> 24) & 0xFF));
        for (uint8_t b : buffer) result.push_back(b);
        if (bitsFilled > 0) {
            currentByte <<= (8 - bitsFilled);
            result.push_back(currentByte);
            currentByte = 0;
            bitsFilled = 0;
        }
        return result;
    }
};

class BitReader {
    const std::vector<uint8_t>& buffer;
    size_t byteIndex = 4;
    int bitIndex = 7;
    uint32_t totalBits = 0;
    uint32_t bitsRead = 0;

public:
    BitReader(const std::vector<uint8_t>& buf) : buffer(buf) {
        if (buffer.size() < 4) {
            totalBits = 0;
        } else {
            totalBits = static_cast<uint32_t>(buffer[0]) |
                        (static_cast<uint32_t>(buffer[1]) << 8) |
                        (static_cast<uint32_t>(buffer[2]) << 16) |
                        (static_cast<uint32_t>(buffer[3]) << 24);
        }
    }

    bool readBit() {
        if (bitsRead >= totalBits) throw std::runtime_error("Read past end");
        if (byteIndex >= buffer.size()) throw std::runtime_error("Read past end");
        bool bit = (buffer[byteIndex] >> bitIndex) & 1;
        if (--bitIndex < 0) {
            bitIndex = 7;
            ++byteIndex;
        }
        bitsRead++;
        return bit;
    }
};

std::string readString() {
	std::string resultingStr;
	char currentChar; 
    	
	while(std::cin.get(currentChar)) {
		resultingStr += currentChar;
    }

    return resultingStr;
}

struct ShannonProbabilityBytePair {
    uint8_t byte;
    double probability;
};

ShannonProbabilityBytePair getProbabilityOfAppearenceForByte(uint8_t byte, const std::vector<uint8_t>& data) {
    size_t count = 0;
    for (size_t i = 0; i < data.size(); ++i) {
        if (data[i] == byte) {
            ++count;
        }
    }

    return {byte, static_cast<double>(count) / data.size()};
}

std::vector<ShannonProbabilityBytePair> getProbabilityOfBytes(const std::vector<uint8_t>& data) {
    std::unordered_map<uint8_t, size_t> countMap;
    for (uint8_t b : data) countMap[b]++;
    
    std::vector<ShannonProbabilityBytePair> result;
    for (const auto& pair : countMap) {
        result.push_back({pair.first, 
            static_cast<double>(pair.second)/data.size()});
    }
    
    std::sort(result.begin(), result.end(), 
        [](const auto& a, const auto& b) {
            return a.probability > b.probability;
        });
    
    return result;
}

struct ShannonDictionaryPair {
    std::vector<bool> code;
    uint8_t byte;
};

std::vector<ShannonDictionaryPair> getOptimalDictionary(const std::vector<ShannonProbabilityBytePair>& probabilities) {
    std::vector<ShannonDictionaryPair> codes;
    double cumulativeProb = 0.0;

    for (const auto& symbol : probabilities) {
        const double p = symbol.probability;
        const int length = static_cast<int>(ceil(-log2(p)));
        std::vector<bool> code;
        double frac = cumulativeProb;
        for (int i = 0; i < length; ++i) {
            frac *= 2;
            int bit = static_cast<int>(frac);
            code.push_back(bit == 1);
            frac -= bit;
        }
        codes.push_back({code, symbol.byte});
        cumulativeProb += p;
    }

    std::sort(codes.begin(), codes.end(),
        [](const auto& a, const auto& b) {
            return a.code.size() < b.code.size();
        });

    return codes;
}

std::unordered_map<uint8_t, std::vector<bool>> buildCodeMap(const std::vector<ShannonDictionaryPair>& codes) {
    std::unordered_map<uint8_t, std::vector<bool>> codeMap;
    for (const auto& pair : codes) {
        codeMap[pair.byte] = pair.code;
    }
    return codeMap;
}

std::vector<uint8_t> shannonEncode(const std::vector<uint8_t>& data, std::vector<ShannonDictionaryPair>& codes) {
    if (data.empty() || codes.empty()) return std::vector<uint8_t>{0, 0, 0, 0};
    // Если только один байт в словаре, кодируем каждый байт одним битом (0)
    if (codes.size() == 1) {
        BitWriter writer;
        for (size_t i = 0; i < data.size(); ++i) {
            writer.writeBit(0);
        }
        return writer.getBuffer();
    }
    auto codeMap = buildCodeMap(codes);

    BitWriter writer;
    for (uint8_t b : data) {
        const auto& bits = codeMap[b];
        writer.writeBits(bits);
    }
    return writer.getBuffer();
}

void writeDictionaryFile(std::vector<ShannonDictionaryPair>& codes, std::string& filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Не удалось открыть файл для записи словаря");
    }

    uint16_t num_codes = static_cast<uint16_t>(codes.size());
    file.write(reinterpret_cast<const char*>(&num_codes), sizeof(num_codes));

    for (const auto& pair : codes) {
        // Writing down the code length
        uint8_t code_length = static_cast<uint8_t>(pair.code.size());
        file.write(reinterpret_cast<const char*>(&code_length), sizeof(code_length));

        // Writing down the code (by bytes)
        uint8_t current_byte = 0;
        int bits_filled = 0;
        for (bool bit : pair.code) {
            current_byte = (current_byte << 1) | (bit ? 1 : 0);
            bits_filled++;
            if (bits_filled == 8) {
                file.write(reinterpret_cast<const char*>(&current_byte), sizeof(current_byte));
                current_byte = 0;
                bits_filled = 0;
            }
        }
        if (bits_filled > 0) {
            current_byte <<= (8 - bits_filled);
            file.write(reinterpret_cast<const char*>(&current_byte), sizeof(current_byte));
        }

        // Записываем байт
        file.write(reinterpret_cast<const char*>(&pair.byte), sizeof(pair.byte));
    }

    if (!file) {
        throw std::runtime_error("Ошибка при записи словаря");
    }
}

std::vector<ShannonDictionaryPair> readDictionaryFile(std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Не удалось открыть файл словаря");
    }

    // Reading the number of codes
    uint16_t num_codes;
    if (!file.read(reinterpret_cast<char*>(&num_codes), sizeof(num_codes))) {
        throw std::runtime_error("Ошибка при чтении количества кодов");
    }

    std::vector<ShannonDictionaryPair> codes;
    codes.reserve(num_codes);
    
    for (uint16_t i = 0; i < num_codes; ++i) {
        // Reading the code length
        uint8_t code_length;
        if (!file.read(reinterpret_cast<char*>(&code_length), sizeof(code_length))) {
            throw std::runtime_error("Ошибка при чтении длины кода");
        }
        
        // Reading the code
        std::vector<bool> code;
        code.reserve(code_length);
        uint8_t current_byte;
        int bits_read = 0;
        while (bits_read < code_length) {
            if (!file.read(reinterpret_cast<char*>(&current_byte), sizeof(current_byte))) {
                throw std::runtime_error("Ошибка при чтении кода");
            }
            for (int j = 7; j >= 0 && bits_read < code_length; --j) {
                code.push_back((current_byte >> j) & 1);
                bits_read++;
            }
        }
        
        // Reading the byte
        uint8_t byte;
        if (!file.read(reinterpret_cast<char*>(&byte), sizeof(byte))) {
            throw std::runtime_error("Ошибка при чтении байта");
        }
        
        codes.push_back({code, byte});
    }
    
    return codes;
}

std::vector<uint8_t> shannonDecode(const std::vector<ShannonDictionaryPair>& codes, const std::vector<uint8_t>& encodedData) {
    if (encodedData.size() < 4) return std::vector<uint8_t>();
    if (codes.empty()) return std::vector<uint8_t>();

    // Getting the total number of bits from the first four bytes
    uint32_t totalBits = static_cast<uint32_t>(encodedData[0]) |
                        (static_cast<uint32_t>(encodedData[1]) << 8) |
                        (static_cast<uint32_t>(encodedData[2]) << 16) |
                        (static_cast<uint32_t>(encodedData[3]) << 24);
    
    // Checking that we have enough data
    size_t minBytes = (totalBits + 7) / 8;
    if (encodedData.size() < minBytes + 4) {
        throw std::runtime_error("Недостаточно данных для декодирования");
    }

    // If only one byte in the dictionary, return the vector of the total bits from this byte
    if (codes.size() == 1) {
        return std::vector<uint8_t>(totalBits, codes[0].byte);
    }

    struct TrieNode {
        uint8_t byte = 0;
        bool is_terminal = false;
        std::unordered_map<bool, TrieNode*> children;
        ~TrieNode() {
            for (auto& pair : children) {
                delete pair.second;
            }
        }
    };

    TrieNode root;
    for (const auto& pair : codes) {
        TrieNode* node = &root;
        for (bool bit : pair.code) {
            if (!node->children.count(bit)) {
                node->children[bit] = new TrieNode();
            }
            node = node->children[bit];
        }
        node->byte = pair.byte;
        node->is_terminal = true;
    }

    std::vector<uint8_t> result;
    BitReader reader(encodedData);
    TrieNode* node = &root;

    try {
        for (uint32_t bits = 0; bits < totalBits; ++bits) {
            bool bit = reader.readBit();
            if (!node->children.count(bit)) {
                throw std::runtime_error("Неверный код");
            }
            node = node->children[bit];
            if (node->is_terminal) {
                result.push_back(node->byte);
                node = &root;
            }
        }
    } catch (const std::runtime_error& e) {
        if (node != &root) {
            throw std::runtime_error("Неожиданный конец данных");
        }
    }

    return result;
}

#ifdef TESTING
// Disable main when building tests
#else
int main(int argc, char* argv[])
{
    std::string mode = "e";
    std::string dictFile = "codes.txt";
    
    for (int i = 1; i < argc; i++) {
        std::string arg(argv[i]);
        if (arg == "-d") {
            mode = "d";
        } else if (arg == "-e") {
            mode = "e";
        } else if (arg == "--dict" && i + 1 < argc) {
            dictFile = argv[++i];
        } else {
            std::cerr << "Неизвестный аргумент: " << arg << std::endl;
            std::cerr << "Использование: " << argv[0] << " [-e|-d] [--dict filename]" << std::endl;
            return EXIT_FAILURE;
        }
    }

    if (mode == "d") {
        try {
            std::vector<uint8_t> buffer;
            char c;
            while (std::cin.get(c)) {
                buffer.push_back(static_cast<uint8_t>(c));
            }
            std::vector<ShannonDictionaryPair> codes = readDictionaryFile(dictFile);
            std::vector<uint8_t> decoded = shannonDecode(codes, buffer);
            std::cout.write(reinterpret_cast<const char*>(decoded.data()), decoded.size());
        } catch (const std::exception& e) {
            std::cerr << "Ошибка декодирования: " << e.what() << std::endl;
            return EXIT_FAILURE;
        }
    } else if (mode == "e") {
        try {
            std::vector<uint8_t> data;
            char c;
            while (std::cin.get(c)) {
                data.push_back(static_cast<uint8_t>(c));
            }
            auto probabilities = getProbabilityOfBytes(data);
            std::vector<ShannonDictionaryPair> codes = getOptimalDictionary(probabilities);
            writeDictionaryFile(codes, dictFile);
            std::vector<uint8_t> buffer = shannonEncode(data, codes);
            std::cout.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
        } catch (const std::exception& e) {
            std::cerr << "Ошибка кодирования: " << e.what() << std::endl;
            return EXIT_FAILURE;
        }
    }
    
    return 0;
}
#endif
