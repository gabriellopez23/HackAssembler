#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <map>
#include <cctype>
using namespace std;

class HackAssembler{
    private:
        short instructionNum, symbolAddress;
        string command, label, binaryOutput;
        ifstream asmFile;
        ofstream hackFile;
        
        //Maps for each set of instructions and symbols
        map<string, string> destMap, compMap, jumpMap, 
                            symbolsMap;

    public:
        HackAssembler(string);
        void addLabels();
        void assembleFile();
        string convertToBinary(string);
        string cInstruction();
        string aInstruction();
        bool verifyIfNumber();
};

HackAssembler::HackAssembler(string fileName){

    //Opening input file and creating output file with .hack extension
    asmFile.open(fileName);
    hackFile.open(fileName.substr(0, fileName.find('.')) + ".hack.txt"); 

    instructionNum = -1;
    symbolAddress = 16;
    
    //All symbols and instructions are stored as binary string to save time
    destMap = { {"0", "000"},  {"M", "001"},
                {"D", "010"},  {"MD", "011"},
                {"A", "100"},  {"AM", "101"},
                {"AD", "110"}, {"AMD", "111"}
    };

    compMap = { {"0", "0101010"},   {"1", "0111111"},   {"-1", "0111010"},
                {"D", "0001100"},   {"A", "0110000"},   {"!D", "0001101"},
                {"!A", "0110001"},  {"-D", "0001111"},  {"-A", "0110011"},
                {"D+1", "0011111"}, {"A+1", "0110111"}, {"D-1", "0001110"},
                {"A-1", "0110010"}, {"D+A", "0000010"}, {"D-A", "0010011"},
                {"A-D", "0000111"}, {"A&D", "0000000"}, {"D|A", "0010101"},
                {"M", "1110000"},   {"!M", "1110001"},  {"-M", "1110011"},
                {"M+1", "1110111"}, {"M-1", "1110010"}, {"D+M", "1000010"},
                {"D-M", "1010011"}, {"M-D", "1000111"}, {"D&M", "1000000"},
                {"D|M", "1010101"}
    };

    jumpMap = { {"0", "000"},  {"JGT", "001"},
                {"JEQ", "010"}, {"JGE", "011"},
                {"JLT", "100"}, {"JNE", "101"},
                {"JLE", "110"}, {"JMP", "111"}
    };

    symbolsMap = {  {"R0", "0000000000000000"},   {"R1", "0000000000000001"},   {"R2", "0000000000000010"},   {"R3", "0000000000000011"},   
                    {"R4", "0000000000000100"},   {"R5", "0000000000000101"},   {"R6", "0000000000000110"},   {"R7", "0000000000000111"},   
                    {"R8", "0000000000001000"},   {"R9", "0000000000001001"},   {"R10", "0000000000001010"},  {"R11", "0000000000001011"}, 
                    {"R12", "0000000000001100"},  {"R13", "0000000000001101"},  {"R14", "0000000000001110"},  {"R15", "0000000000001111"}, 
                    {"SP", "0000000000000000"},   {"LCL", "0000000000000001"},  {"ARG", "0000000000000010"},  {"THIS", "0000000000000011"},    
                    {"THAT", "0000000000000100"}, {"SCREEN", "0100000000000000"}, {"KBD", "0110000000000000"}
    };

    addLabels();    //Record all labels first
};

void HackAssembler::addLabels(){

    /*First extracts all labels and adds them to symbolsMap for later use.
      This is necessary to avoid issues and simplify symbol table lookup.*/

    while(getline(asmFile, label)){
    
        label.erase(remove_if(label.begin(), label.end(), ::isspace), label.end());

        if(label[0] == '@' || isalnum(label[0])) 
            ++instructionNum;                                       //Only counts up if it is an instruction (labels, empty lines, and comments don't count)

        else if(label[0] == '(')                                    //Only stores in symbolsMap if it is a label ()
            symbolsMap.emplace(label.substr(1, label.find_first_of(')')-1), convertToBinary(to_string(instructionNum+1))); 
        
    }

    label.~basic_string();                                          //Destroys the string once it is no longer in use to conserve memory
    assembleFile();                                                 //Proceeds to assemble the entire file with recorded symbols
};

void HackAssembler::assembleFile(){

    asmFile.clear();                                                //Restarts reading the asm file
    asmFile.seekg(0);

    while(getline(asmFile, command)){                               //Gets line and removes spaces for easy character identification
        command.erase(remove_if(command.begin(), command.end(), ::isspace), command.end());

        if(command[0] == '@')                             
            hackFile << aInstruction() + '\n';
        else if(isalnum(command[0]))
            hackFile << cInstruction() + '\n';
        
    }
};

string HackAssembler::aInstruction(){
    /*Converts the A-instruction mnemonics into a binary
      string. First, checks to remove comments from line 
      to save time, memory, and avoid bugs. If there is 
      no comment, then it only removes the @ from the line*/

    short slashLocation = (short) command.find_first_of('/');
    command = command.substr(1, (slashLocation != -1 ? slashLocation-1 : command.length()));
    
    if(verifyIfNumber()){                                           //Checks to see if the instruction is a symbol or a number
        return convertToBinary(command);                            //Converts it immediately to binary if it is a number
    
    }else{                                                          //Iterator to see if certain symbol exists in symbolsMap; gets added if it doesn't
        map<string, string>::iterator itr = symbolsMap.find(command.substr());
        
        if(itr != symbolsMap.end()){                                //If it exists, simply return the binary value        
            return itr->second;
        }else{                                                      //If it doesn't, gets added and is then returned
            symbolsMap.emplace(command, convertToBinary(to_string(symbolAddress++)));
            return symbolsMap.find(command)->second;
        }
    }
};

string HackAssembler::cInstruction(){

    /*Converts the C-instructoin mnemonics into a binary
      string. First, checks to remove comments from line 
      to save time, memory, and avoid bugs. If there is 
      no comment, then it stays the same*/

    short slashLocation = (short) command.find_first_of('/');
    command = command.substr(0, (slashLocation != -1 ? slashLocation : command.length()));

    string destBits, jumpBits, compBits;                            //Strings where each respective instruction will be stored

    short destPos = (short) command.find_first_of('=', 1) + 1,      //Positions of key characters that help for extracting seperate instructions
          jumpPos = (short) command.find_first_of(';', 1) + 1;

    jumpBits = jumpMap.find((jumpPos != 0 ? command.substr(jumpPos,3) : "0"))->second;
    destBits = destMap.find((destPos != 0 ? command.substr(0, destPos-1) : "0"))->second;
    compBits = compMap.find(command.substr(destPos, (jumpPos != 0 ? jumpPos-1 : command.length()-destPos)))->second;

    return "111" + compBits + destBits + jumpBits;
};

bool HackAssembler::verifyIfNumber(){
    
    for(short i = 0; i < command.length(); i++){                    //True only if all characters are digits
        if(!isdigit(command[i]))
            return false;
    }

    return true;
};

string HackAssembler::convertToBinary(string command){

    short decimalNumber = stoi(command);                            //Converts the string to a number for calculation
    string binaryNumber(16, '0');                                   //16 bits of zeros for easy creation

    for (short i = 15; i >= 0; decimalNumber/=2, i--)
        binaryNumber[i] = (decimalNumber%2 + '0');                  //Simple binary conversion method

    return binaryNumber;
};

int main(){

    string fileName = "Rect.asm";

    // cout << "Enter the name of the file to compile (must include .asm extension): ";
    // getline(cin, fileName);

    HackAssembler assembler(fileName);

    cout << "\nYour file has finished compiling. Press ENTER to exit.";
    cin.get();
    return 0;
}