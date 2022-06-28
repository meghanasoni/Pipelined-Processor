#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

using namespace std;

typedef unsigned char Byte;
typedef unsigned short Bytes_2;
typedef unsigned int Block;

enum OPCODE {
  // encoding the opcodes as required
  ADD = 0,
  SUB = 1,
  MUL = 2,
  INC = 3,
  AND = 4,
  OR = 5,
  NOT = 6,
  XOR = 7,
  LOAD = 8,
  STORE = 9,
  JMP = 10,
  BEQZ = 11,
  HLT = 15
};

/* Used as register file for the processor */
class RegisterFile {
private:
  int size = 16;             // 16 registers present in processor
  vector<Byte> registerVal;  // stores data of registers
  vector<bool> registerOpen; // stores status of registers

public:
  RegisterFile(ifstream *tst) // take input from tst file
  {
    // initialisation
    registerVal = vector<Byte>(size);
    registerOpen = vector<bool>(size, false);
    int index = 0;
    string temp;
    Byte temp1;
    while (*tst >> temp) {
      temp1 = stoi(temp, 0, 16);
      registerVal[index] = (int)temp1;
      index++;
    }
    tst->close();
  }
  // returns value present in the register at the given index
  Byte getRegVal(Byte index) { return (int)registerVal[index]; }

  // sets value of register at the given index to val
  void setRegVal(Byte index, Byte val) {
    if (index != 0)
      registerVal[index] = val;
  }

  // returns status of the register at the given index
  bool getRegOpen(Byte index) { return registerOpen[index]; }

  // sets status of register at the given index as true if it is  being written
  // to
  void setRegOpen(Byte index, bool val) { registerOpen[index] = val; }
};

/* Used as instruction cache for the processor */
class InstructionCache {
private:
  int cacheSize = 256;
  int blockSize = 4;
  vector<Block> table;
  bool writePort = false;
  bool readPort = false;

public:
  // reads tst file into instruction cache
  InstructionCache(ifstream *tst) {
    table = std::vector<Block>(cacheSize / blockSize, 0);
    std::string temp;
    Block temp2;
    Block valuetobeWritten = 0;
    int index = 0;
    int iter = 0;
    while (*tst >> temp) {
      temp2 = stoi(temp, 0, 16);
      valuetobeWritten = valuetobeWritten << 8;
      valuetobeWritten += temp2;
      iter++;
      if (iter == 4) {
        table[index] = valuetobeWritten;
        index++;
        iter = iter % 4;
        valuetobeWritten = 0;
      }
    }
    tst->close();
  }

  //returns table content at the given address
  Block getBlock(Byte address) {
    Block index = address >> 2;
    return table[index];
  }

  //returns two bytes from the given address
  Bytes_2 getInstruction(Byte address) {
    Block Data = getBlock(address);
    int offset = address & 3;//take the last two bytes of address
    switch (offset) {
    case 0: {
      return (Data & 0xffff0000) >> 16;
    }
    case 1: {
      return (Data & 0x00ffff00) >> 8;
    }
    case 2: {
      return (Data & 0x0000ffff);
    }
    default: {
      Block d1 = Data & 0x000000ff;
      Block d2 = getBlock(address + 1) & 0xff000000;
      return (d1 << 8) + (d2 >> 24);
    }
    }
  }
};

/* Used as data cache for the processor */
class DataCache {
private:
  int cacheSize = 256;
  int blockSize = 4;
  vector<Block> table;
  bool writePort = false;
  bool readPort = false;

public:
  //constructor
  DataCache(std::ifstream *tst) {
    table = std::vector<Block>(cacheSize / blockSize, 0);
    std::string temp;
    Block temp2;
    Block valuetobeWritten = 0;
    int index = 0;
    int iter = 0;
    while (*tst >> temp) {
      temp2 = stoi(temp, 0, 16);
      valuetobeWritten = valuetobeWritten << 8;
      valuetobeWritten += temp2;
      iter++;
      if (iter == 4) {
        table[index] = valuetobeWritten;
        index++;
        iter = iter % 4;
        valuetobeWritten = 0;
      }
    }
    tst->close();
  }

   //returns table content at the given address
  Block getBlock(Byte address) {
    Block index = address >> 2;
    return table[index];
  }

  Block getByte(Byte address) {
    Block Data = getBlock(address);
    int offset = address & 3;
    switch (offset) {
    case 0: {
      return ((Data & 0xff000000) >> 24);
    }
    case 1: {
      return ((Data & 0x00ff0000) >> 16);
    }
    case 2: {
      return ((Data & 0x0000ff00) >> 8);
    }
    default: {
      return (Data & 0x000000ff);
    }
    }
  }

  void setByte(Byte address, Byte value) {
    Block val = value;
    Block Data = getBlock(address);
    int offset = address & 3;
    switch (offset) {
    case 0: {
      table[address >> 2] = (Data & 0x00ffffff) + (value << 24);
      break;
    }
    case 1: {
      table[address >> 2] = (Data & 0xff00ffff) + (value << 16);
      break;
    }
    case 2: {
      table[address >> 2] = (Data & 0xffff00ff) + (value << 8);
      break;
    }
    default: {
      table[address >> 2] = (Data & 0xffffff00) + (value);
    }
    }
  }

  void writeDCacheFile(string outfile) {
    std::ofstream output;
    output.open(outfile, std::ofstream::trunc);
    int i;
    for (int i = 0; i < cacheSize; i++){ 
       output << setfill('0') << setw(2) << std::hex << (int)getByte(i) << endl;
    }
    output.close();
  }
};

class Pipeline {
private:
public:
  InstructionCache *I$; // pointer to store instruction cache
  DataCache *D$;        // object pointer to store data cache
  RegisterFile *R$;     // register file object pointer

  // buffers
  Bytes_2 PC = 0u;

  Block IR = 0u;
  Block IR_EX = 0u;
  Block IR_MM = 0u;
  Block IR_WB = 0u;
  Bytes_2 PC_EX = 0u;
  Block A_EX = 0u;
  Block B_EX = 0u;
  short L1_EX = 0u;
  Byte D_EX = 0u;
  Byte ALUOutput = 0u;
  Bytes_2 REG = 0u;
  Bytes_2 REG_2 = 0u;
  Bytes_2 VAL_WB = 0u;
  Byte ALUOutput_WB = 0u;

  // bool variables to check if we need to run the corresponding stages
  bool IFtoRun = true;
  bool IDtoRun = false;
  bool EXtoRun = false;
  bool MMtoRun = false;
  bool WBtoRun = false;

  // stall the corresponding state if the bool value is true
  bool IDtoStall = false;
  bool EXtoStall = false;

  bool toHalt = false;

public:
  // variables to know the status of the processor
  int totalInstructions = 0;
  int logicalInstructions = 0;
  int arithmeticInstructions = 0;
  int dataInstructions = 0;
  int controlInstructions = 0;
  int haltInstructions = 0;
  int totalCycles = 0;
  int totalStalls = 0;
  int dataStalls = 0;
  int controlStalls = 0;

  void Start() {
    while (!toHalt) {
      runCycle();
    }
  }

  void runCycle() {
    WBstage();
    MMstage();
    EXstage();
    IDstage();
    IFstage();
    totalCycles++;
  }
  // constructor
  Pipeline(ifstream *ip, ifstream *dp, ifstream *rp) {
    I$ = new InstructionCache(ip);
    D$ = new DataCache(dp);
    R$ = new RegisterFile(rp);
  }
  ~Pipeline() {
    delete (I$);
    delete (D$);
    delete (R$);
  }

  /* Instruction Fetch Cycle */
  void IFstage() {
    if (!IFtoRun) {
      return;
    }

    if (IDtoRun || IDtoStall) {
      IFtoRun = true;
      return;
    }
    IR = I$->getInstruction(PC);
    PC += 2;
    IDtoRun = true;
  }

  /* Instruction Decode Cycle */
  void IDstage() {
    if ((!IDtoRun) || IDtoStall) {
      return;
    }

    if (EXtoRun || EXtoStall) {
      IDtoRun = true;
      return;
    }
    IR_EX = IR;

    Bytes_2 opCode = (int)((IR & 0xf000) >> 12); // opcode is first 4 bits
    if ((int)opCode == HLT) {
      IFtoRun = false;
      IDtoRun = false;
      EXtoRun = true;
      haltInstructions++;
      return;
    }

    IDtoRun = false;
    EXtoRun = true;

    if ((int)opCode == JMP) // unconditional jump to L1
    {
      controlStalls += 2;
      PC_EX = PC;
      L1_EX = (0x0ff0 & IR) >> 4; // value of L1
      IDtoStall = true;
      return;
    }

    if ((int)opCode == BEQZ) {
      IDtoStall = true;
      REG = (int)(IR & 0x0f00) >> 8; // value of R1
      if (R$->getRegOpen(REG)) {
        dataStalls++;
        EXtoRun = false;
        IDtoRun = true;
        IDtoStall = false;
        return;
      }

      controlStalls += 2;
      PC_EX = PC;
      A_EX = R$->getRegVal((IR & 0x0f00) >> 8);
      L1_EX = IR & 0x00ff;
      return;
    }

    if ((int)opCode == STORE) {
      REG = (IR & 0x00f0) >> 4; // value of R2
      REG_2 = (IR & 0x0f00) >> 8;
      if (R$->getRegOpen(REG) || R$->getRegOpen(REG_2)) {
        dataStalls++;
        EXtoRun = false;
        IDtoRun = true;
        return;
      }
      A_EX = R$->getRegVal(REG); // A=value in R2
      B_EX = IR & 0x000f;        // B=X
      return;
    }

    if ((int)opCode == LOAD) {
      REG = (IR & 0x00f0) >> 4;
      if (R$->getRegOpen(REG)) {
        dataStalls++;
        EXtoRun = false;
        IDtoRun = true;
        return;
      }
      A_EX = R$->getRegVal(REG);
      B_EX = IR & 0x000f;
      REG = (IR & 0x0f00) >> 8;
      R$->setRegOpen(REG, true);
      return;
    }

    if (opCode == ADD || opCode == SUB || opCode == MUL) {
      REG = (IR & 0x00f0) >> 4;
      REG_2 = IR & 0x000f;
      if (R$->getRegOpen(REG) || R$->getRegOpen(REG_2)) {
        dataStalls++;
        EXtoRun = false;
        IDtoRun = true;
        return;
      }
      A_EX = R$->getRegVal(REG);
      B_EX = R$->getRegVal(REG_2);
      R$->setRegOpen((IR & 0x0f00) >> 8, true);
      return;
    }

    if (opCode == INC) {
      REG = (IR & 0x0f00) >> 8;
      if (R$->getRegOpen(REG)) {
        dataStalls++;
        EXtoRun = false;
        IDtoRun = true;
        return;
      }
      A_EX = R$->getRegVal(REG);
      R$->setRegOpen((REG), true);
      return;
    }

    if (opCode == AND || opCode == OR || opCode == XOR) {
      REG = (IR & 0x00f0) >> 4;
      REG_2 = IR & 0x000f;
      if (R$->getRegOpen(REG) || R$->getRegOpen(REG_2)) {
        dataStalls++;
        EXtoRun = false;
        IDtoRun = true;
        return;
      }
      A_EX = R$->getRegVal(REG);
      B_EX = R$->getRegVal(REG_2);
      R$->setRegOpen((IR & 0x0f00) >> 8, true);
      return;
    }

    if (opCode == NOT) {
      REG = (IR & 0x00f0) >> 4;
      if (R$->getRegOpen(REG)) {
        dataStalls++;
        EXtoRun = false;
        IDtoRun = true;
        return;
      }
      A_EX = R$->getRegVal(REG);
      R$->setRegOpen((IR & 0x0f00) >> 8, true);
      return;
    }
  }

  /* Execution Address Cycle */
  void EXstage() {
    if ((!EXtoRun) || EXtoStall) {
      return;
    }
    
    Bytes_2 opCode = (IR_EX & 0xf000) >> 12;
    totalInstructions++;
    //memory cycle to run next
    MMtoRun = true;
    EXtoRun = false;
    switch (opCode) {
    case ADD: {
      ALUOutput = A_EX + B_EX;
      arithmeticInstructions++;
      break;
    }
    case SUB: {
      ALUOutput = A_EX - B_EX;
      arithmeticInstructions++;
      break;
    }
    case MUL: {
      ALUOutput = A_EX * B_EX;
      arithmeticInstructions++;
      break;
    }
    case INC: {
      ALUOutput = A_EX + 1;
      arithmeticInstructions++;
      break;
    }
    case AND: {
      ALUOutput = A_EX & B_EX;
      logicalInstructions++;
      break;
    }
    case OR: {
      ALUOutput = A_EX | B_EX;
      logicalInstructions++;
      break;
    }
    case NOT: {
      ALUOutput = ~A_EX;
      logicalInstructions++;
      break;
    }
    case XOR: {
      ALUOutput = A_EX ^ B_EX;
      logicalInstructions++;
      break;
    }
    case LOAD: {
      ALUOutput = A_EX + B_EX;
      dataInstructions++;
      break;
    }
    case STORE: {
      ALUOutput = A_EX + B_EX;
      dataInstructions++;
      break;
    }
    case JMP: {
      EXtoStall = true;
      int PCr=PC_EX;
      int jump;
      //sign-extension of L1
      if((L1_EX & 128)==128){
        jump = 0xffff0000 + L1_EX;
      }else{
        jump = L1_EX;
      }
      //calculating effective destination pc counter
      ALUOutput = PCr + (jump<<1);
      controlInstructions++;
      break;
    }
    case BEQZ: {
      EXtoStall = true;
      if ((int)A_EX == 0) {
      int PCr=PC_EX;
      int jump;
      if((L1_EX & 128)==128){
        jump = 0xffff0000 + L1_EX;
      }else{
        jump = L1_EX;
      }
      ALUOutput = PCr + (jump<<1); 
      } else {
        ALUOutput = PC_EX;
      }
      controlInstructions++;
      break;
    }
    }
    
    IR_MM = IR_EX;
  }

  /* Memory Access Cycle */
  void MMstage() {
    if (!MMtoRun) {
      return;
    }
    MMtoRun = false;
    Bytes_2 opCode = (IR_MM & 0xf000) >> 12;
    Bytes_2 R1 = (IR_MM & 0x0f00) >> 8;

    if (opCode == HLT) {
      WBtoRun = true;
      IR_WB = IR_MM;
      return;
    }

    if (opCode == JMP || opCode == BEQZ) {
      if (ALUOutput == PC) {
        IDtoRun = false;
        IDtoStall = false;
        EXtoStall = false;
      } else {
        PC = ALUOutput;
        flushPipeline();
      }
      WBtoRun = false;
      return;
    }

    if (opCode == STORE) {
      D$->setByte(ALUOutput, R$->getRegVal((IR_MM & 0x0f00) >> 8));
      VAL_WB = 0u;
      IR_WB = 0u;
      ALUOutput_WB = 0u;
      MMtoRun = false;
      WBtoRun = true;
      return;
    }

    if (opCode == LOAD) {
      VAL_WB = D$->getByte(ALUOutput);
      IR_WB = IR_MM;
      ALUOutput_WB = ALUOutput;
      WBtoRun = true;
      MMtoRun = false;
      return;
    }

    MMtoRun = false;
    IR_WB = IR_MM;
    VAL_WB = 0u;
    ALUOutput_WB = ALUOutput;
    WBtoRun = true;
  }

  /* Write-back Cycle */
  void WBstage() {
    if (!WBtoRun) {
      return;
    }
    Bytes_2 opCode = (IR_WB & 0xf000) >> 12;
    Bytes_2 R1 = (IR_WB & 0x0f00) >> 8;
    if (opCode == HLT) {
      WBtoRun = false;
      toHalt = true;
      return;
    }
    if (opCode == LOAD) {
      R$->setRegVal(R1, VAL_WB);
    } else {//all Arithmetic and logical instructions
      R$->setRegVal(R1, ALUOutput_WB);
    }
    R$->setRegOpen(R1, false);
    WBtoRun = false;
  }

  // for flushing the pipeline
  void flushPipeline() {
    IFtoRun = true;
    IDtoRun = true;
    EXtoRun = true;
    MMtoRun = true;
    WBtoRun = true;

    IDtoStall = false;
    EXtoStall = false;

    IR = 0u;
    IR_EX = 0u;
    IR_MM = 0u;
    IR_WB = 0u;
    PC_EX = 0u;
    A_EX = 0u;
    B_EX = 0u;
    L1_EX = 0u;
    D_EX = 0u;
    ALUOutput = 0u;
    REG = 0u;
    REG_2 = 0u;
    VAL_WB = 0u;
    ALUOutput_WB = 0u;
  }

  // for printing the processor stats
  void printStatistics(string DCache, string outfilename){

     D$->writeDCacheFile(DCache);
    ofstream output;
    output.open(outfilename);

    output << "Total number of instructions executed: " << totalInstructions
           << endl;
    output << "Number of instructions in each class-" << endl;
    output << "Arithmetic Instructions     :" << arithmeticInstructions << endl;
    output << "Logical Instructions        :" << logicalInstructions << endl;
    output << "Data Instructions           :" << dataInstructions << endl;
    output << "Control Instructions        :" << controlInstructions << endl;
    output << "Halt Instructions           :" << haltInstructions << endl;
    output << "Cycles per Instruction(CPI) :"
           << ((double)totalCycles / totalInstructions) << endl;
    output << "Total number of stalls      :" << dataStalls + controlStalls
           << endl;
    output << "Data stalls (RAW)           :" << dataStalls << endl;
    output << "Control stalls              :" << controlStalls << endl;
  }
};

int main() {
  ifstream iC, dC, rF; //creating objects neede for handling files
  iC.open("ICache.txt"); //pointer of the instruction cache file
  dC.open("DCache.txt"); //pointer of the data cache file
  rF.open("RF.txt");  //input for the register file
  Pipeline Processor(&iC, &dC, &rF);
  Processor.Start(); //running the processor
  Processor.printStatistics("ODCache.txt", "Output.txt");
  return 0;
}