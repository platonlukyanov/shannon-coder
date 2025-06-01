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

struct ShannonProbabilityCharPair {
	char character;
	double probability;
};

ShannonProbabilityCharPair getProbabilityOfAppearenceForSymbol(char character, std::string& text) {
	size_t count = 0;
	for (size_t i = 0; i < text.size(); ++i) {
		if (text[i] == character) {
			++count;
		}
	}

	return {character, static_cast<double>(count) / text.size()};
}

std::vector<ShannonProbabilityCharPair> getProbabilityOfSymbols(std::string& text) {
    std::unordered_map<char, size_t> countMap;
    for (char c : text) countMap[c]++;
    
    std::vector<ShannonProbabilityCharPair> result;
    for (const auto& pair : countMap) {
        result.push_back({pair.first, 
            static_cast<double>(pair.second)/text.size()});
    }
    
    std::sort(result.begin(), result.end(), 
        [](const auto& a, const auto& b) {
            return a.probability > b.probability;
        });
    
    return result;
}


struct ShannonDictionaryPair {
	std::vector<bool> code;
	char character;
};

std::vector<ShannonDictionaryPair> getOptimalDictionary(const std::vector<ShannonProbabilityCharPair>& probabilities) {
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
        codes.push_back({code, symbol.character});
        cumulativeProb += p;
    }
    return codes;
}

std::unordered_map<char, std::vector<bool>> buildCodeMap(const std::vector<ShannonDictionaryPair>& codes) {
    std::unordered_map<char, std::vector<bool>> codeMap;
    for (const auto& pair : codes) {
        codeMap[pair.character] = pair.code;
    }
    return codeMap;
}
std::vector<uint8_t> shannonEncode(std::string& text, std::vector<ShannonDictionaryPair>& codes) {
    if (text.empty() || codes.empty()) return std::vector<uint8_t>{0, 0};
    // Если только один символ в словаре, кодируем каждый символ одним битом (0)
    if (codes.size() == 1) {
        BitWriter writer;
        for (size_t i = 0; i < text.size(); ++i) {
            writer.writeBit(0);
        }
        return writer.getBuffer();
    }
    auto codeMap = buildCodeMap(codes);

    BitWriter writer;
    for (char c : text) {
        const auto& bits = codeMap[c];
        writer.writeBits(bits);
    }
    return writer.getBuffer();
}

void writeDictionaryFile(std::vector<ShannonDictionaryPair>& codes) {
    std::ofstream file("codes.txt");
    for (const auto& pair : codes) {
        for (bool bit : pair.code) {
            file << (bit ? '1' : '0');
        }
        file << ' ' << pair.character << '\n';
    }
}

std::vector<ShannonDictionaryPair> readDictionaryFile() {
    std::ifstream file("codes.txt");
    std::vector<ShannonDictionaryPair> codes;
    std::string line;
    
    while (std::getline(file, line)) {
        size_t space_pos = line.find(' ');
        if (space_pos == std::string::npos) continue;
        
        std::vector<bool> code;
        for (char c : line.substr(0, space_pos)) {
            code.push_back(c == '1');
        }
        char character = line[space_pos+1];
        codes.push_back({code, character});
    }
    
    return codes;
}


std::string shannonDecode(const std::vector<ShannonDictionaryPair>& codes, const std::vector<uint8_t>& encodedData) {
    if (encodedData.size() < 2) return "";
    if (codes.empty()) return "";
    // Если только один символ в словаре, возвращаем строку длины totalBits из этого символа
    if (codes.size() == 1) {
        size_t totalBits = static_cast<size_t>(encodedData[0]) | (static_cast<size_t>(encodedData[1]) << 8);
        return std::string(totalBits, codes[0].character);
    }
    struct TrieNode {
        char character = 0;
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
        node->character = pair.character;
    }

    std::string result;
    BitReader reader(encodedData);
    TrieNode* node = &root;

    try {
        while (true) {
            bool bit = reader.readBit();
            if (!node->children.count(bit)) {
                throw std::runtime_error("Invalid code");
            }
            node = node->children[bit];
            if (node->character != 0) {
                result += node->character;
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
	std::string txt = "abracadabra";
	auto probabilities = getProbabilityOfSymbols(txt);
	std::vector<ShannonDictionaryPair> codes = getOptimalDictionary(probabilities);

	writeDictionaryFile(codes);

	std::vector<uint8_t> buffer = shannonEncode(txt, codes);
	std::cout << "Buffer: " << std::endl;

	for (uint8_t b : buffer) {
		std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)b << std::endl;
	}

	std::cout << "Decoding: " << shannonDecode(codes, buffer) << std::endl;

	std::cout << std::endl;

	std::string mode = "e";
	if (argc > 1) {
		std::string arg(argv[1]);
		
		if (arg == "-d") {
			mode = "d";
		}
	}

	if (mode == "d") {
		try {
			std::string buffer = readString();
			std::cout << "Decoding: " << buffer << std::endl;
		} catch (const std::exception& e) {
			std::cerr << "Decoding Error: " << e.what() << std::endl;
			return EXIT_FAILURE;
		}
	}
	
	if (mode == "e") {
		try {
			std::cout << "Buffer: " << std::endl;
		} catch (const std::exception& e) {
			std::cerr << "Encoding Error: " << e.what() << std::endl;
			return EXIT_FAILURE;
		}
	}

	
	return 0;
}
#endif
