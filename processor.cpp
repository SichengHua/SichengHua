#include "processor.h"
#include "InstructionSet.h"


using namespace std;

//   #define PROCESSOR_DEBUG 1
//    #define PROCESSOR_STATUS 1
//by comment and uncomment the above codes to get the corresponding information


void CycleExecution(Processor *m_Processor, string machineCode)
{
    //initialization per instruction
    m_Processor->instructionString = machineCode;
    m_Processor->currentInstruction = 0;
    memset(m_Processor->controlSignal,0,sizeof(m_Processor->controlSignal));
    memset(m_Processor->registerAddress,0,sizeof(m_Processor->registerAddress));
    m_Processor->shamt = 0;
    m_Processor->funct = 0;
    m_Processor->imm = 0;
    m_Processor->address = 0;
    m_Processor->instructionClass = undefined;
    m_Processor->isWriteBack = false;
    m_Processor->m_RegisterUnit= {0,0,0,false,0,0,0};
    m_Processor->m_ALUUnit={0,0,false,0};
    m_Processor->m_DataMemoryUnit.address=0;
    m_Processor->m_DataMemoryUnit.writeData=0;
    m_Processor->m_DataMemoryUnit.ReadData=0;


    fetch(m_Processor);
    m_Processor->cycles++;

    decode(m_Processor);
    m_Processor->cycles++;
    
    if(m_Processor->instructionClass == undefined)
        assert(("instructionClass undefined", false));
    
    execute(m_Processor);
    m_Processor->cycles++;

    if(m_Processor->instructionClass == loadWord | m_Processor->instructionClass == storeWord)
    {
    memoryAccess(m_Processor);
    m_Processor->cycles++;
    }

    bool RegWrite= m_Processor->controlSignal[7];
    m_Processor->isWriteBack = RegWrite ? true : false; 

    if(m_Processor->instructionClass == rType | m_Processor->instructionClass == loadWord | m_Processor->instructionClass == addi)
    {
    writeBack(m_Processor);
    m_Processor->cycles++;
    }
}


void fetch(Processor *m_Processor)
{
    if(m_Processor->instructionString.find("0x00000000")==string::npos)
    {
        m_Processor->currentInstruction = stoul(m_Processor->instructionString, nullptr, 0);
    }
    else
    {
        m_Processor->currentInstruction = 0x00000000;
    }
}

void decode(Processor *m_Processor)
{
    unsigned int opCode = m_Processor->currentInstruction >> 26;
    unsigned int signal = 0;

    // RegDst, Jump, Branch, MemRead, MemtoReg, MemWrite, ALUSrc, RegWrite, ALUOp1, ALUOp0
    switch (opCode) {
      case 0x0:  // R-format
        signal = signals[0];
        m_Processor->instructionClass = rType;
        break;
      case 0x23:  // load word
        signal = signals[1];
        m_Processor->instructionClass = loadWord;
        break;
      case 0x2b:  // store word
        signal = signals[2];
        m_Processor->instructionClass = storeWord;
        break;
      case 0x04:  // beq
        signal = signals[3];
        m_Processor->instructionClass = branch;
        break;
      case 0x02:  // jump
        signal = signals[4];
        m_Processor->instructionClass = jump;
        break;
      case 0x08:  // addi
        signal = signals[5];
        m_Processor->instructionClass = addi;
        break;
      default:
        cout << "opCode to control signal not found!" << endl;
        break;
    }

    for (int j = 9; j >= 0; j--) {
      m_Processor->controlSignal[j] = signal & 1;
      signal >>= 1;
    }
    
    //set up formats
    unsigned int instruction = m_Processor->currentInstruction;
    unsigned int bit25_21 = (instruction >> 21) & 0b11111;
    unsigned int bit20_16 = (instruction >> 16) & 0b11111;
    unsigned int bit15_11 = (instruction >> 11) & 0b11111;
    unsigned int bit10_06 = (instruction >> 6) & 0b11111;
    unsigned int bit05_00 = instruction & 0b111111;
    unsigned int bit15_00 = instruction & 0b1111111111111111;
    unsigned int bit25_00 = instruction & 0b1111111111111111111111111;
    
    
    switch(m_Processor->instructionClass){
        case rType:  //rType
            m_Processor->registerAddress[0] = (unsigned int) bit25_21;
            m_Processor->registerAddress[1] = (unsigned int) bit20_16;
            m_Processor->registerAddress[2] = (unsigned int) bit15_11;
            m_Processor->shamt= (unsigned int) bit10_06;
            m_Processor->funct= (unsigned int) bit05_00;
            m_Processor->imm=-1;
            m_Processor->address=-1;
            break;
        case loadWord: case storeWord: case branch: case addi:
            m_Processor->registerAddress[0] = (unsigned int) bit25_21;
            m_Processor->registerAddress[1] = (unsigned int) bit20_16;
            m_Processor->imm= (unsigned int) bit15_00;
            m_Processor->registerAddress[2] =-1;
            m_Processor->shamt=-1;
            m_Processor->funct=-1;
            m_Processor->address=-1;
            break;
        case jump:
            m_Processor->address= (unsigned int) bit25_00;
            m_Processor->registerAddress[0] =-1;
            m_Processor->registerAddress[1] =-1;
            m_Processor->registerAddress[2] =-1;
            m_Processor->shamt=-1;
            m_Processor->funct=-1;
            m_Processor->imm=-1;
            break;
        case nop:
            //do nothing except for progran counter increment
            m_Processor->registerAddress[0] =-1;
            m_Processor->registerAddress[1] =-1;
            m_Processor->registerAddress[2] =-1;
            m_Processor->shamt=-1;
            m_Processor->funct=-1;
            m_Processor->imm=-1;
            m_Processor->address=-1;
            break;
        default:
            cout << "Registers setup failed!" << endl;     
    }

}

void execute(Processor *m_Processor)
{
    if(m_Processor->instructionClass==jump)
    {
        int jumpResult=0;
        jumpResult = m_Processor->address * 4;
        m_Processor->programCounter = jumpResult; 
    }
    else if(m_Processor->instructionClass==nop)
    {
        m_Processor->programCounter = m_Processor->programCounter+4;
    }
    else
    {
        registerInit(m_Processor);
        ALUInit(m_Processor);
        unsigned int tempProgramCounter = m_Processor->programCounter + 4;
        bool branch = m_Processor->controlSignal[2];
    
        bool AluZero = m_Processor->m_ALUUnit.zero;
        int AddResult=0;
        AddResult = tempProgramCounter + (SignExtensionInit(m_Processor->imm) << 2);
        m_Processor->programCounter = (branch && AluZero) ? AddResult : tempProgramCounter;
    }
}

void memoryAccess(Processor *m_Processor)
{
    DataMemoryInit(m_Processor);
}

void writeBack(Processor *m_Processor)
{
    registerInit(m_Processor);
}

int SignExtensionInit(unsigned int bit16)
{   
    return (int)(bit16 << 16) >> 16;
}

void registerInit(Processor *m_Processor)
{
    bool RegDst = m_Processor->controlSignal[0];
    bool MemtoReg = m_Processor->controlSignal[4];
    bool RegWrite = m_Processor->controlSignal[7];
    bool isWriteBack = m_Processor->isWriteBack;
    unsigned int readRegister1 = m_Processor->registerAddress[0]; //rs
    unsigned int readRegister2 = m_Processor->registerAddress[1]; //rt
    //mux for instruction[20-16] and instruction[15-11]
    unsigned int writeRegister = RegDst ? m_Processor->registerAddress[2] : m_Processor->registerAddress[1];
    //mux for ALU result and Read data from memory
    unsigned int writeData = MemtoReg ? m_Processor->m_DataMemoryUnit.ReadData : (unsigned int) m_Processor->m_ALUUnit.result;

    unsigned int readData1 = m_Processor->registers[readRegister1];
    unsigned int readData2 = m_Processor->registers[readRegister2];

    if(isWriteBack && RegWrite)
    {
        m_Processor->registers[writeRegister] = writeData;
    }

    m_Processor->m_RegisterUnit.readRegister1 = readRegister1;
    m_Processor->m_RegisterUnit.readRegister2 = readRegister2;
    m_Processor->m_RegisterUnit.readData1 = readData1;
    m_Processor->m_RegisterUnit.readData2 = readData2;
    m_Processor->m_RegisterUnit.writeRegister = writeRegister;
    m_Processor->m_RegisterUnit.writeData = writeData;

    // cout << "-------------------------" << endl;
}

void ALUInit(Processor *m_Processor)
{
    //control setup
    bool ALUOp1 = m_Processor->controlSignal[8];
    bool ALUOp0 = m_Processor->controlSignal[9];
    int a = m_Processor->m_RegisterUnit.readData1;
    int b = m_Processor->controlSignal[6] ? SignExtensionInit(m_Processor->imm) : m_Processor->m_RegisterUnit.readData2;

    bool zero = false;
    int result = 0;
    if (!ALUOp1 && !ALUOp0) // lw and sw
    {
        result = a + b;
    }
    else if (!ALUOp1 && ALUOp0) // beq
    {
        result = a - b;
    }
    else if (ALUOp1 && !ALUOp0) // R-type instructions
    {
        switch (m_Processor->funct)
        {
            case 0x20: //add
                result = a + b;
                break;
            case 0x22: //sub
                result = a - b;
                break;
            case 0x24: //and
                result = a & b;
                break;
            case 0x25: //or
                result = a | b;
                break;
            case 0x2a: //slt
                result = (a < b) ? 0b1 : 0b0;
                break;
            case 0x27: //nor
                result = ~(a | b);
                break;
            case 0x26: //xor
                result = a ^ b;
                break;
            default:
                if((m_Processor->address != -1)&(m_Processor->registerAddress[0]!=-1)){
                    cout<< "func map not found!" << endl;
                }
                break;
        }
    }

    zero = (result == 0);
    m_Processor->m_ALUUnit.inputA = a;
    m_Processor->m_ALUUnit.inputB = b;
    m_Processor->m_ALUUnit.zero = zero;
    m_Processor->m_ALUUnit.result = result;
}

void DataMemoryInit(Processor *m_Processor)
{
    unsigned int address = m_Processor->m_ALUUnit.result;
    unsigned int writeData = m_Processor->m_RegisterUnit.readData2;
    unsigned int ReadData=0;
    bool MemWrite = m_Processor->controlSignal[5];
    bool MemRead  = m_Processor->controlSignal[3];
    if(MemWrite)
    {
        m_Processor->m_DataMemoryUnit.memoryAddress.push_back(address);
        m_Processor->m_DataMemoryUnit.memoryData.push_back(writeData);
    }
    else if(MemRead)
    {
        for(int i =0; i<m_Processor->m_DataMemoryUnit.memoryAddress.size();i++) {
            if(m_Processor->m_DataMemoryUnit.memoryAddress[i] == address)
            {
                ReadData = m_Processor->m_DataMemoryUnit.memoryData[i];            
            }
        }
    }
    m_Processor->m_DataMemoryUnit.ReadData = ReadData;
}

string outputRegister(Processor *m_Processor)
{
    stringstream res;
    for(int i=0; i<Registers.size(); i++)
    {
        res << Registers[i] << ":" << uintToHex(m_Processor->registers[i]) << "\n";
    }
    res << "pc:" << uintToHex(m_Processor->programCounter) << "\n";
    return res.str();
}

string uintToHex(unsigned int uint)
{
    stringstream ss;
    ss << hex << setw(8) << setfill('0') << uint;
    return "0x" + ss.str();
}
