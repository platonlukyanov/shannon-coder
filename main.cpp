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
    size_t totalBits = 0;

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
        // Всегда два байта для длины (младший, старший)
        result.push_back(static_cast<uint8_t>(totalBits & 0xFF));
        result.push_back(static_cast<uint8_t>((totalBits >> 8) & 0xFF));
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
    size_t byteIndex = 2;
    int bitIndex = 7;
    size_t totalBits = 0;
    size_t bitsRead = 0;

public:
    BitReader(const std::vector<uint8_t>& buf) : buffer(buf) {
        if (buffer.size() < 2) {
            totalBits = 0;
        } else {
            totalBits = static_cast<size_t>(buffer[0]) | (static_cast<size_t>(buffer[1]) << 8);
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
    if (data.empty() || codes.empty()) return std::vector<uint8_t>{0, 0};
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

    for (const auto& pair : codes) {
        // Записываем длину кода (1 байт)
        file << static_cast<uint8_t>(pair.code.size());
        // Записываем код (по байтам)
        uint8_t current_byte = 0;
        int bits_filled = 0;
        for (bool bit : pair.code) {
            current_byte = (current_byte << 1) | (bit ? 1 : 0);
            bits_filled++;
            if (bits_filled == 8) {
                file << current_byte;
                current_byte = 0;
                bits_filled = 0;
            }
        }
        if (bits_filled > 0) {
            current_byte <<= (8 - bits_filled);
            file << current_byte;
        }
        // Записываем байт
        file << pair.byte;
    }
}

std::vector<ShannonDictionaryPair> readDictionaryFile(std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    std::vector<ShannonDictionaryPair> codes;
    
    while (file) {
        // Читаем длину кода
        uint8_t code_length;
        if (!file.read(reinterpret_cast<char*>(&code_length), 1)) break;
        
        // Читаем код
        std::vector<bool> code;
        uint8_t current_byte;
        int bits_read = 0;
        while (bits_read < code_length) {
            if (!file.read(reinterpret_cast<char*>(&current_byte), 1)) break;
            for (int i = 7; i >= 0 && bits_read < code_length; --i) {
                code.push_back((current_byte >> i) & 1);
                bits_read++;
            }
        }
        
        // Читаем байт
        uint8_t byte;
        if (!file.read(reinterpret_cast<char*>(&byte), 1)) break;
        
        codes.push_back({code, byte});
    }
    
    return codes;
}

std::vector<uint8_t> shannonDecode(const std::vector<ShannonDictionaryPair>& codes, const std::vector<uint8_t>& encodedData) {
    if (encodedData.size() < 2) return std::vector<uint8_t>();
    if (codes.empty()) return std::vector<uint8_t>();
    // Если только один байт в словаре, возвращаем вектор длины totalBits из этого байта
    if (codes.size() == 1) {
        size_t totalBits = static_cast<size_t>(encodedData[0]) | (static_cast<size_t>(encodedData[1]) << 8);
        return std::vector<uint8_t>(totalBits, codes[0].byte);
    }
    struct TrieNode {
        uint8_t byte = 0;
        std::unordered_map<bool, TrieNode*> children;
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
    }

    std::vector<uint8_t> result;
    BitReader reader(encodedData);
    TrieNode* node = &root;

    try {
        while (true) {
            bool bit = reader.readBit();
            if (!node->children.count(bit)) {
                throw std::runtime_error("Invalid code");
            }
            node = node->children[bit];
            if (node->byte != 0) {
                result.push_back(node->byte);
                node = &root;
            }
        }
    } catch (const std::runtime_error& e) {
        if (node != &root) {
            throw std::runtime_error("Unexpected end of data");
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
    
    // Парсинг аргументов командной строки
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
            // Читаем закодированные данные из stdin (бинарно)
            std::vector<uint8_t> buffer;
            char c;
            while (std::cin.get(c)) {
                buffer.push_back(static_cast<uint8_t>(c));
            }
            // Читаем словарь из файла
            std::vector<ShannonDictionaryPair> codes = readDictionaryFile(dictFile);
            // Декодируем и выводим результат
            std::vector<uint8_t> decoded = shannonDecode(codes, buffer);
            std::cout.write(reinterpret_cast<const char*>(decoded.data()), decoded.size());
        } catch (const std::exception& e) {
            std::cerr << "Ошибка декодирования: " << e.what() << std::endl;
            return EXIT_FAILURE;
        }
    } else if (mode == "e") {
        try {
            // Читаем данные для кодирования из stdin (бинарно)
            std::vector<uint8_t> data;
            char c;
            while (std::cin.get(c)) {
                data.push_back(static_cast<uint8_t>(c));
            }
            // Получаем вероятности и строим словарь
            auto probabilities = getProbabilityOfBytes(data);
            std::vector<ShannonDictionaryPair> codes = getOptimalDictionary(probabilities);
            // Сохраняем словарь в файл
            writeDictionaryFile(codes, dictFile);
            // Кодируем данные
            std::vector<uint8_t> buffer = shannonEncode(data, codes);
            // Выводим закодированные данные в stdout (бинарно)
            std::cout.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
        } catch (const std::exception& e) {
            std::cerr << "Ошибка кодирования: " << e.what() << std::endl;
            return EXIT_FAILURE;
        }
    }
    
    return 0;
}
#endif
