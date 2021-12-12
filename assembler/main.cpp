#include <fstream>
#include <regex>
#include <iostream>
#include <filesystem>

#include "exception.hpp"

using namespace std::filesystem;
using std::filesystem::path;
using std::string;
using std::regex;
using std::ifstream;
using std::ofstream;
using std::chrono::duration_cast;
using std::chrono::seconds;
using std::chrono::high_resolution_clock;

void replace_all(std::string& str, std::string_view what, std::string_view with){
    std::string::size_type pos{};
    while (std::string::npos != (pos = str.find(what.data(), pos, what.length()))){
        str.replace(pos, what.length(), with.data(), with.length());
        pos += with.length();
    }
}

unsigned short convert_instruction(const std::string& instruction, std::vector<std::string>& args){
    if (std::regex_match(instruction, std::regex("lsls?"))){
        // Immediate (5bits)
        // LSLS <Rd > , <Rm> ,# <imm5>
        // 15 14 13 12 11 | 10 9 8 7 6 | 5 4 3 | 2 1 0
        //  0  0  0  0  0 |       imm5 |    Rm |    Rd
        return 0b00000'00000'000'000;

        // Registers
        // LSLS <Rdn > , <Rm>
        // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
        //  0  1  0  0  0  0 0 0 1 0    Rm   Rdn
        return 0b0100000010'000000;
    } else if (std::regex_match(instruction, std::regex("lsrs?"))){
        // Immediate (5bits)
        // LSRS <Rd > , <Rm> ,# <imm5>
        // 15 14 13 12 11 | 10 9 8 7 6 | 5 4 3 | 2 1 0
        //  0  0  0  0  1 |       imm5 |    Rm |    Rd
        return 0b00001'00000'000'000;

        // Registers
        // LSRS <Rdn > , <Rm>
        // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
        //  0  1  0  0  0  0 0 0 1 1    Rm   Rdn
        return 0b0100000011'000'000;
    } else if (std::regex_match(instruction, std::regex("asrs?"))){
        // ASRS <Rd > , <Rm> ,# <imm5>
        // 15 14 13 12 11 | 10 9 8 7 6 | 5 4 3 | 2 1 0
        //  0  0  0  1  0 |       imm5 |    Rm |    Rd
        return 0b00010'00000'000'000;
    } else if (std::regex_match(instruction, std::regex("adds?"))){
        // Registers
        // ADDS <Rd > , < Rn > , <Rm>
        // 15 14 13 12 11 10 9 | 8 7 6 | 5 4 3 | 2 1 0
        //  0  0  0  1  1  0 0 |    Rm |    Rn |    Rd
        return 0b0001100'000'000'000;

        // Immediate (3bits)
        // ADDS <Rd > , < Rn> , <#imm3>
        // 15 14 13 12 11 10 9 | 8 7 6 | 5 4 3 | 2 1 0
        //  0  0  0  1  1  1 0 |  Imm3 |    Rn |    Rd
        return 0b0001110'000'000'000;

        // Immediate (8bits)
        // ADDS <Rdn > , [ < Rdn > , ] #<imm8>
        // 15 14 13 12 11 | 10 9 8 | 7 6 5 4 3 2 1 0
        //  0  0  1  1  0 |     Rd |            imm8
        return 0b00110'000'00000000;
    } else if (std::regex_match(instruction, std::regex("subs?"))){
        // Registers
        // SUBS <Rd > , < Rn > , <Rm>
        // 15 14 13 12 11 10 9 | 8 7 6 | 5 4 3 | 2 1 0
        //  0  0  0  1  1  0 1 |    Rm |    Rn |    Rd
        return 0b0001101'000'000'000;

        // Immediate (3bits)
        // SUBS <Rd > , < Rn> ,# <imm3>
        // 15 14 13 12 11 10 9 | 8 7 6 | 5 4 3 | 2 1 0
        //  0  0  0  1  1  1 1 |  Imm3 |    Rn |    Rd
        return 0b0001111'000'000'000;

        // Immediate (8bits)
        // SUBS <Rdn > , [ < Rdn > , ] #<imm8>
        // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
        //  0  0  1  1  1     Rd            imm8
        return 0b00111'000'00000000;

        // Immediate (7bits)
        // SUB [ SP , ] SP,# <offset>
        // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
        //  1  0  1  1  0  0 0 0 1          imm7
        return 0b101100001'0000000;
    } else if (std::regex_match(instruction, std::regex("movs?"))){
        // MOVS <Rd> ,# <imm8>
        // 15 14 13 12 11 | 10 9 8 | 7 6 5 4 3 2 1 0
        //  0  0  1  0  0 |     Rd |            imm8
        return 0b00100'000'00000000;
    } else if (std::regex_match(instruction, std::regex("cmp"))){
        // Immediate (8bits)
        // CMP <Rd> ,# <imm8>
        // 15 14 13 12 11 | 10 9 8 | 7 6 5 4 3 2 1 0
        //  0  0  1  0  1 |     Rd |            imm8
        return 0b00101'000'00000000;

        // Registers
        // CMP <Rn > , <Rm>
        // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
        //  0  1  0  0  0  0 1 0 1 0    Rm    Rn
        return 0b0100001010'000'000;
    } else if (std::regex_match(instruction, std::regex("ands?"))){
        // ANDS <Rdn > , <Rm>
        // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
        //  0  1  0  0  0  0 0 0 0 0    Rm   Rdn
        return 0b0100000000'000'000;
    } else if (std::regex_match(instruction, std::regex("eors?"))){
        // EORS <Rdn > , <Rm>
        // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
        //  0  1  0  0  0  0 0 0 0 1    Rm   Rdn
        return 0b0100000001'000'000;
    } else if (std::regex_match(instruction, std::regex("asrs?"))){
        // ASRS <Rdn > , <Rm>
        // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
        //  0  1  0  0  0  0 0 1 0 0    Rm   Rdn
        return 0b01000000100'000'000;
    } else if (std::regex_match(instruction, std::regex("adcs?"))){
        // ADCS <Rdn > , <Rm>
        // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
        //  0  1  0  0  0  0 0 1 0 1    Rm   Rdn
        return 0b0100000101'000'000;
    } else if (std::regex_match(instruction, std::regex("sbcs?"))){
        // SBCS <Rdn > , <Rm>
        // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
        //  0  1  0  0  0  0 0 1 1 0    Rm   Rdn
        return 0b0100000110'000'000;
    } else if (std::regex_match(instruction, std::regex("rors"))){
        // RORS <Rdn > , <Rm>
        // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
        //  0  1  0  0  0  0 0 1 1 1    Rm   Rdn
        return 0b0100000111'000'000;
    } else if (std::regex_match(instruction, std::regex("tst"))){
        // TST <Rn > , <Rm>
        // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
        //  0  1  0  0  0  0 1 0 0 0    Rm    Rn
        return 0b0100001000'000'000;
    } else if (std::regex_match(instruction, std::regex("rsbs?"))){
        // RSBS <Rd > , < Rn> ,#0
        // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
        //  0  1  0  0  0  0 1 0 0 1    Rn    Rd
        return 0b0100001001'000'000;
    } else if (std::regex_match(instruction, std::regex("cmn"))){
        // CMN <Rn > , <Rm>
        // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
        //  0  1  0  0  0  0 1 0 1 1    Rm    Rn
        return 0b0100001011'000'000;
    } else if (std::regex_match(instruction, std::regex("orrs?"))){
        // ORRS <Rdn > , <Rm>
        // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
        //  0  1  0  0  0  0 1 1 0 0    Rm   Rdn
        return 0b0100001100'000'000;
    } else if (std::regex_match(instruction, std::regex("muls?"))){
        // MULS <Rdm> , < Rn > , <Rdm>
        // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
        //  0  1  0  0  0  0 1 1 0 1    Rn   Rdm
        return 0b0100001101'000'000;
    } else if (std::regex_match(instruction, std::regex("bics?"))){
        // BICS <Rdn > , <Rm>
        // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
        //  0  1  0  0  0  0 1 1 1 0    Rm   Rdn
        return 0b0100001110'000'000;
    } else if (std::regex_match(instruction, std::regex("mvns?"))){
        // MVNS <Rd > , <Rm>
        // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
        //  0  1  0  0  0  0 1 1 1 1    Rm    Rd
        return 0b0100001111'000'000;
    } else if (std::regex_match(instruction, std::regex("str"))){
        // STR <Rt > , [ SP,# <offset> ]
        // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
        //  1  0  0  1  0     Rt            imm8
        return 0b10010'000'00000000;
    } else if (std::regex_match(instruction, std::regex("ldr"))){
        // LDR <Rt > , [ SP{ , # <offset> } ]
        // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
        //  1  0  0  1  1     Rt            imm8
        return 0b10011'000'00000000;
    } else if (std::regex_match(instruction, std::regex("cmp?"))){
        // ADD [ SP , ] SP,# <offset>
        // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
        //  1  0  1  1  0  0 0 0 0          imm7
        return 0b101100000'0000000;
    } else if (std::regex_match(instruction, std::regex("b?"))){
        // Immediate (8nits)
        // B<c> <label>
        // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
        //  1  1  0  1      cond            imm8
        return 0b1101'0000'00000000;

        // Immediate (11bits)
        // B <label>
        // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
        //  1  1  1  0  0                  imm11
        return 0b11100'00000000000;
    }

    std::string message = "unknown instruction " + instruction;

    for (const std::string& arg: args){
        message += " " + arg;
    }

    throw exception(message);
}

int main(int argc, char** argv){

    std::cout << "avengers assemble!" << std::endl;

    auto startTime = high_resolution_clock::now();
    regex extensionRegex("^(.*)[.]s$");
    regex tabRegex("^\t.*$");
    regex startsWithPointRegex("^\t?[.].*$");
    string reason;

    try {

        if (argc > 1){

            path inFile = path(argv[1]);

            if (regex_match(inFile.string(), extensionRegex)){

                path outFile = regex_replace(inFile.string(), extensionRegex, "$1.bin");

                ifstream in(inFile, std::ios::in);
                ofstream out(outFile, std::ios::out);

                std::cout << "assembling in file " << outFile.string() << std::endl;

                std::string line;
                std::string label;

                while (std::getline(in, line)){
                    replace_all(line, ", ", ",");
                    std::istringstream iss(line);

                    bool point = regex_match(line, startsWithPointRegex);

                    std::cout << "newline" << (point ? " . point here" : "") << std::endl;

                    if (regex_match(line, tabRegex)){

                        std::vector<std::string> words {
                                std::istream_iterator<std::string>(iss),
                                std::istream_iterator<std::string>()};

                        std::string instruction = words[0];
                        words.erase(words.begin());
                        std::cout << std::hex << convert_instruction(instruction, words) << std::endl;

                        for (const auto& x : words)
                            std::cout << "word: " << x << std::endl;

                    } else {

                        label = line.substr(0, line.find(':'));
                        std::cout << "label: " << label << std::endl;

                    }
                }


                //out << "2.0 raw" << std::endl;
                std::cout << "avengers assembled in " << outFile << " in " << duration_cast<seconds>(high_resolution_clock::now() - startTime).count() << "s!" << std::endl;

            } else {

                throw exception("file " + path(inFile).filename().string() + " not being in assembly format (*.s).");

            }

        } else {

            throw exception("lack of argument.");

        }

    } catch (exception& e){

        std::cerr << "avengers didn't assemble due to " << e.what() << std::endl;
        return 1;

    } catch (...){

        std::cerr << "avengers didn't assemble due to unknown error." << std::endl;
        return 1;

    }

    return 0;
}
