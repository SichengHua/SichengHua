#include "InstructionSet.h"
#include <iostream>
#include <sstream>
#include <bitset>
#include <string>
#include <cmath>
using namespace std;


string BinaryToHex(string binary_str) {
    stringstream res;
    string Hex = "";
    bitset<32> set(binary_str);
    res << hex << set.to_ulong() << "\n";
    Hex = res.str();
    while(Hex.size()<9){Hex.insert(0,1,'0');}
    return "0x"+Hex;
}

vector<string> split(const string &s, char delim) {
    vector<string> elems;
    stringstream ss(s);
    string item;
    while (getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}


Type TypeClassfy(string operation) {
    for(int i =0; i<Rtype.size();i++) {
        if(Rtype[i] == operation){
            return R;
        }
    }
    for(int i = 0; i<Itype.size();i++){
        if(Itype[i] == operation){
            return I;
        }
    }
    if(Jtype[0] == operation){
        return J;
    }
    bool find_nop;
    find_nop= (operation.find("nop")!=string::npos);
    if(find_nop){
        return R;
    }
    cout<<"Error: Type Not Find in TypeClassfy"<<endl;
    return R;
}

int RegisterAddress(string operation) {
    operation.erase(remove(operation.begin(),operation.end(), ','), operation.end());
    size_t found;
    for(int i =0; i<Registers.size();i++) {
        found = operation.find(Registers[i]);
        if(found!=string::npos)
        {
            return i;
        }
    }
    cout<<"Error: Register Not Find"<<endl;
    return 0;
}

// const vector<string> Rtype = {"add","and","nor","or","xor","sub","jr", "slt","nop"};//nop is in Rtype here
void RtypeClassfy(Rformat *format,string operation)
{
    bool find_nop= (operation.find("nop")!=string::npos);
    format->IsRa = false;
    if(operation=="add"){
        format->funct = 0x20;
    }else if(operation=="and"){
        format->funct = 0x24;
    }else if(operation=="nor"){
        format->funct = 0x27;
    }else if(operation=="or"){
        format->funct = 0x25;
    }else if(operation=="xor"){
        format->funct = 0x26;
    }else if(operation=="sub"){
        format->funct = 0x22;
    }else if(operation=="jr"){
        format->funct = 0x08;
        format->IsRa = true;
    }else if(operation=="slt"){
        format->funct = 0x2a;
    }else if(find_nop){
        format->funct = 0;
    }else{
        cout<<"error: Do not find this funct in R type!"<<endl;
    }
}

//const vector<string> Itype = {"addi","beq", "bne", "lw","sw"};
void ItypeClassfy(Iformat *format, string operation)
{
    if(operation=="addi"){
        format->op = 0x08;
        format->IsConstant = true;
        format->IsLabel = false;
    }else if(operation=="beq"){
        format->op = 0x04;
        format->IsConstant = true;
        format->IsLabel = true;
    }else if(operation=="bne"){
        format->op = 0x05;
        format->IsConstant = true;
        format->IsLabel = true;
    }else if(operation=="lw"){
        format->op = 0x23;
        format->IsConstant = false;
        format->IsLabel = false;
    }else if(operation=="sw"){
        format->op = 0x2b;
        format->IsConstant = false;
        format->IsLabel = false;
    }else{
        cout<<"error: Do not find this funct in I type!"<<endl;
    }
}

//const vector<string> Jtype = {"j"};
void JtypeClassfy(Jformat *format, string operation)
{
    if(operation=="j"){
        format->op = 0x02;
    }else{
        cout<<"error: Do not find this funct in J type!"<<endl;
    }
}


string RtypeInit(vector<string> assembly_code)
{
    Rformat *m_format = new Rformat;

    m_format->op = 0;
    m_format->shamt = 0;
    RtypeClassfy(m_format, assembly_code[0]);
    if(m_format->IsRa)
    {
        m_format->rs = RegisterAddress(assembly_code[1]);
        m_format->rt = 0;
        m_format->rd = 0;
    }
    else if(m_format->funct==0)
    {
        m_format->rs = 0;
        m_format->rt = 0;
        m_format->rd = 0;
    }
    else
    {
        m_format->rd = RegisterAddress(assembly_code[1]);
        m_format->rs = RegisterAddress(assembly_code[2]);
        m_format->rt = RegisterAddress(assembly_code[3]);
    }

    bitset<6> op_bit(m_format->op);
    bitset<5> rs_bit(m_format->rs);
    bitset<5> rt_bit(m_format->rt);
    bitset<5> rd_bit(m_format->rd);
    bitset<5> shamt_bit(m_format->shamt);
    bitset<6> funct_bit(m_format->funct);
    string MachineCode;

    MachineCode.append(op_bit.to_string());
    MachineCode.append(rs_bit.to_string());
    MachineCode.append(rt_bit.to_string());
    MachineCode.append(rd_bit.to_string());
    MachineCode.append(shamt_bit.to_string());
    MachineCode.append(funct_bit.to_string());
    delete m_format;
    return BinaryToHex(MachineCode);
}


string ItypeInit(vector<string> assembly_code, vector<string> labels, vector<unsigned int> labelsAddress, unsigned int pc)
{
    Iformat *m_format = new Iformat;

    ItypeClassfy(m_format, assembly_code[0]);
    m_format->rt = RegisterAddress(assembly_code[1]);
    m_format->rs = RegisterAddress(assembly_code[2]);
    
    if(m_format->IsConstant && !m_format->IsLabel)
    {
        m_format->imm = stoi(assembly_code[3],nullptr,0);
    }
    else if(m_format->IsConstant && m_format->IsLabel)
    {
        m_format->rt = RegisterAddress(assembly_code[2]);
        m_format->rs = RegisterAddress(assembly_code[1]);

        for(int i =0; i<labels.size();i++) {
            if(labels[i] == assembly_code[3]){
                m_format->imm = (int) (labelsAddress[i]-pc-4)/4;
            }
        }
        unsigned int pc_origin = 0x00400000;
        m_format->imm = (pc_origin + stoi(assembly_code[3],nullptr,0)*4)/4;
    }
    else if(!m_format->IsConstant && !m_format->IsLabel)
    {
        size_t found = assembly_code[2].find_first_not_of("0123456789");
        if(found!=string::npos)
        {
            m_format->imm = (int)stoul(assembly_code[2].substr(0,found));
        }
        else
            cout << "relative address of register not found!" << endl;
    }
    else
    {
        cout<<"map error"<<endl;
    }
    

    bitset<6> op_bit(m_format->op);
    bitset<5> rs_bit(m_format->rs);
    bitset<5> rt_bit(m_format->rt);
    bitset<16> imm_bit(m_format->imm);
    string MachineCode;

    if(m_format->IsConstant && !m_format->IsLabel)
    {
        int16_t imm_int = static_cast<int16_t>(~imm_bit.to_ulong() + 1);
        imm_int = imm_int << 16 >> 16;
        if (imm_int < 0) {
                imm_int = (abs(imm_int) ^ 0xffffffff) + 1;
        }
        bitset<16> imm_bit(imm_int);
    }
    MachineCode.append(op_bit.to_string());
    MachineCode.append(rs_bit.to_string());
    MachineCode.append(rt_bit.to_string());
    MachineCode.append(imm_bit.to_string());
    delete m_format;
    return BinaryToHex(MachineCode);
}


string JtypeInit(vector<string> assembly_code, vector<string> labels, vector<unsigned int> labelsAddress)
{
    Jformat *m_format = new Jformat;
    JtypeClassfy(m_format, assembly_code[0]);
    unsigned int pc_origin = 0x00400000;
    m_format->address = (pc_origin + stoi(assembly_code[1],nullptr,0)*4)/4;
    for(int i =0; i<labels.size();i++) {
            if(labels[i] == assembly_code[1]){
                m_format->address = (int) (labelsAddress[i])/4;
            }
        }
    bitset<6> op_bit(m_format->op);
    bitset<26> address_bit(m_format->address);
    string MachineCode;

    MachineCode.append(op_bit.to_string());
    MachineCode.append(address_bit.to_string());
    delete m_format;
    return BinaryToHex(MachineCode);
}




