#include <stdio.h>
#include <stdlib.h>
#include "string.h"
#include <ctype.h>

#define DS 0
#define SS 1
#define ES 2
#define CS 3
#define HP 4
#define IP 5
#define SP 6
#define BP 7
#define CC 8
#define AC 9
#define EAX 10
#define EBX 11
#define ECX 12
#define EDX 13
#define tamHeader 512

typedef struct
{
  char reg[5];
  int pos;
} Tvec;

typedef struct
{
  char mnemo[10];
  int hex;
} TvecMnemo;

typedef struct
{
  char cad[100];
} Tdisassembler;

typedef struct
{
  int tipoAr;
  unsigned int version;
  unsigned long long int identificador;
  int fechaCreacion;
  int horaCreacion;
  char tipo;
  char cantCil;
  char cantCab;
  char cantSec;
  unsigned int Ssize;
  char Relleno[472];
} THeaderDisco;

typedef struct
{
  unsigned int cantCab, cantSec, cantCil;
  unsigned int Ssize;
  FILE *arch;
} TVecDisco;

int Reg[16] = {0};
int memoriaRAM[8192] = {0};
int header[6] = {0};
THeaderDisco headerDisco;
TVecDisco vecDisco[255];
int flagB = 0;
int flagC = 0;
int flagD = 0;
Tdisassembler Disassembler[1500];

Tvec registros[16];
TvecMnemo inst[32];
int posDisassembler = 0;
int ejecutando = 1;
int estado = 0;

void cambiaCC(int val);
void setRegistros();
int lecturaBin(char nombreArch[]);
void creaReg(Tvec registros[]);
void step();
void decodificaInstruccion(int instruccion, int *cod, int *topA, int *topB, int *opA, int *opB, int *cantOp);
void ejecutaInstruccion(int cod, int topA, int topB, int *opA, int *opB, int cantOp);
void guardaReg(int pos, int val);
void muestraValores(char rta[]);
void desarmaRespuesta(char rta[], char cad1[], char cad2[]);
void pasoApaso(char rta[]);
void proxinstruccion();
void disassembler(int instruccion);
void leer();
void escribir();
void breakpoint();
void creaInstrucciones(TvecMnemo vec[]);
int valRegistro(int op);
int valRegIndirecto(int op);
void muestraDisassembler(Tdisassembler Disassembler[]);
void stringWrite();
void stringRead();
int getPosicion(int valor);
int getParteAlta(int valor);
int getParteBaja(int valor);
void setParteAlta(int *num, int val);
void setParteBaja(int *num, int val);
void creaVdd();
int getPosicion(int valor);
void lecturaVdd(char nombreArch[], int k);
void accesoDisco();
int posDirectoIndirecto(int top, int op);

int main(int argc, char *argv[])
{
  int status = 0;
  int i = 2, k = 0;
  creaReg(registros);
  creaInstrucciones(inst);
  if (argc < 2)
    printf("Error. Faltan argumentos. Recomendacion:\n mvx.exe BinFilename [-b] [-c] [-d] (flags opcionales[]) \n");
  else
  {
    int j = 2;
    while (j < argc)
    {
      if (strcmp(argv[j], "-b") == 0)
      {
        flagB = 1;
      }
      if (strcmp(argv[j], "-c") == 0)
      {
        flagC = 1;
        system("cls");
      }
      if (strcmp(argv[j], "-d") == 0)
      {
        flagD = 1;
      }
      j++;
    }
  }
  if (argc > 1)
  {
    status = lecturaBin(argv[1]);
    //printf ("i = %d, k = %d\n", i, k);
    while (strcmp(argv[i], "-b") != 0 && strcmp(argv[i], "-c") != 0 && strcmp(argv[i], "-d") != 0 && i<argc)
    {
      lecturaVdd(argv[i], k);
      i++;
      k++;
    }
    if (i == 2)
      creaVdd();
  }
  
  if (!status)
  {
    do
    {
      step();
    } while (ejecutando && (0 <= getParteBaja(Reg[IP])) && (getParteBaja(Reg[IP]) < getParteAlta(Reg[CS])));
    printf("\n");
    printf("\nEjecucion finalizada\n");
  }
  return 0;
}

void step()
{

  int cod = 0x0, topA = 0x0, topB = 0x0, opA = 0x0, opB = 0x0, cantOp = 0x0;

  decodificaInstruccion(memoriaRAM[getPosicion(Reg[IP])], &cod, &topA, &topB, &opA, &opB, &cantOp);
  /* if (flagD)
  {
    disassembler(memoriaRAM[getPosicion(Reg[IP])]);
  }  */
  //printf("decod\n");
  Reg[IP]++;
  ejecutaInstruccion(cod, topA, topB, &opA, &opB, cantOp);
}

void muestraDisassembler(Tdisassembler Disassembler[])
{
  for (int i = 0; i < posDisassembler; i++) // arreglar strlen
    printf("%s \n", Disassembler[i].cad);
}

void decodificaInstruccion(int instruccion, int *cod, int *topA, int *topB, int *opA, int *opB, int *cantOp)
{
  //*cod = instruccion & mnem;
  *cod = instruccion;
  *cod = (*cod) >> 20;
  *cod = (*cod) & 0x00000fff;

  if (*cod >> 4 == 0xff)
  { // intruccion sin operandos
    *cantOp = 0;
  }
  else
  {
    *cod = *cod >> 4;
    if (*cod >> 4 == 0xf)
    { // instruccion de 1 operando
      *cantOp = 1;
      *opA = instruccion & 0x0000ffff;
      *opA = *opA << 16;
      *opA = *opA >> 16;
      *topA = instruccion & 0x00ffffff;
      *topA = *topA >> 22;
    }
    else
    { // instruccion de 2 operandos
      *cantOp = 2;
      *cod = (*cod) >> 4;
      *opB = instruccion;
      *opB = *opB & 0xfff;
      *opB = *opB << 20;
      *opB = *opB >> 20;
      *opA = (instruccion >> 12) & 0x00000fff; // no se si es legal hacerlo en la misma linea
      *opA = *opA << 20;
      *opA = *opA >> 20;
      *topB = instruccion >> 24;
      *topB = *topB & 0x3;
      *topA = instruccion & 0x0fffffff;
      *topA = (*topA) >> 26;
    }
  }
}

void setRegistros()
{
  int offset;
  // Data Segment
  Reg[DS] = header[1];
  Reg[DS] = Reg[DS] << 16; // la parte alta del reg contiene el tamano en celdas del segmento, y la parte baja la direccion absoluta en memoria // 0xXXXX 0000
  // printf("Reg[DS]: %x\n",Reg[DS]);
  // Stack Segment
  offset = header[1] & 0x0000ffff;
  Reg[SS] = ((header[2] << 16)) + offset;
  // printf("Reg[SS]: %x\n",Reg[SS]);
  // Extra segment
  offset += header[2];
  offset = offset & 0x0000ffff;
  Reg[ES] = ((header[3] << 16)) + offset;
  // printf("Reg[ES]: %x\n",Reg[ES]);
  // Code Segment
  offset += header[3];
  offset = offset & 0x0000ffff;
  Reg[CS] = ((header[4] << 16)) + offset;
  Reg[IP] = 0x00030000;
  Reg[SP] = 0x00010000;
  setParteBaja(&Reg[SP], getParteAlta(Reg[SS]));
  Reg[BP] = 0x00010000;
  // setParteBaja(&Reg[BP],getParteAlta(Reg[SS]));
  Reg[HP] = 0x00020000;
}

void lecturaVdd(char nombreArch[], int k)
{ // se pasa con .vdd
  int temp;

  FILE *arch = fopen(nombreArch, "rb");
  if (arch != NULL)
  {
    for (int t = 0; t<9; t++)
        fread(&temp, sizeof(temp), 1, arch);
    //printf("temp %x\n",temp);
    vecDisco[k].cantCil = getParteAlta(temp) & 0xFF;
    vecDisco[k].cantCab = ((getParteBaja(temp)) >> 8) & 0xFF;
    vecDisco[k].cantSec = getParteBaja(temp) & 0xFF;
    fread(&temp, sizeof(temp), 1, arch);
    //printf("temp %x\n",temp); //00000200   00020000
    vecDisco[k].Ssize = temp;
    vecDisco[k].arch = arch;
    fclose(arch);

    /* printf("cantCab %d\n",vecDisco[k].cantCab);
    printf("cantCil %d\n",vecDisco[k].cantCil);
    printf("cantSec %d\n",vecDisco[k].cantSec);
    printf("Size %d\n",vecDisco[k].Ssize);
    printf("arch %s\n",vecDisco[k].arch); */
  }
  else
    printf("Error al abrir el archivo binario %s.\n", nombreArch);
}

void creaVdd()
{
  FILE *arch = fopen("disk0.vdd", "wb");
  THeaderDisco hd;
  hd.cantCil = hd.cantCab = hd.cantSec = 128;
  hd.tipo = hd.version = 1;
  hd.tipoAr = 0x56444430;
  hd.Ssize = 512;
  hd.identificador = rand() % (0xFFFFFFFFFFFFFFFF);
  fwrite(&hd, sizeof(hd), 1, arch);
  vecDisco[0].cantCil = vecDisco[0].cantSec = vecDisco[0].cantCab = 128;
}

int lecturaBin(char nombreArch[])
{ // se pasa con .mv2
  FILE *arch = fopen(nombreArch, "rb");
  int temp, i = 0;
  int error = 0;
  for (i = 0; i < 6; i++)
  {
    fread(&temp, sizeof(temp), 1, arch);
    header[i] = temp;
  }
  if ((header[0] == 0x4D562D32) && (header[5] == 0x562E3232))
  {
    if (header[1] + header[2] + header[3] <= 8192 - header[4])
    {
      setRegistros();
      i = 0;
      while (fread(&temp, sizeof(temp), 1, arch) == 1)
      {
        memoriaRAM[getParteBaja(Reg[CS]) + i] = temp;
        //printf("%x\n",temp);
        disassembler(memoriaRAM[getParteBaja(Reg[CS]) + i]);
        i++;
      }
    }
    else
    {
      printf("ERROR: La cantidad de Celdas de memoria a almacenar supera el limite permitido!\n");
      error = 1;
    }
  }
  else
  {
    printf("ERROR: El formato del archivo %s no es correcto", nombreArch);
    error = 1;
  }
  return error;
}

void creaReg(Tvec registros[])
{
  for (int i = 0; i < 16; i++)
    strcpy(registros[i].reg, "\0");
  strcpy(registros[0].reg, "DS");
  registros[0].pos = 0;
  strcpy(registros[1].reg, "SS");
  registros[1].pos = 1;
  strcpy(registros[2].reg, "ES");
  registros[2].pos = 2;
  strcpy(registros[3].reg, "CS");
  registros[3].pos = 3;
  strcpy(registros[4].reg, "HP");
  registros[4].pos = 4;
  strcpy(registros[5].reg, "IP");
  registros[5].pos = 5;
  strcpy(registros[6].reg, "SP"); // setear parte alta con 1
  registros[6].pos = 6;
  strcpy(registros[7].reg, "BP"); // setear parte alta con 1
  registros[7].pos = 7;
  strcpy(registros[8].reg, "CC");
  registros[8].pos = 8;
  strcpy(registros[9].reg, "AC");
  registros[9].pos = 9;
  strcpy(registros[10].reg, "AX");
  registros[10].pos = 10;
  strcpy(registros[11].reg, "BX");
  registros[11].pos = 11;
  strcpy(registros[12].reg, "CX");
  registros[12].pos = 12;
  strcpy(registros[13].reg, "DX");
  registros[13].pos = 13;
  strcpy(registros[14].reg, "EX");
  registros[14].pos = 14;
  strcpy(registros[15].reg, "FX");
  registros[15].pos = 15;
}

void creaInstrucciones(TvecMnemo vec[])
{
  strcpy(vec[0].mnemo, "MOV");
  vec[0].hex = 0x00;
  strcpy(vec[1].mnemo, "ADD");
  vec[1].hex = 0x01;
  strcpy(vec[2].mnemo, "SUB");
  vec[2].hex = 0x02;
  strcpy(vec[3].mnemo, "SWAP");
  vec[3].hex = 0x03;
  strcpy(vec[4].mnemo, "MUL");
  vec[4].hex = 0x04;
  strcpy(vec[5].mnemo, "DIV");
  vec[5].hex = 0x05;
  strcpy(vec[6].mnemo, "CMP");
  vec[6].hex = 0x06;
  strcpy(vec[7].mnemo, "SHL");
  vec[7].hex = 0x07;
  strcpy(vec[8].mnemo, "SHR");
  vec[8].hex = 0x08;
  strcpy(vec[9].mnemo, "AND");
  vec[9].hex = 0x09;
  strcpy(vec[10].mnemo, "OR");
  vec[10].hex = 0x0A;
  strcpy(vec[11].mnemo, "XOR");
  vec[11].hex = 0x0B;
  strcpy(vec[12].mnemo, "SYS");
  vec[12].hex = 0xF0;
  strcpy(vec[13].mnemo, "JMP");
  vec[13].hex = 0xF1;
  strcpy(vec[14].mnemo, "JZ");
  vec[14].hex = 0xF2;
  strcpy(vec[15].mnemo, "JP");
  vec[15].hex = 0xF3;
  strcpy(vec[16].mnemo, "JN");
  vec[16].hex = 0xF4;
  strcpy(vec[17].mnemo, "JNZ");
  vec[17].hex = 0xF5;
  strcpy(vec[18].mnemo, "JNP");
  vec[18].hex = 0xF6;
  strcpy(vec[19].mnemo, "JNN");
  vec[19].hex = 0xF7;
  strcpy(vec[20].mnemo, "LDL");
  vec[20].hex = 0xF8;
  strcpy(vec[21].mnemo, "LDH");
  vec[21].hex = 0xF9;
  strcpy(vec[22].mnemo, "RND");
  vec[22].hex = 0xFA;
  strcpy(vec[23].mnemo, "NOT");
  vec[23].hex = 0xFB;
  strcpy(vec[24].mnemo, "STOP");
  vec[24].hex = 0xFF1;
  strcpy(vec[25].mnemo, "SLEN");
  vec[25].hex = 0x0C;
  strcpy(vec[26].mnemo, "SMOV");
  vec[26].hex = 0x0d;
  strcpy(vec[27].mnemo, "SCMP");
  vec[27].hex = 0x0E;
  strcpy(vec[28].mnemo, "PUSH");
  vec[28].hex = 0xFC;
  strcpy(vec[29].mnemo, "POP");
  vec[29].hex = 0xFD;
  strcpy(vec[30].mnemo, "CALL");
  vec[30].hex = 0xFE;
  strcpy(vec[31].mnemo, "RET");
  vec[31].hex = 0xFF0;
}

int posRegistro(int pos)
{
  return pos & 0x000F;
}

void guardaReg(int pos, int val) //AH, 3 0010
{
  int sec_reg = (pos & 0x00F0) >> 4;
  int aux = posRegistro(pos);

  switch (sec_reg)
  {
  case 0:
    Reg[aux] = val;
    break;
  case 1:
     // printf ("Bajaaaa\n");
    val = val << 24; //
    val = val >> 24; //
    val = val & 0x000000FF;
    Reg[aux] = Reg[aux] & 0xFFFFFF00;
    Reg[aux] = Reg[aux] | val;
    break;
  case 2:    //0000 0000 0000 0000 0000 0000 0000 0001
    val = val << 24; //
    val = val >> 16; //
    // val = val<<8;
    val = val & 0x0000FF00;
    Reg[aux] = Reg[aux] & 0xFFFF00FF;
    Reg[aux] = Reg[aux] | val;
    break;
  case 3:
    val = val & 0x0000FFFF;
    Reg[aux] = Reg[aux] & 0xFFFF0000;
    Reg[aux] = Reg[aux] | val;
    break;
  }
}

int valRegIndirecto(int op)
{
  //int direc;
  int offset, aux = op & 0xf;
  //offset = (op >> 4) & 0xff;
  offset = op;
  offset = offset << 20;
  offset = offset >> 24;
  //printf("off %d",offset);
  return (Reg[aux] + offset);
}

int valRegistro(int op)
{
  int aux;
  int sec_reg = (op >> 4) & 0x3;
  op = op & 0x000F;
  /*   if(op == SP || op == BP){
      aux = getParteBaja(Reg[op]);
    }else */
  switch (sec_reg)
  {
  case 0:
    aux = Reg[op];
    break;
  case 1:
    aux = Reg[op] & 0x000000FF;
    aux = aux << 24;
    aux = aux >> 24;
    break;
  case 2:
    aux = (Reg[op] & 0x0000FF00);
    aux = aux << 16;
    aux = aux >> 24;
    break;
  case 3:
    aux = (Reg[op] & 0x0000FFFF);
    aux = aux << 16;
    aux = aux >> 16;
    break;
  }
  return aux;
}

void opDirectoyRegistro(int top, int *op)
{ // con esta funcion piso el valor del op con lo que hay en la direccion del registro o de la ram, no se si es correcto

  if (top == 1)
  {
    *op = valRegistro(*op);
  }
  else if (top == 2)
  {
    *op = memoriaRAM[getParteBaja(Reg[DS]) + *op];
  }
  else if (top == 3)
  {
    *op = memoriaRAM[getPosicion(valRegIndirecto(*op))];
  }
}

// calculo de direccion
int getPosicion(int valor)
{
  int valorH = getParteAlta(valor); // numero de segmento
  int valorL = getParteBaja(valor); // direc de memoria en ese segmento
  int pos;
  pos = getParteBaja(Reg[valorH]) + valorL;

  //printf("valorL %x\n",valorL);
  //printf("get %x\n",getParteAlta(Reg[valorH]));

  if (valorL <= getParteAlta(Reg[valorH]) && valorH >= 0 && valorH <= 3) // si valorL <= tamaño del segmento y el nro de segmento es valido
    return pos;
  else
  {
    printf("Segmentation Fault. Invalid memory reference. \n");
    exit(1);
  }
}

int getParteAlta(int valor)
{
  return (valor & 0xFFFF0000) >> 16;
}

int getParteBaja(int valor)
{
  return valor & 0xFFFF;
}

void setParteAlta(int *num, int val)
{
  val = (val & 0xFFFF) << 16;
  *num = *num & 0x0000FFFF;
  *num = *num | val;
}

void setParteBaja(int *num, int val)
{
  val = val & 0xFFFF;
  *num = *num & 0xFFFF0000;
  *num = *num | val;
}

void cambiaCC(int val)
{
  if (val == 0)
    Reg[8] = 0x00000001;
  else if (val < 0)
    Reg[8] = 0x80000000;
  else
    Reg[8] = 0x00000000;
}
// OPERACIONES
void MOV(int top1, int top2, int *v1, int *v2)
{
  int pos;
 
  if (top1 == 0)
  {
    *v1 = *v2;
  }
  else if (top1 == 1)
  {
    guardaReg(*v1, *v2);
  }
  else
  {
    if (top1 == 2)
    {
      pos = getPosicion(*v1);
    }
    else if (top1 == 3)
    {
      pos = getPosicion(valRegIndirecto(*v1));
    }
    memoriaRAM[pos] = *v2;
  }
}

void ADD(int top, int topB, int *v1, int *v2)
{
  int pos;
  if (top == 0)
  {
    *v1 = *v1 + *v2;
    cambiaCC(*v1);
  }
  else if (top == 1)
  {
    guardaReg(*v1, valRegistro(*v1) + *v2);
    cambiaCC(valRegistro(*v1));
  }
  else
  {
    if (top == 2)
    {
      pos = getPosicion(*v1);
    }
    else
    {
      pos = getPosicion(valRegIndirecto(*v1));
    }
    memoriaRAM[pos] += *v2;
    cambiaCC(memoriaRAM[pos]);

  }
}

void SUB(int top, int topB, int *v1, int *v2)
{
  int pos;

  if (top == 0)
  {
    *v1 = *v1 - *v2;
    cambiaCC(*v1);
  }
  else if (top == 1)
  {
    guardaReg(*v1, valRegistro(*v1) - *v2);
    cambiaCC(valRegistro(*v1));
  }
  else
  {
    if (top == 2)
    {
      pos = getPosicion(*v1);
    }
    else
    {
      pos = getPosicion(valRegIndirecto(*v1));
    }
    memoriaRAM[pos] -= *v2;
    cambiaCC(memoriaRAM[pos]);
  }
}

void MUL(int top, int topB, int *v1, int *v2)
{
  int pos;


  if (top == 0)
  {
    *v1 = *v1 * (*v2);
    cambiaCC(*v1);
  }
  else if (top == 1)
  {
    guardaReg(*v1, valRegistro(*v1) * (*v2));
    cambiaCC(valRegistro(*v1));
  }
  else
  {
    if (top == 2)
    {
      pos = getPosicion(*v1);
    }
    else
    {
      pos = getPosicion(valRegIndirecto(*v1));
    }
    memoriaRAM[pos] = memoriaRAM[pos] * (*v2);
    cambiaCC(memoriaRAM[pos]);
  }
}

void DIV(int top, int topB, int *v1, int *v2)
{
  int pos;

  if (top == 0)
  {
    Reg[9] = *v1 % (*v2);
    *v1 = (int)*v1 / (*v2);
    cambiaCC(*v1);
  }
  else if (top == 1)
  {
    Reg[9] = valRegistro(*v1) % (*v2);
    guardaReg(*v1, (int)valRegistro(*v1) / (*v2));
    cambiaCC(valRegistro(*v1));
  }
  else
  {
    if (top == 2)
    {
      pos = getPosicion(*v1);
    }
    else
    {
      pos = getPosicion(valRegIndirecto(*v1));
    }
    Reg[9] = memoriaRAM[pos] % (*v2);
    memoriaRAM[pos] = (int)memoriaRAM[pos] / (*v2);
    cambiaCC(memoriaRAM[pos]);
  }
}

void SWAP(int top1, int top2, int *v1, int *v2)
{
  int aux;

  /*if(top1 == 1){
      val1 = valRegistro(*v1);
  }else
    if(top1 == 2){
        val1 = memoriaRAM[getPosicion(*v2)];
    }else
      if(top1 == 3){
          val1 = memoriaRAM[getPosicion(valRegIndirecto(*v2))];
    }
    mov(top1,top2,v1,v2);
    mov(top2,top1,v2,&val1);*/

  if (top1 == 1 && top2 == 1)
  {
    aux = valRegistro(*v1);
    guardaReg(*v1, valRegistro(*v2));
    guardaReg(*v2, aux);
  }
  else if (top1 == 1 && top2 == 2)
  {
    aux = valRegistro(*v1);
    guardaReg(*v1, memoriaRAM[getPosicion(*v2)]);
    memoriaRAM[getPosicion(*v2)] = aux;
  }
  else if (top1 == 1 && top2 == 3)
  {
    aux = valRegistro(*v1);
    guardaReg(*v1, memoriaRAM[getPosicion(valRegistro(*v2))]);
    memoriaRAM[getPosicion(valRegistro(*v2))] = aux;
  }
  else if (top1 == 2 && top2 == 1)
  {
    aux = memoriaRAM[getPosicion(*v1)];
    memoriaRAM[getPosicion(*v1)] = valRegistro(*v2);
    guardaReg(*v2, aux);
  }
  else if (top1 == 2 && top2 == 2)
  {
    aux = memoriaRAM[getPosicion(*v1)];
    memoriaRAM[getPosicion(*v1)] = memoriaRAM[getPosicion(*v2)];
    memoriaRAM[getPosicion(*v2)] = aux;
  }
  else if (top1 == 2 && top2 == 3)
  {
    aux = memoriaRAM[getPosicion(*v1)];
    memoriaRAM[getPosicion(*v1)] = memoriaRAM[getPosicion(valRegistro(*v2))];
    memoriaRAM[getPosicion(*v2)] = aux;
  }
  else if (top1 == 3 && top2 == 1)
  {
    aux = memoriaRAM[getPosicion(valRegistro(*v1))];
    memoriaRAM[getPosicion(valRegistro(*v1))] = valRegistro(*v2);
    guardaReg(*v2, aux);
  }
  else if (top1 == 3 && top2 == 2)
  {
    aux = memoriaRAM[getPosicion(valRegistro(*v1))];
    memoriaRAM[getPosicion(valRegistro(*v1))] = memoriaRAM[getPosicion(*v2)];
    memoriaRAM[getPosicion(*v2)] = aux;
  }
  else if (top1 == 3 && top2 == 3)
  {
    aux = memoriaRAM[getPosicion(valRegistro(*v1))];
    memoriaRAM[getPosicion(valRegistro(*v1))] = memoriaRAM[getPosicion(valRegistro(*v2))];
    memoriaRAM[getPosicion(valRegistro(*v2))] = aux;
  }
}

void CMP(int top, int top2, int *v1, int *v2)
{
  int aux, pos;

  if (top == 0)
  {
    aux = *v1 - *v2;
  }
  else if (top == 1)
  {
    aux = valRegistro(*v1) - (*v2);
  }
  else
  {
    if (top == 2)
    {
      pos = getPosicion(*v1);
    }
    else
    {
      pos = getPosicion(valRegIndirecto(*v1));
    }
    aux = memoriaRAM[pos] - *v2;
  }
  cambiaCC(aux);
}

void SHL(int top, int top2, int *v1, int *v2)
{
  int pos;

  if (top == 1)
  {
    guardaReg(*v1, valRegistro(*v1) << *v2);
    cambiaCC(valRegistro(*v1));
  }
  else
  {
    if (top == 2)
    {
      pos = getPosicion(*v1);
    }
    else
    {
      pos = getPosicion(valRegIndirecto(*v1));
    }
    memoriaRAM[pos] = memoriaRAM[pos] << (*v2);
    cambiaCC(memoriaRAM[pos]);
  }
}

void SHR(int top, int top2, int *v1, int *v2)
{
  int pos;

  if (top == 1)
  {
    guardaReg(*v1, valRegistro(*v1) >> *v2);
    cambiaCC(valRegistro(*v1));
  }
  else
  {
    if (top == 2)
    {
      pos = getPosicion(*v1);
    }
    else
    {
      pos = getPosicion(valRegIndirecto(*v1));
    }
    memoriaRAM[pos] = memoriaRAM[pos] >> (*v2);
    cambiaCC(memoriaRAM[pos]);
  }
}

void AND(int top, int top2, int *v1, int *v2)
{
  int pos;

  if (top == 0)
  {
    *v1 = *v1 & *v2;
  }
  else if (top == 1)
  {
    guardaReg(*v1, valRegistro(*v1) & *v2);
    cambiaCC(valRegistro(*v1));
  }
  else
  {
    if (top == 2)
    {
      pos = getPosicion(*v1);
    }
    else
    {
      pos = getPosicion(valRegIndirecto(*v1));
    }
    memoriaRAM[pos] = memoriaRAM[pos] & (*v2);
    cambiaCC(memoriaRAM[pos]);
  }
}

void OR(int top, int top2, int *v1, int *v2)
{
  int pos;

  if (top == 0)
  {
    *v1 = *v1 | *v2;
  }
  else if (top == 1)
  {
    guardaReg(*v1, valRegistro(*v1) | *v2);
    cambiaCC(valRegistro(*v1));
  }
  else
  {
    if (top == 2)
    {
      pos = getPosicion(*v1);
    }
    else
    {
      pos = getPosicion(valRegIndirecto(*v1));
    }
    memoriaRAM[pos] = memoriaRAM[pos] | (*v2);
    cambiaCC(memoriaRAM[pos]);
  }
}

void XOR(int top, int top2, int *v1, int *v2)
{
  int pos;
  if (top == 0)
  {
    *v1 = *v1 ^ *v2;
  }
  else if (top == 1)
  {
    guardaReg(*v1, valRegistro(*v1) ^ *v2);
    cambiaCC(valRegistro(*v1));
  }
  else
  {
    if (top == 2)
    {
      pos = getPosicion(*v1);
    }
    else
    {
      pos = getPosicion(valRegIndirecto(*v1));
    }
    memoriaRAM[pos] = memoriaRAM[pos] ^ (*v2);
    cambiaCC(memoriaRAM[pos]);
  }
}

void JMP(int *v1)
{
  setParteBaja(&Reg[5], *v1);
}

void JZ(int *v1)
{
  if (Reg[8] == 0x00000001)
    setParteBaja(&Reg[5], *v1);
}

void JP(int *v1)
{
  if (Reg[8] == 0)
    setParteBaja(&Reg[5], *v1);
}

void JN(int *v1)
{
  if (Reg[8] == 0x80000000)
    setParteBaja(&Reg[5], *v1);
}

void JNZ(int *v1)
{
  if (Reg[8] != 0x00000001)
    setParteBaja(&Reg[5], *v1);
}

void JNP(int *v1)
{
  if (Reg[8] == 0x80000000 || Reg[8] == 0x00000001)
    setParteBaja(&Reg[5], *v1);
}

void JNN(int *v1)
{
  if (Reg[8] == 0 || Reg[8] == 0x00000001)
  {
    setParteBaja(&Reg[5], *v1);
  }
}

void LDL(int top, int *v1)
{
  //printf ("entre LDL\n");
  int pos;
  Reg[9] = (Reg[9] & 0xFFFF0000);
  if (top == 0)
    Reg[9] += (*v1 & 0x0000FFFF);
  else if (top == 1)
    Reg[9] += (valRegistro(*v1) & 0x0000FFFF);
  else
  {
    if (top == 2)
    {
      pos = getPosicion(*v1);
    }
    else
    {
      pos = getPosicion(valRegIndirecto(*v1));
    }
    Reg[9] += memoriaRAM[pos] & 0x0000FFFF;
  }
}

void LDH(int top, int *v1)
{
   //printf ("entre LDH\n");
  int pos;

  Reg[9] = (Reg[9] & 0x0000FFFF);
  if (top == 0)
    Reg[9] += (*v1 & 0x0000FFFF) << 16;
  else if (top == 1)
    Reg[9] += (valRegistro(*v1) & 0x0000FFFF) << 8;
  else
  {
    if (top == 2)
    {
      pos = getPosicion(*v1);
    }
    else
    {
      pos = getPosicion(valRegIndirecto(*v1));
    }
    Reg[9] += (memoriaRAM[pos] & 0x0000FFFF) << 8;
  }
}

void RND(int top, int *v1)
{
  int pos;

  if (top == 0)
    Reg[9] = rand() % (*v1 + 1);
  else if (top == 1)
    Reg[9] = rand() % (valRegistro(*v1) + 1);
  else
  {
    if (top == 2)
    {
      pos = getPosicion(*v1);
    }
    else
    {
      pos = getPosicion(valRegIndirecto(*v1));
    }
    Reg[9] = rand() % (memoriaRAM[pos] + 1);
  }
}

void NOT(int top, int *v1)
{
  int pos;

  if (top == 0)
  {
    *v1 = ~(*v1);
    cambiaCC(*v1);
  }
  else if (top == 1)
  {
    guardaReg(*v1, ~valRegistro(*v1));
    cambiaCC(valRegistro(*v1));
  }
  else
  {
    if (top == 2)
    {
      pos = getPosicion(*v1);
    }
    else
    {
      pos = getPosicion(valRegIndirecto(*v1));
    }
    memoriaRAM[pos] = ~memoriaRAM[pos];
    cambiaCC(memoriaRAM[pos]);
  }
}

void PUSH(int *valor) // push [1001]
{
  if (getParteBaja(Reg[SP]) >= 0) //>=0 ???????? la parte alta tiene 1, el codigo de segmento de SS
  {
    setParteBaja(&Reg[SP], getParteBaja(Reg[SP]) - 1); // Decrementamos el SP (solo la direccion, no tocamos el segmento)
    /*if (top == 0)*/
    memoriaRAM[getPosicion(Reg[SP])] = *valor;
    /*else
        if (top == 1)
            memoriaRAM[getPosicionAbsoluta(Reg[SP])] = valRegistro(*valor);
        else
            memoriaRAM[getPosicionAbsoluta(Reg[SP])] = memoriaRAM[getPosicionAbsoluta(*valor)]*/
    //printf("stack: %d\n",memoriaRAM[getPosicion(Reg[SP])]);
  }
  else
  {
    printf("OverFlow en la pila\n");
    exit(1);
  }
}

void POP(int top, int *valor) // Extrae el dato del tope de pila y lo almacena en el operando (que puede ser registro, memoria o
                              // indirecto).
{
  if (getParteAlta(Reg[SS]) > getParteBaja(Reg[SP]))
  {
    if (top == 0)
      *valor = memoriaRAM[getPosicion(Reg[SP])];
    else if (top == 1)
      guardaReg(*valor, memoriaRAM[getPosicion(Reg[SP])]);
    else
      memoriaRAM[getPosicion(*valor)] = memoriaRAM[getPosicion(Reg[SP])];

    setParteBaja(&Reg[SP], getParteBaja(Reg[SP]) + 1);
  }
  else // pila vacia SP(L)>=SS(H)
  {
    printf("UnderFlow en la pila\n");
    exit(1);
  }
}

void SYS(int *v1)
{
  switch (*v1)
  {
  case 1:
    leer();
    break;
  case 2:
    escribir();
    break;
  case 3:
    stringRead();
    break;
  case 4:
    stringWrite();
    break;
  case 13:
    accesoDisco();
    break;
  case 15:
    breakpoint();
    break;
  }
}

void SLEN(int top1, int top2, int *v1, int *v2)
{
  int pos, largo = 0;
  //printf("entre \n");
  if (top2 == 2)
  {
    pos = getPosicion(*v2);
  }
  else
  {
    if (top2 == 3)
    {
      pos = getPosicion(valRegIndirecto(*v2));
    }
  }

  while (memoriaRAM[pos] != '\0')
  {
    //printf("%c",memoriaRAM[pos]);
    largo++;
    pos++;
  }

  if (top1 == 1)
  {
    guardaReg(*v1, largo);
  }
  else
  {
    if (top1 == 2)
    {
      pos = getPosicion(*v1);
    }
    else if (top1 == 3)
    {
      pos = getPosicion(valRegIndirecto(*v1));
    }
    memoriaRAM[pos] = largo;
  }
  //printf("\nLargo %d\n",largo);
}

void SMOV(int top1, int top2, int *v1, int *v2)
{
  int pos1, pos2;

  pos1 = posDirectoIndirecto(top1, *v1);
  pos2 = posDirectoIndirecto(top2, *v2);

  do
  {
    memoriaRAM[pos1] = memoriaRAM[pos2];
    pos1++;
    pos2++;
  } while (memoriaRAM[pos2] != '\0');
}

void SCMP(int top1, int top2, int *v1, int *v2)
{
  int pos1, pos2, resta = 0;

  pos1 = posDirectoIndirecto(top1, *v1);
  pos2 = posDirectoIndirecto(top2, *v2);

/*
   while (resta == 0 && memoriaRAM[pos1] == '\0' && memoriaRAM[pos2] == '\0')
  {
    resta = (memoriaRAM[pos1++] - memoriaRAM[pos2++]);
  }
 */
   while (resta == 0 && memoriaRAM[pos1] != '\0' && memoriaRAM[pos2] != '\0')
  {
    resta = memoriaRAM[pos1++] - memoriaRAM[pos2++];
  }

   if(resta == 0 && (memoriaRAM[pos1] == '\0' && memoriaRAM[pos2] != '\0')) // la especificacion no dice que hacer cuando los strings son iguales parcialmente
    resta = -1;                                                              // por ejemplo "holi", "holixd" ------> sin los ifs devuelve que son iguales
  else
    if(resta == 0 && (memoriaRAM[pos1] != '\0' && memoriaRAM[pos2] == '\0'))
      resta = 1;

  cambiaCC(resta);
}

void CALL(int *op)
{
  int aux = Reg[IP];
  PUSH(&aux);
  setParteBaja(&Reg[IP],*op);
}

void RET()
{
  // int aux;

  int reg = IP;

  POP(1, &reg);

  // POP(0,&aux);
  // Reg[IP] = aux;
}

void STOP()
{
  ejecutando = 0;
}

int posDirectoIndirecto(int top, int op)
{

  int pos;

  if (top == 2)
  {
    pos = getPosicion(op);
  }
  else if (top == 3)
  {
    pos = getPosicion(valRegIndirecto(op));
  }

  return pos;
}

void leer()
{
  int prompt = (Reg[10] >> 10) & 0x1; // Reg[10] & 0x1000
  //int texto = (Reg[10] >> 7) & 0x1;
  int texto = (Reg[10] >> 8);
  int formato = Reg[10] & 0x0000000F; // 4 bits menos significativos -> 1 = dec , 4 = octal , 8 = hexa

  int desde = Reg[EDX] + getParteBaja(Reg[DS]);
  int cantCeldas = Reg[ECX];
  int aux;
  char buffer[100];

  printf("texto %d\n", texto);

  if (!texto)
  {
    for (int i = 0; i < cantCeldas; i++)
    {
      if (!prompt) // debo escribir un prompt[0000]: con la direccion de memoria en decimal
        printf("[%04d]", desde + i);
      switch (formato)
      {
      case 1:
        scanf(" %d", &aux);
        break;
      case 4:
        scanf(" %o", &aux);
        break;
      case 8:
        scanf(" %x", &aux);
        break;
      }
      memoriaRAM[desde + i] = aux;
    }
  }
  else
  {
    //printf("lee\n");
    if (!prompt)
      printf("[%04d]", desde);
    scanf(" %s", buffer);
    int max = strlen(buffer) > (cantCeldas - 1) ? (cantCeldas - 1) : strlen(buffer);
    for (int i = 0; i < max; i++)
    {
      memoriaRAM[desde + i] = buffer[i];
    }
    memoriaRAM[desde + max] = '\0';
  }
}

void escribir()
{
  int prompt = (Reg[EAX] >> 11);
  int endline = (Reg[EAX] >> 8);
  int car = (Reg[EAX] >> 4);
  int formato = Reg[EAX] & 0xF; // 1 = dec , 4 = octal , 8 = hexa
  //int desde = getPosicion(Reg[EDX] + getParteBaja(Reg[DS]));
  int desde = getPosicion(Reg[EDX]);

  int cantCeldas = Reg[ECX];
  //printf("desde %d\n",desde);
  for (int i = 0; i < cantCeldas; i++)
  {
    if (!prompt)
    {
      //printf("[%04d]", getPosicion(Reg[EDX] + i));
      printf("[%04d]", getParteBaja(Reg[EDX]) + i);
    }
    if (!car)
    {
      if (formato & 0x8)
        printf("%c%X ", '%', memoriaRAM[desde + i]);
      if (formato & 0x4)
        printf("@%o ", memoriaRAM[desde + i]);
      if (formato & 0x1)
      {
        printf("%d ", memoriaRAM[desde + i]);
      }
    }
    else
    {
      int letra = memoriaRAM[desde + i] & 0x000000FF;;
      if (letra <= 126 && letra >= 32)
        printf("%c ", letra);
      else
        printf(". ");
    }
    if (!endline)
      printf("\n");
  }
}

void breakpoint()
{
  int k = 0, j = 0, i;
  int posDis = posDisassembler;
  int ip = getParteBaja(Reg[5]);
  char rta[20], cad[500];
  if (flagC)
    system("cls");
  if (flagB)
  {
    printf("[%04d] cmd: ", getParteBaja(Reg[5]));
    fflush(stdin);
    fgets(rta, 15, stdin);
    printf("\n");
    if (rta[0] == 'p')
      pasoApaso(rta);
    else if (rta[0] != 'r') // rta=numero decimal
      muestraValores(rta);
  }
  if (flagD)
  {
    // Muestra 10 lineas a partir del IP 3 antes y 6 despues
    printf("Codigo:\n");
    if (ip - 3 >= 0)
      for (i = ip - 3; i <= ip + 6; i++)
      {
        if (i == ip)
          printf(">");
        printf(" %s\n", Disassembler[i].cad);
      }
    else
      for (i = 0; i < 10; i++)
      {
        if (i == ip)
          printf(">");
        printf("%s\n", Disassembler[i].cad);
      }
    printf("%s", "Registros: \n");
    for (k = 1; k <= 4; k++)
    {
      while (j < k * 4)
      { // 0<4
        if (strcmp(registros[j].reg, " ") != 0)
          printf("%s =      %d|", registros[j].reg, getParteBaja(Reg[j])); // ver formato reg
        else
          printf("            |");
        j++;
      }
      printf("\n");
    }
    printf("[%04d] cmd: %d\n\n\n", getParteBaja(Reg[5]), i);
  }
}

void stringRead()
{
  char entrada[100];
  fflush(stdin);
  fgets(entrada, 20, stdin);
  int i = 0;
  while (entrada[i] != '\n' && entrada[i] != '\0' && i < Reg[12]) // Reg[12] = CX -> cantidad maxima de caracteres
  {
    memoriaRAM[getPosicion(Reg[13] + i)] = entrada[i];
    i++;
  }
  memoriaRAM[getPosicion(Reg[13] + i)] = '\0';
}

void stringWrite()
{
  int prompt = (Reg[EAX] >> 11);
  int endline = (Reg[EAX] >> 8);
  char salida[100];
  int i = 0;
  int pos = getPosicion(Reg[13]);
  if (!prompt)
    printf("[%04d]", pos);
  do
  {
    salida[i] = memoriaRAM[pos + i];
    i++;
  } while (memoriaRAM[pos + i] != '\0');
  printf("%s\n", salida);
  if (!endline)
    printf("\n");
}

void accesoDisco()
{
  int operacion = (getParteBaja(Reg[EAX]) & 0xFF00) >> 8;
  int cantSectores = getParteBaja(Reg[EAX]) & 0xFF;
  int cantSectoresActuales = 0;
  int nroCil = (getParteBaja(Reg[ECX]) & 0xFF00) >> 8;
  int nroCab = getParteBaja(Reg[ECX]) & 0xFF;
  int nroSector = (getParteBaja(Reg[EDX]) & 0xFF00) >> 8;
  int nroDisco = getParteBaja(Reg[EDX]) & 0xFF;
  int celda = Reg[EBX];
  int nroSegmento = getParteAlta(celda);
  int inicioBuffer = getParteBaja(celda);
  int i = 0, temp = 0;

  unsigned int cantCilindros = vecDisco[nroDisco].cantCil;
  unsigned int cantSectoresTotal = vecDisco[nroDisco].cantSec;
  unsigned int tamSector = vecDisco[nroDisco].Ssize;
  unsigned long long nroByte = 0;

  int auxExtiende;
  int cantLeer;
  unsigned int espacioDiscoCreado;
  unsigned int tamDisco;

  if (nroDisco >= 0 || nroDisco < sizeof(vecDisco))
  {
    if (operacion == 0x08)
    {                                                                            // obtener los par del disco -> va aca pq no tiene q verificar nada mas que el nro de disco
      Reg[EAX] = Reg[EAX] & 0xFFFF00FF;                                          // AH = %00 -> operacion exitosa
      Reg[ECX] = (vecDisco[nroDisco].cantCil << 8) + vecDisco[nroDisco].cantCab; // CH = cantCil y CL = cantCab
      Reg[EDX] = vecDisco[nroDisco].cantSec << 8;                                // DH=cantSectores
    }
    else
    {
      if (nroCil > cantCilindros || nroCab > vecDisco[nroDisco].cantCab || nroSector > cantSectores)
        Reg[EAX] = (Reg[EAX] & 0xFFFF00FF) + 0x0B00;
      if (nroCab > vecDisco[nroDisco].cantCab)
        Reg[EAX] = (Reg[EAX] & 0xFFFF00FF) + 0x0C00;
      if (nroSector > cantSectores)
        Reg[EAX] = (Reg[EAX] & 0xFFFF00FF) + 0x0D00;
      if (operacion != 0x00 || operacion != 0x02 || operacion != 0x03)
      {
        printf("Funcion invalida.\n");
        Reg[EAX] = (Reg[EAX] & 0xFFFF00FF) + 0x0100;
      }
      if (((getParteBaja(Reg[EAX]) & 0xFF00) >> 8) == 0)
      { // no hay ningun error en los parametros del disco
        nroByte = tamHeader + nroCil * cantCilindros * cantSectoresTotal * tamSector + nroCab * cantSectoresTotal * tamSector + nroSector * tamSector;
        //////CREO QUE ACA HAY QUE CHEQUEAR EL TAMANO DINAMICO DEL ARCHIVO/DISCO
        fseek(vecDisco[nroDisco].arch, 2, SEEK_END); // voy al final del disco
        tamDisco = cantCilindros * vecDisco[nroDisco].cantCab * cantSectoresTotal * tamSector;
        espacioDiscoCreado = ftell(vecDisco[nroDisco].arch) - 2;                                          // el archivo binario vacio ya ocupa 2 bytes
        if (nroByte + 512 * cantSectores < tamDisco && nroByte + 512 * cantSectores > espacioDiscoCreado) // hay que extender el disco
        {                                                                                                 // leemos/ escribimos desde nroByte hasta nroByte + 128*cantSectores
          auxExtiende = 0;
          cantLeer = (int)(nroByte + 512 * cantSectores - espacioDiscoCreado) / 4;
          for (int j = 0; j < cantLeer; j++)
            fwrite(&auxExtiende, sizeof(auxExtiende), 1, vecDisco[nroDisco].arch);
        }
        fseek(vecDisco[nroDisco].arch, nroByte, SEEK_SET); // se posiciona en el comienzo de donde hay q empezar a leer / esc
        if (operacion == 0x00)
        { // consultar ultimo estado
          printf("%02X", (getParteBaja(Reg[EAX]) & 0xFF00) >> 8);
        }
        else
        {
          // if(tamDisco > nroByte + 512*cantSectores)
          if (operacion == 0x02)
          { // leer el disco ->EBX = 0x 0000 0002 0000 0065
            if (inicioBuffer + (512 / 4) * cantSectores < getParteAlta(Reg[nroSegmento]))
            { // parte alta del segmento = cantidad de celdas del segmento
              if (nroSegmento == 0x2)
                Reg[HP] = inicioBuffer + 128 * cantSectores;
              while (i < 128 * cantSectores && fread(&temp, sizeof(temp), 1, vecDisco[nroDisco].arch) == 1)
              {
                memoriaRAM[getPosicion(celda + i)] = temp;
                i++;
                if (128 % i == 0) // sector completo leido
                  cantSectoresActuales++;
              }
              if (i < 128 * cantSectores)
              {                                   // el disco no tiene mas info
                Reg[EAX] = Reg[EAX] & 0xFFFF0000; // AH = %00 -> operacion exitosa
                Reg[EAX] += cantSectoresActuales; // AL = cantSectores Leidos
              }
            }
            else
            {
              printf("Error en la lectura.\n");
              Reg[EAX] = (Reg[EAX] & 0xFFFF00FF) + 0x0400;
            }
          }
          else
          { // escribir el disco
            if (inicioBuffer + 128 * cantSectores < getParteAlta(Reg[nroSegmento]))
            {
              while (i < 128 * cantSectores) // i == 0;
              {
                temp = memoriaRAM[getPosicion(celda + i)];
                if (fwrite(&temp, sizeof(temp), 1, vecDisco[nroDisco].arch) != sizeof(temp))
                  i++;
                else
                {
                  printf("Falla en la operacion.\n");
                  Reg[EAX] = (Reg[EAX] & 0xFFFF00FF) + 0xFF00;
                  exit(1);
                }
              }
            }
            else
            {
              printf("Falla de escritura.\n");
              Reg[EAX] = (Reg[EAX] & 0xFFFF00FF) + 0xCC00;
            }
          }
        }
      }
    }
  }  
  else
  { // no existe nroDisco
    printf("No existe el disco.\n");
    Reg[EAX] = (Reg[EAX] & 0xFFFF00FF) + 0x3100;
  }
}

void escribeOp(int top, int op, char cad[])
{
  char letras[] = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 'A', 'B', 'C', 'D', 'E', 'F'};

  if (top == 1)
  {
    int sec_reg = (op & 0x00F0) >> 4;
    op = op & 0x000F;
    switch (sec_reg)
    {
    case 0:
      sprintf(cad, "%s, ", registros[op].reg);
      break;
    case 1:
      sprintf(cad, "%cL, ", letras[op]);
      break;
    case 2:
      sprintf(cad, "%cH, ", letras[op]);
      break;
    case 3:
      sprintf(cad, "%cX, ", letras[op]);
      break;
    }
  }
  else
  {
    if (top == 2)
    {
      sprintf(cad, "[%x] ", op);
    }
    else
     // if(top == 0)
        sprintf(cad, "%d ", op);
     // else
       // sprintf(cad,"[%s]",op);
  }
}
void disassembler(int instruccion)
{
  char cad1[100] = {'\0'};
  char cad2[100] = {'\0'};
  int k = 0;
  
  if(instruccion >> 20){
    int cod = 0x0, topA = 0x0, topB = 0x0, opA = 0x0, opB = 0x0, cantOp = 0x0;

   sprintf(cad1, "[%04d]: %02X %02X %02X %02X\t %d: \t", posDisassembler, (instruccion >> 24) & 0xFF, (instruccion >> 16) & 0xFF, (instruccion >> 8) & 0xFF, (instruccion >> 0) & 0xFF, posDisassembler + 1);
   decodificaInstruccion(instruccion, &cod, &topA, &topB, &opA, &opB, &cantOp);
    
    
    while (cod != inst[k].hex){
      k++;
    }
    sprintf(cad2, "%s     ", inst[k].mnemo);
    strcat(cad1, cad2);
    if(cantOp == 2 || cantOp == 1){
      escribeOp(topA, opA, cad2);
      strcat(cad1, cad2);
    }
      
    if(cantOp == 2){
      escribeOp(topB, opB, cad2);
      strcat(cad1, cad2);
    }
    strcpy(Disassembler[posDisassembler].cad, cad1);
    // Disassembler[posDisassembler] = cad1;
    printf("%s\n", Disassembler[posDisassembler].cad);
    posDisassembler++;
  }
}

void muestraValores(char rta[])
{
  char cad1[15] = {"\0"}, cad2[15] = {"\0"};
  int rtaI1, rtaI2;
  if (strlen(rta) > 0)
  {
    desarmaRespuesta(rta, cad1, cad2);
    // Caso 1 solo valor
    if (strlen(cad2) == 0)
    {
      rtaI1 = strtol(cad1, NULL, 10); // Lo transformo en int
      printf("[%04d]: %04X %04X %d\n", rtaI1, (memoriaRAM[rtaI1] >> 16) & 0x0000FFFF, memoriaRAM[rtaI1] & 0x0000FFFF, memoriaRAM[rtaI1]);
    }
    // Caso rango de valores
    else
    {
      rtaI1 = strtol(cad1, NULL, 10);
      rtaI2 = strtol(cad2, NULL, 10);
      for (; rtaI1 <= rtaI2; rtaI1++)
        printf("[%04d]: %04X %04X %d\n", rtaI1, (memoriaRAM[rtaI1] >> 16) & 0x0000FFFF, memoriaRAM[rtaI1] & 0x0000FFFF, memoriaRAM[rtaI1]);
    }
    printf("\n");
  }
}

void desarmaRespuesta(char rta[], char cad1[], char cad2[])
{
  int i = 0;
  int j = 0;
  while (rta[i] != '\0' && rta[i] != ' ')
  {
    cad1[j] = rta[i];
    i++;
    j++;
  }
  i++;
  j = 0;
  while (rta[i] != '\0' && rta[i] != ' ')
  {
    cad2[j] = rta[i];
    i++;
    j++;
  }
}

void pasoApaso(char rta[])
{
  // Ya se hizo el Reg[5]++
  while (rta[0] == 'p' && Reg[5] <= header[1])
  {
    step();
    printf("[%04d] cmd: ", Reg[5]);
    fflush(stdin);
    fgets(rta, 20, stdin);
    printf("\n");
    if (strlen(rta) > 0)
      muestraValores(rta);
  }
}

void ejecutaInstruccion(int cod, int topA, int topB, int *opA, int *opB, int cantOp)
{

  int cod1[] = {0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xfe, 0xfc, 0xf8, 0xf9, 0xfa, 0xfb};

  int i;

  void (*fun1[])(int *) = {SYS, JMP, JZ, JP, JN, JNZ, JNP, JNN, CALL, PUSH};

  void (*fun2[])(int, int, int *, int *) = {MOV, ADD, SUB, SWAP, MUL, DIV, CMP, SHL, SHR, AND, OR, XOR, SLEN, SMOV, SCMP};

  if (cantOp == 2)
  {
    if (cod == 3) // swap
      SWAP(topA, topB, opA, opB);
    else
      if(cod == 0xc)
        SLEN(topA, topB, opA, opB);
      else
        if(cod == 0xd)
          SMOV(topA, topB, opA, opB);
        else
          if(cod == 0xe)
            SCMP(topA, topB, opA, opB);
          else
    {
      opDirectoyRegistro(topB, opB);
      (*fun2[cod])(topA, topB, opA, opB);
    }
  }
  else
  {
    if (cantOp == 1)
    {
      if (cod < 0xf8 || cod == 0xfe || cod == 0xfc)
      {
        i = 0;
        while (i <= 9 && cod1[i] != cod)
          i++;
        opDirectoyRegistro(topA, opA);
        (*fun1[i])(opA);
      }
      else if (cod == 0xf8)
        LDL(topA, opA);
      else if (cod == 0xf9)
        LDH(topA, opA);
      else if (cod == 0xfa)
        RND(topA, opA);
      else if (cod == 0xfb) // not
        NOT(topA, opA);
      else if (cod == 0xfd)
        POP(topA, opA);
    }
    else if (cod == 0xff1)
      STOP();
    else
      RET();
  }
}
