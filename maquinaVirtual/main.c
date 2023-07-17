#include <stdio.h>
#include <stdlib.h>
#include "parser.h"
#include <string.h>
#include <ctype.h>
#include "verificaOperandos.h"

#define MAX 32
typedef struct
{
    char mnemo[10];
    int hex;
} Tvec;

/*typedef struct constante
{
    char equ[50];
    int valor;
    struct constante *sig;
} constante;*/

int contSimbolos = 0;
int contRotulos = 0;
int offset = 0;

TSimboloS strings[20];
TSimboloC constantes[20];
TRotulo rotulos[20];



void CargaArchivo(char *argv[], int *contInstruccion, int v[], int *error);
void creaInstruccion(char **parsed, int *error, int *contInstruccion, int v[], char *argv[]);
void verificaInst(char mnemo[], int cantOperandos, int *insMnemo, int *cumple);
int generaInstruccion(int mnemo, int operandoA, int operandoB, int tipoOperandoA, int tipoOperandoB, int cantOperandos);
void GuardaBinario(char *argv[], int contInstruccion, int v[], int error);
void creaReg(Tvec vec[]);
void Mayusculas(char s[]);
void muestra(int v[], int n);
int buscarRotulo(char *rot);
void muestraInstruccion(int instruccion, char rotulo[], char mnemo[], char operandoA[], char operandoB[], int contInstruccion, char comentario[]);
int getSimbolos(FILE *arch);
int buscaDuplicados(char parsed[], TSimboloC constantes[], int n, TSimboloS strings[], int m);
int header[6] = {0, 1024, 1024, 1024, 0, 0};

int main(int argc, char *argv[])
{
    int v[500] = {0};
    int contInstruccion = 0;
    int error = 0;
    CargaArchivo(argv, &contInstruccion, v, &error);
    if (!error)
        GuardaBinario(argv, contInstruccion, v, error);
    else
        printf("El archivo binario no fue generado");
    return 0;
}

void CargaArchivo(char *argv[], int *contInstruccion, int v[], int *error)
{
    FILE *arch;
    char linea[256];
    arch = fopen(strcat(argv[1], ".asm"), "rt");
    int duplicado = 0;
    if (arch == NULL)
    {
        printf("Error no se ha encontrado el archivo especificado\n");
    }
    else
    {
        fseek(arch, 0, SEEK_SET);
        duplicado = getSimbolos(arch);
        if (!*error && duplicado)
            *error = duplicado;
        fseek(arch, 0, SEEK_SET);
        while (fgets(linea, 256, arch))
        {
            char **parsed = parseline(linea);
            if (parsed != NULL)
            {
                creaInstruccion(parsed, error, contInstruccion, v, argv);
            }
            else
            {
                printf("Error en el parseo\n");
            }
        }
        fclose(arch);
    }
    if (*error)
        printf("Error: hay error de simbolo duplicado o indefinido\n");
}

void creaInstruccion(char **parsed, int *error, int *contInstruccion, int v[], char *argv[])
{
    int cantOperandos = 0;
    int flagError = 0;
    int mnemo;
    // int tipoOperandoA = 3;    // estaba inicializado asi, ahora hay tipo de op 3, asi que lo saque
    // int tipoOperandoB = 3;
    int tipoOperandoA;
    int tipoOperandoB;
    int operandoA;
    int operandoB;
    int errorMnemo = 0;

    if (parsed[2] != NULL && parsed[3] != NULL)
    {
        cantOperandos = 2;
    }
    else if (parsed[2] != NULL)
    {
        cantOperandos = 1;
    }

    if (parsed[2] != NULL)
    {
        Mayusculas(parsed[2]);
        // Mayusculas(parsed[1]);
        if (parsed[1][0] == 'J' || parsed[1][0] == 'j' || strcmp(parsed[1], "CALL") == 0 || strcmp(parsed[1], "call") == 0)
        {
            operandoA = buscarRotulo(parsed[2]);
            if(operandoA == -1){
                printf("No se encuentra el rotulo\n");
            }
            tipoOperandoA = 0;
        }
        else
        {
            // printf("op %s\n",parsed[2]);
            devuelveOperando(parsed[2], &tipoOperandoA, &operandoA);
            // printf("op %x\n",operandoA);
            // printf("top %x\n",tipoOperandoA);
        }
    }
    if (parsed[3] != NULL)
    {
        /* if (parsed[3][0] == 39)
        {
            devuelveOperando(parsed[3], &tipoOperandoB, &operandoB);
        }
        else
        {  */  
            //printf("op %s\n",parsed[3]);
            Mayusculas(parsed[3]);
            devuelveOperando(parsed[3], &tipoOperandoB, &operandoB);
             //printf("op %x\n",operandoB);
             //printf("top %x\n",tipoOperandoB);
        //}
    }
    if (parsed[2] != NULL && parsed[3] != NULL)
    {
        int aux = operandoB;
        if (operandoB < 0)
        {
            operandoB = operandoB & 0xFFF;
        }
        if (abs(operandoA) > 2048)
        {
            operandoA = operandoA & 0x00000FFF;
            printf("ADVERTENCIA: Se ha truncado el valor del operando --> %s!\n", parsed[2]);
        }
        if (abs(operandoB) > 2048 && aux > 0)
        {
            operandoB = operandoB & 0x00000FFF;
            printf("ADVERTENCIA: Se ha truncado el valor del operando --> %s!\n", parsed[3]);
        }
    }
    else if (parsed[2] != NULL)
    {
        if ((operandoA >> 16) > 0)
        {
            operandoA = operandoA & 0x0000FFFF;
            printf("ADVERTENCIA: Se ha truncado el valor del operando --> %s!\n", parsed[2]);
        }
    }
    if (parsed[1] != NULL)
    {
        Mayusculas(parsed[1]);
        verificaInst(parsed[1], cantOperandos, &mnemo, &errorMnemo);

        if ((errorMnemo != -1 && (parsed[2] != NULL || parsed[3] != NULL)) || (errorMnemo != -1 && ((strcmp(parsed[1], "STOP") == 0) || (strcmp(parsed[1], "RET") == 0))))
        {
            v[*contInstruccion] = generaInstruccion(mnemo, operandoA, operandoB, tipoOperandoA, tipoOperandoB, cantOperandos);
        }
        else
        {
            if (parsed[1][0] == 'J' || parsed[1][0] == 'j')
            {
                v[*contInstruccion] = 0xf1000fff;
            }
            else
            {
                v[*contInstruccion] = 0xffffffff;
            }
            flagError = 1;
            *error = 1;
        }
        if (!flagError)
        {
            if (argv[3] != NULL && strcmp(argv[3], "-o") == 0)
            {
                muestraInstruccion(v[*contInstruccion], parsed[0], parsed[1], parsed[2], parsed[3], *contInstruccion, parsed[4]);
            }
        }
        else
        {
            printf("  Error en la instruccion --> ");
            muestraInstruccion(v[*contInstruccion], parsed[0], parsed[1], parsed[2], parsed[3], *contInstruccion, parsed[4]);
        }
        (*contInstruccion)++;
    }
    if (parsed[5] != NULL)
    {
        if (strcmp(parsed[5], "DATA") == 0)
        {
            header[1] = atoi(parsed[6]);
        }
        else if (strcmp(parsed[5], "EXTRA") == 0)
        {
            header[3] = atoi(parsed[6]);
        }
        else if (strcmp(parsed[5], "STACK") == 0)
        {
            header[2] = atoi(parsed[6]);
        }
        else
        {
            printf("error en la definicion de la directiva\n");
        }
        printf("%s\t", parsed[5]);
        printf("%s\n", parsed[6]);
    }
    if (parsed[7] != NULL)
    {
        Mayusculas(parsed[7]);
        Mayusculas(parsed[8]);
        printf("%s\t", parsed[7]);
        printf("EQU %s\n", parsed[8]);
    }
}

void creaReg(Tvec vec[])
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
    vec[26].hex = 0x0D;
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

void verificaInst(char mnemo[], int cantOperandos, int *insMnemo, int *errorMnemo)
{
    Tvec vec[MAX];
    creaReg(vec);
    int cantOp[MAX] = {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 2, 2, 2, 1, 1, 1, 0};
    int i = 0;
    while (i <= MAX && strcmp(mnemo, vec[i].mnemo) != 0)
        i++;
    if (i < MAX && strcmp(mnemo, vec[i].mnemo) == 0 && cantOp[i] == cantOperandos)
        *insMnemo = vec[i].hex;
    else
        *errorMnemo = -1;
}

int generaInstruccion(int mnemo, int operandoA, int operandoB, int tipoOperandoA, int tipoOperandoB, int cantOperandos)
{
    int aux = 0x00000000;
    if (cantOperandos == 2)
    {
        operandoA = operandoA & 0xfff;
        operandoB = operandoB & 0xfff;
        aux = mnemo;
        aux = aux << 2;
        aux += tipoOperandoA;
        aux = aux << 2;
        aux += tipoOperandoB;
        aux = aux << 12;
        aux += operandoA;
        aux = aux << 12;
        aux += operandoB;
    }
    else if (cantOperandos == 1)
    {
        operandoA = operandoA & 0xffff;
        aux = aux << 4;
        aux = mnemo;
        aux = aux << 2;
        aux += tipoOperandoA;
        aux = aux << 22;
        aux += operandoA;
    }
    else if (cantOperandos == 0)
    {
        aux = mnemo;
        aux = aux << 20;
    }
    return aux;
}

void GuardaBinario(char *argv[], int contInstruccion, int v[], int error)
{
    if (!error)
    {
        FILE *arch;
        //printf("offset %d",offset);
        arch = fopen(strcat(argv[2], ".mv2"), "wb");
        header[0] = 0x4d562d32;
        header[4] = offset;
        header[5] = 0x562e3232;
        fwrite(&header[0], sizeof(int), 1, arch);
        fwrite(&header[1], sizeof(int), 1, arch);
        fwrite(&header[2], sizeof(int), 1, arch);
        fwrite(&header[3], sizeof(int), 1, arch);
        fwrite(&header[4], sizeof(int), 1, arch);
        fwrite(&header[5], sizeof(int), 1, arch);
        for (int i = 0; i < contInstruccion; i++)
        {
            fwrite(&v[i], sizeof(int), 1, arch);
        }
        int i = 0;
        while (i < contSimbolos)
        {
            int j = 0;
            int equ;
            do
            {
                //printf("%c",strings[i].constante[j]);
                equ = strings[i].constante[j];
                fwrite(&equ, sizeof(int), 1, arch);
                //j++;
            }while (strings[i].constante[j++] != '\0');
            //equ = strings[i].constante[j];
            //fwrite(&equ, sizeof(int), 1, arch);
            i++;
        }
        fclose(arch);
        printf("\n");
        printf("Archivo Binario Generado con exito.\n");
    }
    else
    {
        printf("Error en la traduccion, el archivo binario no fue generado!\n");
    }
}

void muestraInstruccion(int instruccion, char rotulo[], char mnemo[], char operandoA[], char operandoB[], int contInstruccion, char comentario[])
{
    //printf("[%04d] \t %04X\t", contInstruccion, instruccion);
    if(rotulo == NULL)
        printf("[%04d]: %02X %02X %02X %02X:\t%d:\t", contInstruccion, (instruccion >> 24) & 0xFF, (instruccion >> 16) & 0xFF, (instruccion >> 8) & 0xFF, (instruccion >> 0) & 0xFF,contInstruccion+1);
    else
        printf("[%04d]: %02X %02X %02X %02X:\t%s:\t", contInstruccion, (instruccion >> 24) & 0xFF, (instruccion >> 16) & 0xFF, (instruccion >> 8) & 0xFF, (instruccion >> 0) & 0xFF,rotulo);

    if (mnemo != NULL)
        printf("%s\t", mnemo);
    else
        printf("\t");

    if (operandoA != NULL)
    {
        if (operandoB != NULL)
            printf("%s,%s\t", operandoA, operandoB);
        else
            printf("%s\t", operandoA);
    }
    else
        printf("\t");
    if (comentario != NULL)
        printf(";%s", comentario);
    printf("\n");
}

void Mayusculas(char s[])
{
    char aux[strlen(s)];
    int i;
    for (i = 0; i < strlen(s); i++)
    {
        aux[i] = toupper(s[i]);
    }
    aux[i] = '\0';
    strcpy(s, aux);
}

int buscarRotulo(char *rot)
{
    int i = 0;
    int res = -1;
    while (i < contRotulos && strcmp(rotulos[i].simbolo,rot) != 0)
        i++;
    if(i < contRotulos)
        res = rotulos[i].posReferencia;
    return res;
}

// la funcion tiene que recorrer todo el archivo, buscando los EQU y donde se llama la constante
// reemplazar por el valor como operando inmediato

int getSimbolos(FILE *arch)
{
    char linea[256];
    int i = 0;
    int j = 0;
    int k = 0;
    int o = 0,r = 0;

    int cantLineas = 0;
    int duplicado = 0;
    char aux [10];
    while (fgets(linea, 256, arch))
    {
        char **parsed = parseline(linea);
        if (parsed != NULL)
        {
            if (parsed[7] != NULL && parsed[8] != NULL)
            {
                duplicado = buscaDuplicados(parsed[7], constantes, i, strings, j);
                if (duplicado)
                    printf("Hay simbolos duplicados\n");
                if (parsed[8][0] != '"')
                {
                    strcpy(constantes[i].simbolo, parsed[7]);
                    Mayusculas(constantes[i].simbolo);
                    //char aux[strlen(parsed[8])];
                    /*
                    for (int i = 1; i < strlen(parsed[8]); i++)
                    {
                        aux[i] = parsed[8][i];
                    } */

                    switch (parsed[8][0])
                    {
                    case '#':
                        o = 0;
                        r = 1;
                        do
                        {
                            aux[o] = parsed[8][r];
                            o++;
                            r++;
                        }while (parsed[8][r] != '\0');
                        constantes[i].constante = decimal(aux);
                        //printf("aux: %d\n", constantes[i].constante);
                        break;
                    case '@':
                       /*  o = 0;
                        r = 1;
                        do
                        {
                            aux[o] = parsed[8][r];
                            o++;
                            r++;
                        }while (parsed[8][r] != '\0');
 */
                        constantes[i].constante = octal(parsed[8]);
                        //printf("aux: %o\n", constantes[i].constante);
                        break;
                    case '%':
                       /*  o=0;
                        i = 1;
                        while (parsed[8][i] != '\0')
                        {
                            aux[o++] = parsed[8][i];
                            i++;
                        }
                         aux[o] = '\0'; */
                        constantes[i].constante = hexadecimal(parsed[8]);
                        break;
                    default:
                        constantes[i].constante = decimal(parsed[8]);
                    }
                    i++;
                }
                else
                {
                    char aux[strlen(parsed[7])+1];
                    strcpy(aux,parsed[7]);
                    Mayusculas(aux);
                    strcpy(strings[j].simbolo, aux);

                    //strcpy(strings[j].constante,"");
                    /* i = 0;
                    while(parsed[8][i] != '\0')
                        printf("%c",parsed[8][i++]); */
                    //printf("strlen %d\n",strlen(parsed[8]));

                    for (i = 1; i < strlen(parsed[8]); i++)
                    {
                       strings[j].constante[i-1] = parsed[8][i];
                       //printf("%c\t",strings[j].constante[i]);
                    }
                    strings[j].constante[i-2] = '\0';
                    //printf("cad %c\n",cadena);
                    //strcpy(strings[j].constante,cadena);
                    //strings[j].constante[33] = '\0';
                    //printf("const %s\n",strings[j].constante);
                    j++;
                }
                cantLineas--;
            }
            else if ( parsed[7] != NULL && parsed[8] == NULL)
            {
                printf("error en la definicion del simbolo\n");
                cantLineas--;
            }
        }
        if(parsed[0] != NULL){
            strcpy(rotulos[k].simbolo,parsed[0]);
            Mayusculas(rotulos[k].simbolo);
            rotulos[k].posReferencia = cantLineas;
            k++;
        }
        if (parsed[5] != NULL)
            cantLineas--;
        cantLineas++;
        if(parsed[0] == NULL &&  parsed[1] == NULL && parsed[5] == NULL && parsed[7] == NULL)
            cantLineas--;
    }
    offset = cantLineas;
    for (int i = 0; i < j; i++)
    {
        strings[i].posRelativa = offset;
        offset += strlen(strings[i].constante) + 1;
    }
    contSimbolos = j;
    contRotulos = k;
    return duplicado;
}

int buscaDuplicados(char parsed[], TSimboloC constantes[], int n, TSimboloS strings[], int m)
{
    int i = 0;
    int res = 0;
    while (i < n && strcmp(parsed, constantes[i].simbolo) != 0)
    {
        i++;
    }
    if (i == n)
    {
        i = 0;
        while (i < m && strcmp(parsed, strings[i].simbolo) != 0)
        {
            i++;
        }
        if (i < m)
            res = 1;
    }
    else if (i < n)
        res = 1;
    return res;
}
