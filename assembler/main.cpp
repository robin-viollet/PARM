#include <fstream>
#include <regex>
#include <iostream>
#include <filesystem>
#include <numeric>

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
        // LSLS <Rd > , <Rm> ,# <imm5>
        // 15 14 13 12 11 | 10 9 8 7 6 | 5 4 3 | 2 1 0
        //  0  0  0  0  0 |       imm5 |    Rm |    Rd
        return 0b00000'00000'000'000;
    } else if (std::regex_match(instruction, std::regex("lsrs?"))){
        // LSRS <Rd > , <Rm> ,# <imm5>
        // 15 14 13 12 11 | 10 9 8 7 6 | 5 4 3 | 2 1 0
        //  0  0  0  0  1 |       imm5 |    Rm |    Rd
        return 0b00001'00000'000'000;
    } else if (std::regex_match(instruction, std::regex("asrs?"))){
        // ASRS <Rd > , <Rm> ,# <imm5>
        // 15 14 13 12 11 | 10 9 8 7 6 | 5 4 3 | 2 1 0
        //  0  0  0  1  0 |       imm5 |    Rm |    Rd
        return 0b00010'00000'000'000;
    } else if (std::regex_match(instruction, std::regex("adds?"))){
        // Register
        // ADDS <Rd > , < Rn > , <Rm>
        // 15 14 13 12 11 10 9 | 8 7 6 | 5 4 3 | 2 1 0
        //  0  0  0  1  1  0 0 |    Rm |    Rn |    Rd
        return 0b0001100'000'000'000;

        // Immediate
        // ADDS <Rd > , < Rn> , <#imm3>
        // 15 14 13 12 11 10 9 | 8 7 6 | 5 4 3 | 2 1 0
        //  0  0  0  1  1  1 0 |  Imm3 |    Rn |    Rd
        return 0b0001110'000'000'000;
    } else if (std::regex_match(instruction, std::regex("subs?"))){
        // Register
        // SUBS <Rd > , < Rn > , <Rm>
        // 15 14 13 12 11 10 9 | 8 7 6 | 5 4 3 | 2 1 0
        //  0  0  0  1  1  0 1 |    Rm |    Rn |    Rd
        return 0b0001101'000'000'000;

        // Immediate
        // SUBS <Rd > , < Rn> ,# <imm3>
        // 15 14 13 12 11 10 9 | 8 7 6 | 5 4 3 | 2 1 0
        //  0  0  0  1  1  1 1 |  Imm3 |    Rn |    Rd
        return 0b0001111'000'000'000;
    } else if (std::regex_match(instruction, std::regex("movs?"))){
        // MOVS <Rd> ,# <imm8>
        // 15 14 13 12 11 | 10 9 8 | 7 6 5 4 3 2 1 0
        //  0  0  1  0  0 |     Rd |            imm8
        return 0b00100'000'00000000;
    } else if (std::regex_match(instruction, std::regex("adds?"))){
        //15 14 13 12 11 10 9 | 8 7 6 | 5 4 3 | 2 1 0
        // 0  0  0  1  1  0 0 |    Rm |    Rn |    Rd
        return 0b00011'00000'000'000;
    } else if (std::regex_match(instruction, std::regex("adds?"))){
        //15 14 13 12 11 10 9 | 8 7 6 | 5 4 3 | 2 1 0
        // 0  0  0  1  1  0 0 |    Rm |    Rn |    Rd
        return 0b00011'00000'000'000;
    } else if (std::regex_match(instruction, std::regex("adds?"))){
        //15 14 13 12 11 10 9 | 8 7 6 | 5 4 3 | 2 1 0
        // 0  0  0  1  1  0 0 |    Rm |    Rn |    Rd
        return 0b00011'00000'000'000;
    } else if (std::regex_match(instruction, std::regex("adds?"))){
        //15 14 13 12 11 10 9 | 8 7 6 | 5 4 3 | 2 1 0
        // 0  0  0  1  1  0 0 |    Rm |    Rn |    Rd
        return 0b00011'00000'000'000;
    }

    // CMP <Rd> ,# <imm8>
    // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
    // 0 0 1 0 1 Rd imm8

    // ADDS <Rdn > , [ < Rdn > , ] #<imm8>
    // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
    // 0 0 1 1 0 Rd imm8

    // SUBS <Rdn > , [ < Rdn > , ] #<imm8>
    // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
    // 0 0 1 1 1 Rd imm8

    // ANDS <Rdn > , <Rm>
    // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
    // 0 1 0 0 0 0 0 0 0 0 Rm Rdn

    // EORS <Rdn > , <Rm>
    // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
    // 0 1 0 0 0 0 0 0 0 1 Rm Rdn

    // LSLS <Rdn > , <Rm>
    // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
    // 0 1 0 0 0 0 0 0 1 0 Rm Rdn

    // LSRS <Rdn > , <Rm>
    // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
    // 0 1 0 0 0 0 0 0 1 1 Rm Rdn

    // ASRS <Rdn > , <Rm>
    // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
    // 0 1 0 0 0 0 0 1 0 0 Rm Rdn

    // ADCS <Rdn > , <Rm>
    // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
    // 0 1 0 0 0 0 0 1 0 1 Rm Rdn

    // SBCS <Rdn > , <Rm>
    // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
    // 0 1 0 0 0 0 0 1 1 0 Rm Rdn

    // RORS <Rdn > , <Rm>
    // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
    // 0 1 0 0 0 0 0 1 1 1 Rm Rdn

    // TST <Rn > , <Rm>
    // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
    // 0 1 0 0 0 0 1 0 0 0 Rm Rn

    // RSBS <Rd > , < Rn> ,#0
    // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
    // 0 1 0 0 0 0 1 0 0 1 Rn Rd

    // CMP <Rn > , <Rm>
    // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
    // 0 1 0 0 0 0 1 0 1 0 Rm Rn

    // CMN <Rn > , <Rm>
    // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
    // 0 1 0 0 0 0 1 0 1 1 Rm Rn

    // ORRS <Rdn > , <Rm>
    // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
    // 0 1 0 0 0 0 1 1 0 0 Rm Rdn

    // MULS <Rdm> , < Rn > , <Rdm>
    // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
    // 0 1 0 0 0 0 1 1 0 1 Rn Rdm

    // BICS <Rdn > , <Rm>
    // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
    // 0 1 0 0 0 0 1 1 1 0 Rm Rdn

    // MVNS <Rd > , <Rm>
    // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
    // 0 1 0 0 0 0 1 1 1 1 Rm Rd

    // STR <Rt > , [ SP,# <offset> ]
    // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
    // 1 0 0 1 0 Rt imm8

    // LDR <Rt > , [ SP{ , # <offset> } ]
    // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
    // 1 0 0 1 1 Rt imm8

    // ADD [ SP , ] SP,# <offset>
    // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
    // 1 0 1 1 0 0 0 0 0 imm7

    // SUB [ SP , ] SP,# <offset>
    // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
    // 1 0 1 1 0 0 0 0 1 imm7

    // B<c> <label>
    // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
    // 1 1 0 1 cond imm8

    // B <label>
    // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
    // 1 1 1 0 0 imm11

    else if (std::regex_match(instruction, std::regex(".text"))){
        return 0;
    }

    else {
        std::string message = "unknown instruction " + instruction;

        for (const std::string& arg : args){
            message += " " + arg;
        }

        throw exception(message);
    }
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
