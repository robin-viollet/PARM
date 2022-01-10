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

std::map<std::string, int> labels;
int pc = 0;
bool debug = false;

void replace_all(std::string& str, std::string_view what, std::string_view with){
    std::string::size_type pos{};
    while (std::string::npos != (pos = str.find(what.data(), pos, what.length()))){
        str.replace(pos, what.length(), with.data(), with.length());
        pos += with.length();
    }
}

std::bitset<16> parseRegister(std::string& s){
    if (!s.empty() && s[0] == 'r'){
        return std::bitset<3>(std::stoi(s.substr(1))).to_ullong();
    }

    throw exception("register not parsed");
}

template <int T>
std::bitset<16> parseImm(std::string& s, bool sp = false){
    if (!s.empty() && s[0] == '#'){
        return std::bitset<T>(std::stoi(s.substr(1)) / (sp ? 4 : 1)).to_ullong();
    }

    throw exception("immediate not parsed");
}

std::bitset<16> parseCondition(std::string s){
    transform(s.begin(), s.end(), s.begin(), ::toupper);
    if (s == "EQ")
        return 0b000000000000'0000;
    else if (s == "NE")
        return 0b000000000000'0001;
    else if (s == "CS")
        return 0b000000000000'0010;
    else if (s == "CC" || s == "LO")
        return 0b000000000000'0011;
    else if (s == "MI")
        return 0b000000000000'0100;
    else if (s == "PL")
        return 0b000000000000'0101;
    else if (s == "VS")
        return 0b000000000000'0110;
    else if (s == "VC")
        return 0b000000000000'0111;
    else if (s == "HI")
        return 0b000000000000'1000;
    else if (s == "LS")
        return 0b000000000000'1001;
    else if (s == "GE")
        return 0b000000000000'1010;
    else if (s == "LT")
        return 0b000000000000'1011;
    else if (s == "GT")
        return 0b000000000000'1100;
    else if (s == "LE")
        return 0b000000000000'1101;
    else if (s == "AL")
        return 0b000000000000'1110;
    else
        throw exception("condition not recognized: " + s);
}

template <int T, int U, int V = std::max(T, U)>
std::bitset<V> operator|(std::bitset<T>& b1, std::bitset<U> b2){
    return std::bitset<V>(b1) |= b2;
}

template <int T>
std::bitset<16> parseLabel(const std::string& s, int source){
    return std::bitset<T>(labels[s] - source - 3).to_ullong();
}

std::bitset<16> convert_instruction(const std::string& instruction, std::vector<std::string>& args){
    if (std::regex_match(instruction, std::regex("lsls?"))){
        if (args.size() == 3){
            // Immediate (5bits)
            // LSLS <Rd > , <Rm> ,# <imm5>
            // 15 14 13 12 11 | 10 9 8 7 6 | 5 4 3 | 2 1 0
            //  0  0  0  0  0 |       imm5 |    Rm |    Rd
            return std::bitset<16>(0b00000'00000'000'000) |
                parseImm<5>(args[2]) << 6 | parseRegister(args[1]) << 3 | parseRegister(args[0]);
        } else if (args.size() == 2){
            // Registers
            // LSLS <Rdn > , <Rm>
            // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
            //  0  1  0  0  0  0 0 0 1 0    Rm   Rdn
            return std::bitset<16>(0b0100000010'000'000) |
                parseRegister(args[1]) << 3 | parseRegister(args[0]);
        }
    } else if (std::regex_match(instruction, std::regex("lsrs?"))){
        if (args.size() == 3){
            // Immediate (5bits)
            // LSRS <Rd > , <Rm> ,# <imm5>
            // 15 14 13 12 11 | 10 9 8 7 6 | 5 4 3 | 2 1 0
            //  0  0  0  0  1 |       imm5 |    Rm |    Rd
            return std::bitset<16>(0b00001'00000'000'000) |
                    parseImm<5>(args[2]) << 6 | parseRegister(args[1]) << 3 | parseRegister(args[0]);
        } else if (args.size() == 2){
            // Registers
            // LSRS <Rdn > , <Rm>
            // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
            //  0  1  0  0  0  0 0 0 1 1    Rm   Rdn
            return std::bitset<16>(0b0100000011'000'000) |
                    parseRegister(args[1]) << 3 | parseRegister(args[0]);
        }
    } else if (std::regex_match(instruction, std::regex("asrs?"))){
        if (args.size() == 3){
            // ASRS <Rd > , <Rm> ,# <imm5>
            // 15 14 13 12 11 | 10 9 8 7 6 | 5 4 3 | 2 1 0
            //  0  0  0  1  0 |       imm5 |    Rm |    Rd
            return std::bitset<16>(0b00010'00000'000'000) |
                    parseImm<5>(args[2]) << 6 | parseRegister(args[1]) << 3 | parseRegister(args[0]);
        }
    } else if (std::regex_match(instruction, std::regex("adds?"))){
        if (args[0] != "sp" && args[1] != "sp"){
            if (args.size() == 3 && args[2][0] == 'r'){
                // Registers
                // ADDS <Rd > , < Rn > , <Rm>
                // 15 14 13 12 11 10 9 | 8 7 6 | 5 4 3 | 2 1 0
                //  0  0  0  1  1  0 0 |    Rm |    Rn |    Rd
                return std::bitset<16>(0b0001100'000'000'000) |
                       parseRegister(args[2]) << 6 | parseRegister(args[1]) << 3 | parseRegister(args[0]);
            } else if (args.size() == 3 && args[2][0] == '#'){
                // Immediate (3bits)
                // ADDS <Rd > , < Rn> , <#imm3>
                // 15 14 13 12 11 10 9 | 8 7 6 | 5 4 3 | 2 1 0
                //  0  0  0  1  1  1 0 |  Imm3 |    Rn |    Rd
                return std::bitset<16>(0b0001110'000'000'000) |
                       parseImm<3>(args[2]) << 6 | parseRegister(args[1]) << 3 | parseRegister(args[0]);
            } else {
                // Immediate (8bits)
                // ADDS <Rdn > , [ < Rdn > , ] #<imm8>
                // 15 14 13 12 11 | 10 9 8 | 7 6 5 4 3 2 1 0
                //  0  0  1  1  0 |     Rd |            imm8
                return std::bitset<16>(0b00110'000'00000000) |
                       parseRegister(args[0]) << 8 | parseImm<8>(args[args.size() - 1]);
            }
        } else if (args[0] == "sp"){
            // SP + Immediate (7bits)
            // ADD [ SP , ] SP,# <offset>
            // 15 14 13 12 11 10 9 8 7 | 6 5 4 3 2 1 0
            //  1  0  1  1  0  0 0 0 0 |          imm7
            return std::bitset<16>(0b101100000'0000000) |
                   parseImm<7>(args[args.size() - 1], true);
        }
    } else if (std::regex_match(instruction, std::regex("subs?"))){
        if (args.size() == 3){
            if (args[2][0] == '#'){
                // Immediate (3bits)
                // SUBS <Rd > , < Rn> ,# <imm3>
                // 15 14 13 12 11 10 9 | 8 7 6 | 5 4 3 | 2 1 0
                //  0  0  0  1  1  1 1 |  Imm3 |    Rn |    Rd
                return std::bitset<16>(0b0001111'000'000'000) |
                       parseImm<3>(args[2]) << 6 | parseRegister(args[1]) << 3 | parseRegister(args[0]);
            } else {
                // Registers
                // SUBS <Rd > , < Rn > , <Rm>
                // 15 14 13 12 11 10 9 | 8 7 6 | 5 4 3 | 2 1 0
                //  0  0  0  1  1  0 1 |    Rm |    Rn |    Rd
                return std::bitset<16>(0b0001101'000'000'000) |
                       parseRegister(args[2]) << 6 | parseRegister(args[1]) << 3 | parseRegister(args[0]);
            }
        } else if (args.size() == 2){
            if (args[0] != "sp"){
                // Immediate (8bits)
                // SUBS <Rdn > , [ < Rdn > , ] #<imm8>
                // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
                //  0  0  1  1  1     Rd            imm8
                return std::bitset<16>(0b00111'000'00000000) |
                       parseRegister(args[0]) << 8 | parseImm<8>(args[args.size() - 1]);
            } else {
                // Immediate (7bits)
                // SUB [ SP , ] SP,# <offset>
                // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
                //  1  0  1  1  0  0 0 0 1          imm7
                return std::bitset<16>(0b101100001'0000000) | parseImm<7>(args[args.size() - 1], true);
            }
        }
    } else if (std::regex_match(instruction, std::regex("movs?"))){
        if (args.size() == 2){
            if (args[1][0] == '#'){
                // MOVS <Rd> ,# <imm8>
                // 15 14 13 12 11 | 10 9 8 | 7 6 5 4 3 2 1 0
                //  0  0  1  0  0 |     Rd |            imm8
                return std::bitset<16>(0b00100'000'00000000) |
                parseRegister(args[0]) << 8 | parseImm<8>(args[1]);
            } else {
                // TODO Check that this exists
                // MOVS <Rd> , <Rn>
                // 15 14 13 12 11 10 9 8 7 6 | 5 4 3 | 2 1 0
                //  0  0  0  0  0  0 0 0 0 0 |    Rd |    Rn
                return std::bitset<16>(0b0000000000'000'000) |
                       parseRegister(args[1]) << 3 | parseRegister(args[0]);
            }
        }
    } else if (std::regex_match(instruction, std::regex("cmp"))){
        if (args[1][0] == '#'){
            // Immediate (8bits)
            // CMP <Rd> ,# <imm8>
            // 15 14 13 12 11 | 10 9 8 | 7 6 5 4 3 2 1 0
            //  0  0  1  0  1 |     Rd |            imm8
            return std::bitset<16>(0b00101'000'00000000) |
                   parseRegister(args[0]) << 8 | parseImm<8>(args[1]);
        } else {
            // Registers
            // CMP <Rn > , <Rm>
            // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
            //  0  1  0  0  0  0 1 0 1 0    Rm    Rn
            return std::bitset<16>(0b0100001010'000'000) |
                   parseRegister(args[1]) << 3 | parseRegister(args[0]);
        }
    } else if (std::regex_match(instruction, std::regex("ands?"))){
        if (args.size() == 2){
            // ANDS <Rdn > , <Rm>
            // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
            //  0  1  0  0  0  0 0 0 0 0    Rm   Rdn
            return std::bitset<16>(0b0100000000'000'000) |
                    parseRegister(args[1]) << 3 | parseRegister(args[0]);
        }
    } else if (std::regex_match(instruction, std::regex("eors?"))){
        if (args.size() == 2){
            // EORS <Rdn > , <Rm>
            // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
            //  0  1  0  0  0  0 0 0 0 1    Rm   Rdn
            return std::bitset<16>(0b0100000001'000'000) |
                    parseRegister(args[1]) << 3 | parseRegister(args[0]);
        }
    } else if (std::regex_match(instruction, std::regex("asrs?"))){
        if (args.size() == 2){
            // ASRS <Rdn > , <Rm>
            // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
            //  0  1  0  0  0  0 0 1 0 0    Rm   Rdn
            return std::bitset<16>(0b01000000100'000'000) |
                    parseRegister(args[1]) << 3 | parseRegister(args[0]);
        }
    } else if (std::regex_match(instruction, std::regex("adcs?"))){
        if (args.size() == 2){
            // ADCS <Rdn > , <Rm>
            // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
            //  0  1  0  0  0  0 0 1 0 1    Rm   Rdn
            return std::bitset<16>(0b0100000101'000'000) |
                    parseRegister(args[1]) << 3 | parseRegister(args[0]);
        }
    } else if (std::regex_match(instruction, std::regex("sbcs?"))){
        if (args.size() == 2){
            // SBCS <Rdn > , <Rm>
            // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
            //  0  1  0  0  0  0 0 1 1 0    Rm   Rdn
            return std::bitset<16>(0b0100000110'000'000) |
                    parseRegister(args[1]) << 3 | parseRegister(args[0]);
        }
    } else if (std::regex_match(instruction, std::regex("rors"))){
        if (args.size() == 2){
            // RORS <Rdn > , <Rm>
            // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
            //  0  1  0  0  0  0 0 1 1 1    Rm   Rdn
            return std::bitset<16>(0b0100000111'000'000) |
                    parseRegister(args[1]) << 3 | parseRegister(args[0]);
        }
    } else if (std::regex_match(instruction, std::regex("tst"))){
        if (args.size() == 2){
            // TST <Rn > , <Rm>
            // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
            //  0  1  0  0  0  0 1 0 0 0    Rm    Rn
            return std::bitset<16>(0b0100001000'000'000) |
                    parseRegister(args[1]) << 3 | parseRegister(args[0]);
        }
    } else if (std::regex_match(instruction, std::regex("rsbs?"))){
        if (args.size() == 3){
            // RSBS <Rd > , < Rn> ,#0
            // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
            //  0  1  0  0  0  0 1 0 0 1    Rn    Rd
            return std::bitset<16>(0b0100001001'000'000) |
                    parseRegister(args[1]) << 3 | parseRegister(args[0]);
        }
    } else if (std::regex_match(instruction, std::regex("cmn"))){
        if (args.size() == 2){
            // CMN <Rn > , <Rm>
            // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
            //  0  1  0  0  0  0 1 0 1 1    Rm    Rn
            return std::bitset<16>(0b0100001011'000'000) |
                    parseRegister(args[1]) << 3 | parseRegister(args[0]);
        }
    } else if (std::regex_match(instruction, std::regex("orrs?"))){
        if (args.size() == 2){
            // ORRS <Rdn > , <Rm>
            // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
            //  0  1  0  0  0  0 1 1 0 0    Rm   Rdn
            return std::bitset<16>(0b0100001100'000'000) |
                    parseRegister(args[1]) << 3 | parseRegister(args[0]);
        }
    } else if (std::regex_match(instruction, std::regex("muls?"))){
        if (args.size() == 3){
            // MULS <Rdm> , < Rn > , <Rdm>
            // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
            //  0  1  0  0  0  0 1 1 0 1    Rn   Rdm
            return std::bitset<16>(0b0100001101'000'000) |
                    parseRegister(args[1]) << 3 | parseRegister(args[0]);
        }
    } else if (std::regex_match(instruction, std::regex("bics?"))){
        if (args.size() == 2){
            // BICS <Rdn > , <Rm>
            // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
            //  0  1  0  0  0  0 1 1 1 0    Rm   Rdn
            return std::bitset<16>(0b0100001110'000'000) |
                    parseRegister(args[1]) << 3 | parseRegister(args[0]);
        }
    } else if (std::regex_match(instruction, std::regex("mvns?"))){
        if (args.size() == 2){
            // MVNS <Rd > , <Rm>
            // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
            //  0  1  0  0  0  0 1 1 1 1    Rm    Rd
            return std::bitset<16>(0b0100001111'000'000) |
                    parseRegister(args[1]) << 3 | parseRegister(args[0]);
        }
    } else if (std::regex_match(instruction, std::regex("str"))){
        // STR <Rt > , [ SP,# <offset> ]
        // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
        //  1  0  0  1  0     Rt            imm8
        std::bitset<16> bin(0b10010'000'00000000);

        if (args.size() >= 3){
            return bin | parseRegister(args[0]) << 8 |
                   std::bitset<16>(std::bitset<8>(parseImm<16>(args[2]).to_ullong() / 4).to_ullong());
        } else {
            return bin | parseRegister(args[0]) << 8;
        }
    } else if (std::regex_match(instruction, std::regex("ldr"))){
        // LDR <Rt > , [ SP{ , # <offset> } ]
        // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
        //  1  0  0  1  1     Rt            imm8
        if (args.size() >= 3){
            return std::bitset<16>(0b10011'000'00000000) |
                   parseRegister(args[0]) << 8 |
                   std::bitset<16>(std::bitset<8>(parseImm<16>(args[2]).to_ullong() / 4).to_ullong());
        } else {
            return std::bitset<16>(0b10011'000'00000000) |
                   parseRegister(args[0]) << 8;
        }
    } else if (std::regex_match(instruction, std::regex("cmp?"))){
        if (args[1][0] == '#'){
            // CMP <Rd> ,# <imm8>
            // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
            //  0  0  1  0  1     Rd            imm8
            return std::bitset<16>(0b00101'000'00000000) |
                   parseRegister(args[0]) << 8 |
                   parseImm<8>(args[1]);
        } else {
            // CMP <Rn > , <Rm>
            // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
            //  0  1  0  0  0  0 1 0 1 0    Rm    Rn
            return std::bitset<16>(0b0100001010'000'000) |
                   parseRegister(args[1]) << 3 |
                   parseRegister(args[0]);
        }
    } else if (std::regex_match(instruction, std::regex("b.{0,2}"))){
        if (instruction == "b" || instruction == "bx"){
            // Immediate (11bits)
            // B <label>
            // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
            //  1  1  1  0  0                  imm11
            return std::bitset<16>(0b11100'00000000000) |
                    parseLabel<11>(args[0], pc);
        } else {
            // Immediate (8bits)
            // B<c> <label>
            // 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
            //  1  1  0  1      cond            imm8
            return std::bitset<16>(0b1101'0000'00000000) |
                    parseCondition(instruction.substr(1)) << 8 |
                    parseLabel<8>(args[0], pc);
        }
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
    regex labelRegex("^.*:$");
    regex instructionRegex("^\t?[^.@\t].*[^:]$");
    string reason;

    try {

        if (argc > 1){

            path inFile = path(argv[1]);

            if (regex_match(inFile.string(), extensionRegex)){

                path outFile = regex_replace(inFile.string(), extensionRegex, "$1.bin");

                ifstream in(inFile, std::ios::in);
                ofstream out(outFile, std::ios::out);

                if (in){

                    std::cout << "assembling in file " << outFile.string() << std::endl;

                    std::vector<std::string> lines;

                    if (!debug){

                        out << "v2.0 raw" << std::endl;

                    }

                    {
                        std::string line;

                        while (std::getline(in, line)){
                            lines.push_back(line);

                            if (regex_match(line, labelRegex)){

                                std::string label = line.substr(0, line.find(':'));
                                labels[label] = pc;
                                //labels[label] = pc + 1;
                                std::cout << "label: " << label << "(" << labels[label] << ")" << std::endl;

                            } else if (regex_match(line, instructionRegex)){

                                ++pc;

                            }
                        }
                    }

                    pc = 0;

                    for (std::string& line : lines){
                        replace_all(line, ", ", ",");
                        replace_all(line, "[", "");
                        replace_all(line, "]", "");
                        std::istringstream iss(line);

                        if (regex_match(line, instructionRegex)){

                            std::vector<std::string> words{
                                    std::istream_iterator<std::string>(iss),
                                    std::istream_iterator<std::string>()};

                            std::string instruction = words[0];
                            words.erase(words.begin());

                            std::istringstream argsStream(words[0]);
                            std::string arg;
                            std::vector<string> args;

                            while (std::getline(argsStream, arg, ',')){
                                args.push_back(arg);
                            }

                            try {

                                if (debug){

                                    std::cout << std::hex << convert_instruction(instruction, args).to_ullong()
                                              << std::endl;

                                } else {

                                    out << std::setfill('0') << std::hex << std::setw(4)
                                        << convert_instruction(instruction, args).to_ullong() << ' ';

                                }

                            } catch (exception& e){
                                std::cerr << e.what() << std::endl;
                            }

                            std::cout << instruction << '(';

                            for (const auto& x: args)
                                std::cout << x << ',';

                            std::cout << ')' << std::endl;

                            ++pc;

                        }
                    }

                    if (!debug){

                        out << std::endl;

                    }

                    std::cout << "avengers assembled in " << outFile << " in " <<
                              duration_cast<seconds>(high_resolution_clock::now() - startTime).count() << "s!"
                              << std::endl;

                } else {

                    throw exception("file " + path(inFile).filename().string() + " being non-existent.");

                }

            } else {

                throw exception("file " + path(inFile).filename().string() +
                    " not being in assembly format (*.s).");

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
