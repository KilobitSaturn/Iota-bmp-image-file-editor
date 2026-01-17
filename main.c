#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <locale.h>
#include <ctype.h>

#define BLUR 64

typedef enum {false = 0, true = 1} bool;

struct bmp_header
{
    struct bmp_header_core
    {
        unsigned char chunkID[2];
        uint32_t chunksize, reserved, subchunksize;
    } core;
    struct bmp_header_info
    {
        uint32_t subchunk2size;
        int32_t width, height;
        uint16_t planes, bitsperpixel;
        uint32_t compression, imgsize;
        int32_t XperM, YperM;
        uint32_t usedcolors, importantcolors;
    } info;
};

struct fbmp
{
    FILE *fp;
    char filename[FILENAME_MAX];
    struct bmp_header header;
    int padding;
};

struct pixel
{
    unsigned char b, g, r;
};

struct chroma
{
    FILE *fp;
    struct pixel px;
};

void fbmp_aux_log(const struct fbmp bmp)
{
    FILE *log = fopen("bmplog.txt", "wt");
    fprintf(log, "Filename: \"%s\"\n", bmp.filename);
    fprintf(log, "Core: '%.2s'\n", bmp.header.core.chunkID);
    fprintf(log, "\tchunksize: %d bytes\n\treserved: %d\n\tsubchunksize: %d bytes\n", bmp.header.core.chunksize, bmp.header.core.reserved, bmp.header.core.subchunksize);
    fprintf(log, "Info:\n\tsubchunk2size: %d bytes\n\twidth: %d px\theight: %d px\n", bmp.header.info.subchunk2size, bmp.header.info.width, bmp.header.info.height);
    fprintf(log, "\tplanes: %d\n\tbitsperpixel: %d bit/px\n", bmp.header.info.planes, bmp.header.info.bitsperpixel);
    fprintf(log, "\tcompression: |%d|\n\timgsize: %d bytes\n", bmp.header.info.compression, bmp.header.info.imgsize);
    fprintf(log, "\tXperM: %d px/m\tYperM: %d px/m\n", bmp.header.info.XperM, bmp.header.info.YperM);
    fprintf(log, "\tusedcolors: %d\n\timportantcolors: %d\n", bmp.header.info.usedcolors, bmp.header.info.importantcolors);
    fprintf(log, "Padding: %d bytes\n", bmp.padding);
    fclose(log);
}

int fbmp_aux_header(FILE *fp, struct bmp_header *hdr, bool write)
{
    rewind(fp);
    switch(write)
    {
    case false:
        for(int i = 0; i < 2; i++) hdr->core.chunkID[i] = getc(fp);
        hdr->core.chunksize = getw(fp);
        hdr->core.reserved = getw(fp);
        hdr->core.subchunksize = getw(fp);
        fread(&hdr->info, sizeof (struct bmp_header_info), 1, fp);
        return write;
        break;
    case true:
        for(int i = 0; i < 2; i++) putc(hdr->core.chunkID[i], fp);
        putw(hdr->core.chunksize, fp);
        putw(hdr->core.reserved, fp);
        putw(hdr->core.subchunksize, fp);
        fwrite(&hdr->info, sizeof (struct bmp_header_info), 1, fp);
        return write;
        break;
    default:
        return -1;
    }
}

int fbmp_aux_getpadding(const struct bmp_header hdr)
{
    if(4 - (hdr.info.bitsperpixel/8*hdr.info.width)%4 == 4) return 0;
    else return 4 - (hdr.info.bitsperpixel/8*hdr.info.width)%4;
}

int fbmp_parse(struct fbmp *bmp)
{
    gets(bmp->filename);
    strcat(bmp->filename, ".bmp");
    bmp->fp = fopen(bmp->filename, "rb");
    if(bmp->fp == NULL)
    {
        fclose(bmp->fp);
        return 0;
    }
    else
    {
        fbmp_aux_header(bmp->fp, &bmp->header, false);
        bmp->padding = fbmp_aux_getpadding(bmp->header);
        fbmp_aux_log(*bmp);
        fclose(bmp->fp);
        return 1;
    }
}

struct pixel fbmp_aux_px(const char c, struct pixel px)
{
    unsigned char w = 0.3*px.r +  0.587*px.g + 0.114*px.b;
    switch(c)
    {
    case '-':
        px.b = 255 - px.b;
        px.g = 255 - px.g;
        px.r = 255 - px.r;
        break;
    case 'b':
    case 'B':
        //px.b = 0;
        px.g = 0;
        px.r = 0;
        break;
    case 'g':
    case 'G':
        px.b = 0;
        //px.g = 0;
        px.r = 0;
        break;
    case 'c':
    case 'C':
        //px.b = 0;
        //px.g = 0;
        px.r = 0;
        break;
    case 'r':
    case 'R':
        px.b = 0;
        px.g = 0;
        //px.r = 0;
        break;
    case 'm':
    case 'M':
        //px.b = 0;
        px.g = 0;
        //px.r = 0;
        break;
    case 'y':
    case 'Y':
        px.b = 0;
        //px.g = 0;
        //px.r = 0;
        break;
    case 'w':
    case 'W':
        px.b = w;
        px.g = w;
        px.r = w;
        break;
    }
    return px;
}

void fbmp_aux_filename(char str[FILENAME_MAX], const char *filename)
{
    strncpy(str, filename, strlen(filename) - 4);
    str[strlen(filename) - 4] = '\0';
}

void bmp_rgbw(struct fbmp *bmp)
{
    bmp->fp = fopen(bmp->filename, "rb");
    char str[FILENAME_MAX], pfname[FILENAME_MAX];
    fbmp_aux_filename(str, bmp->filename);
    struct chroma org, blue, green, red, white;
    fseek(bmp->fp, bmp->header.core.subchunksize, SEEK_SET);
    org.fp = bmp->fp;
    const int o = 0x0000;

    sprintf(pfname, "%s (B).bmp", str);
    //printf("\"%s\"\n", pfname);
    blue.fp = fopen(pfname, "wb");
    fbmp_aux_header(blue.fp, &bmp->header, true);

    sprintf(pfname, "%s (G).bmp", str);
    //printf("\"%s\"\n", pfname);
    green.fp = fopen(pfname, "wb");
    fbmp_aux_header(green.fp, &bmp->header, true);

    sprintf(pfname, "%s (R).bmp", str);
    //printf("\"%s\"\n", pfname);
    red.fp = fopen(pfname, "wb");
    fbmp_aux_header(red.fp, &bmp->header, true);

    sprintf(pfname, "%s (W).bmp", str);
    //printf("\"%s\"\n", pfname);
    white.fp = fopen(pfname, "wb");
    fbmp_aux_header(white.fp, &bmp->header, true);

    for(int i = 0; i < bmp->header.info.height; i++)
    {
        for(int j = 0; j < bmp->header.info.width; j++)
        {
            fread(&org.px, bmp->header.info.bitsperpixel/8, 1, org.fp);
            blue.px = fbmp_aux_px('b', org.px);
            green.px = fbmp_aux_px('g', org.px);
            red.px = fbmp_aux_px('r', org.px);
            white.px = fbmp_aux_px('w', org.px);
            fwrite(&blue.px, bmp->header.info.bitsperpixel/8, 1, blue.fp);
            fwrite(&green.px, bmp->header.info.bitsperpixel/8, 1, green.fp);
            fwrite(&red.px, bmp->header.info.bitsperpixel/8, 1, red.fp);
            fwrite(&white.px, bmp->header.info.bitsperpixel/8, 1, white.fp);
        }
        fseek(org.fp, bmp->padding, SEEK_CUR);
        fwrite(&o, bmp->padding, 1, blue.fp);
        fwrite(&o, bmp->padding, 1, green.fp);
        fwrite(&o, bmp->padding, 1, red.fp);
        fwrite(&o, bmp->padding, 1, white.fp);
    }
    fclose(org.fp), fclose(blue.fp), fclose(green.fp), fclose(red.fp), fclose(white.fp);
    fclose(bmp->fp);
}

void bmp_cmy(struct fbmp *bmp)
{
    bmp->fp = fopen(bmp->filename, "rb");
    char str[FILENAME_MAX], pfname[FILENAME_MAX];
    fbmp_aux_filename(str, bmp->filename);
    struct chroma org, cyan, magenta, yellow;
    fseek(bmp->fp, bmp->header.core.subchunksize, SEEK_SET);
    org.fp = bmp->fp;
    const int o = 0x0000;

    sprintf(pfname, "%s (C).bmp", str);
    //printf("\"%s\"\n", pfname);
    cyan.fp = fopen(pfname, "wb");
    fbmp_aux_header(cyan.fp, &bmp->header, true);

    sprintf(pfname, "%s (M).bmp", str);
    //printf("\"%s\"\n", pfname);
    magenta.fp = fopen(pfname, "wb");
    fbmp_aux_header(magenta.fp, &bmp->header, true);

    sprintf(pfname, "%s (Y).bmp", str);
    //printf("\"%s\"\n", pfname);
    yellow.fp = fopen(pfname, "wb");
    fbmp_aux_header(yellow.fp, &bmp->header, true);

    for(int i = 0; i < bmp->header.info.height; i++)
    {
        for(int j = 0; j < bmp->header.info.width; j++)
        {
            fread(&org.px, bmp->header.info.bitsperpixel/8, 1, org.fp);
            cyan.px = fbmp_aux_px('c', org.px);
            magenta.px = fbmp_aux_px('m', org.px);
            yellow.px = fbmp_aux_px('y', org.px);
            fwrite(&cyan.px, bmp->header.info.bitsperpixel/8, 1, cyan.fp);
            fwrite(&magenta.px, bmp->header.info.bitsperpixel/8, 1, magenta.fp);
            fwrite(&yellow.px, bmp->header.info.bitsperpixel/8, 1, yellow.fp);
        }
        fseek(org.fp, bmp->padding, SEEK_CUR);
        fwrite(&o, bmp->padding, 1, cyan.fp);
        fwrite(&o, bmp->padding, 1, magenta.fp);
        fwrite(&o, bmp->padding, 1, yellow.fp);
    }
    fclose(org.fp), fclose(cyan.fp), fclose(magenta.fp), fclose(yellow.fp);
    fclose(bmp->fp);
}

void bmp_inv(struct fbmp *bmp)
{
    bmp->fp = fopen(bmp->filename, "rb");
    char str[FILENAME_MAX];
    fbmp_aux_filename(str, bmp->filename);
    struct chroma org, inv;
    fseek(bmp->fp, bmp->header.core.subchunksize, SEEK_SET);
    org.fp = bmp->fp;
    const int o = 0x0000;

    strcat(str, " (^-1).bmp");
    //printf("\"%s\"\n", str);
    inv.fp = fopen(str, "wb");
    fbmp_aux_header(inv.fp, &bmp->header, true);

    for(int i = 0; i < bmp->header.info.height; i++)
    {
        for(int j = 0; j < bmp->header.info.width; j++)
        {
            fread(&org.px, bmp->header.info.bitsperpixel/8, 1, org.fp);
            inv.px = fbmp_aux_px('-', org.px);
            fwrite(&inv.px, bmp->header.info.bitsperpixel/8, 1, inv.fp);
        }
        fseek(org.fp, bmp->padding, SEEK_CUR);
        fwrite(&o, bmp->padding, 1, inv.fp);
    }
    fclose(org.fp), fclose(inv.fp);
    fclose(bmp->fp);
}

int bmp_crop(const int lo, const int l, const int ho, const int h, struct fbmp *bmp)
{
    if(lo < 0 || l < 0 || ho < 0 || h < 0) return 0;
    else if(lo >= bmp->header.info.width || l > bmp->header.info.width || ho >= bmp->header.info.height || h > bmp->header.info.height) return -1;
    else if(lo >= l || ho >= h) return -2;
    else
    {
        bmp->fp = fopen(bmp->filename, "rb");
        char str[FILENAME_MAX];
        fbmp_aux_filename(str, bmp->filename);
        struct chroma org;
        fseek(bmp->fp, bmp->header.core.subchunksize, SEEK_SET);
        org.fp = bmp->fp;
        const int o = 0x0000;

        struct fbmp ext;
        sprintf(ext.filename, "%s l[%d, %d] h[%d, %d].bmp", str, lo, l, ho, h);
        //printf("\"%s\"\n", pfname);
        ext.fp = fopen(ext.filename, "wb");
        ext.header = bmp->header;
        ext.header.info.width = l - lo;
        ext.padding = fbmp_aux_getpadding(ext.header);
        ext.header.info.height = h- ho;
        ext.header.info.imgsize = ext.header.info.width*ext.header.info.height*ext.header.info.bitsperpixel/8;
        ext.header.core.chunksize = ext.header.core.subchunksize + ext.header.info.imgsize;
        fbmp_aux_header(ext.fp, &ext.header, true);

        fseek(org.fp, lo*bmp->header.info.bitsperpixel/8 + ho*(bmp->header.info.width*bmp->header.info.bitsperpixel/8 + bmp->padding), SEEK_CUR);
        for(int i = 0; i < h - ho; i++)
        {
            for(int j = 0; j < l - lo; j++)
            {
                fread(&org.px, bmp->header.info.bitsperpixel/8, 1, org.fp);
                fwrite(&org.px, ext.header.info.bitsperpixel/8, 1, ext.fp);
            }
            fwrite(&o, ext.padding, 1, ext.fp);
            fseek(org.fp, (bmp->header.info.width - (l - lo))*bmp->header.info.bitsperpixel/8 + bmp->padding, SEEK_CUR);
        }
        fclose(org.fp), fclose(ext.fp);
        fclose(bmp->fp);
        return 1;
    }
}

struct pixel bmp_aux_pxma(struct pixel *px, const int n)
{
    struct pixel s;
    int blue = 0, green = 0, red = 0;
    for(int a = 0; a < n; a++)
    {
        blue+=px->b;
        green+=px->g;
        red+=px->r;
        px++;
    }
    blue/=n, green/=n, red/=n;
    s.b = (unsigned char) blue, s.g = (unsigned char) green, s.r = (unsigned char) red;
    return s;
}

void bmp_hblur(struct fbmp *bmp)
{
    bmp->fp = fopen(bmp->filename, "rb");
    char str[FILENAME_MAX];
    fbmp_aux_filename(str, bmp->filename);
    struct chroma ext;
    struct pixel temp[BLUR];
    int aux;
    fseek(bmp->fp, bmp->header.core.subchunksize, SEEK_SET);
    const int o = 0x0000;

    strcat(str, " (@).bmp");
    //printf("\"%s\"\n", str);
    ext.fp = fopen(str, "wb");
    fbmp_aux_header(ext.fp, &bmp->header, true);

    for(int i = 0; i < bmp->header.info.height; i++)
    {
        for(int j = 0; j < bmp->header.info.width; j++)
        {
            if(j + BLUR <= bmp->header.info.width)
            {
                fread(&temp, bmp->header.info.bitsperpixel/8, BLUR, bmp->fp);
                fseek(bmp->fp, -(BLUR - 1)*bmp->header.info.bitsperpixel/8, SEEK_CUR);

                ext.px = bmp_aux_pxma(&temp, BLUR);

                fwrite(&ext.px, bmp->header.info.bitsperpixel/8, 1, ext.fp);
            }
            else
            {
                fread(&temp, bmp->header.info.bitsperpixel/8, bmp->header.info.width - j, bmp->fp);
                fseek(bmp->fp, -(bmp->header.info.width - j - 1)*bmp->header.info.bitsperpixel/8, SEEK_CUR);

                ext.px = bmp_aux_pxma(&temp, bmp->header.info.width - j);

                fwrite(&ext.px, bmp->header.info.bitsperpixel/8, 1, ext.fp);
            }
        }
        fseek(bmp->fp, bmp->padding, SEEK_CUR);
        fwrite(&o, bmp->padding, 1, ext.fp);
    }
    fclose(ext.fp);
    fclose(bmp->fp);
}

int main()
{
    /** Tarefas
    0. [x] aux
    1. [x] rgbw
    2. [x] cmy
    3. [x] inv
    4. [x] crop
    5. [x] blur? (bmp_hblur); //m�dia aritm�tica de #BLUR pixels;
    **/

    struct fbmp img;
    setlocale(LC_ALL, "Portuguese");

    char e = 0, a;
    int lo, l, ho, h;

    do
    {
        printf("Olá, mundo! - Iota\n");
        printf("Eu sou o seu gerenciador de imagens! Digite uma opção abaixo para alterar a imagem original em um novo arquivo.\n");
        printf("\t1. Separar canais em vermelho, verde, azul e cinza.\n");
        printf("\t2. Recortar.\n");
        printf("\t3. Gerar combinações ciano, magenta e amarela.\n");
        printf("\t4. Inverter os canais de cor.\n");
        printf("\t5. Desfocar horizontalmente.\n");
        printf("Digite '0' para sair.\n");
        printf("Digite sua escolha: ");
        e = getch();
        printf("'%c'\n", e);
        fflush(stdin);
        if(e >= '1' && e <= '5')
        {
            printf("Digite o título do arquivo:\n");
            if(fbmp_parse(&img) == 0)
            {
                printf("Nenhum arquivo encontrado com esse t�tulo.\n");
            }
            else
            {
                switch(e)
                {
                case '1':
                    printf("\tGerando arquivos...\n");
                    bmp_rgbw(&img);
                    printf("Versões do arquivo '%s' geradas com sucesso!\n", img.filename);
                    break;
                case '2':
                    printf("\tComprimento (l): %d px\tAltura (h): %d px\n", img.header.info.width, img.header.info.height);
                    printf("\tDigite o ponto inicial (lo, ho): ");
                    scanf("%d, %d", &lo, &ho);
                    fflush(stdin);
                    printf("\tDigite o ponto final (l, h): ");
                    scanf("%d, %d", &l, &h);
                    a = bmp_crop(lo, l, ho, h, &img);
                    printf("Gerando arquivo...\n");
                    while(a <= 0)
                    {
                        switch(a)
                        {
                        case 0:
                            printf("Não digite nenhum valor negativo");
                            break;
                        case -1:
                            printf("Um ponto não pode ultrapassar os limites da imagem");
                            break;
                        case -2:
                            printf("Os valores iniciais não podem ser maiores do que os valores finais");
                            break;
                        }
                        printf("; digite novamente:\n");
                        printf("\tComprimento (l): %d px\tAltura (h): %d px\n", img.header.info.width, img.header.info.height);
                        printf("\tDigite o ponto inicial (lo, ho): ");
                        scanf("%d, %d", &lo, &ho);
                        fflush(stdin);
                        printf("\tDigite o ponto final (l, h): ");
                        scanf("%d, %d", &l, &h);
                        printf("Gerando arquivo...\n");
                        a = bmp_crop(lo, l, ho, h, &img);
                    }
                    printf("Recorte de '%s' gerado com sucesso!\n", img.filename);
                    break;
                case '3':
                    printf("\tGerando arquivos...\n");
                    bmp_cmy(&img);
                    printf("Versões do arquivo '%s' geradas com sucesso!\n", img.filename);
                    break;
                case '4':
                    printf("\tGerando arquivo...\n");
                    bmp_inv(&img);
                    printf("Versão negativa do arquivo '%s' gerada com sucesso!\n", img.filename);
                    break;
                case '5':
                    printf("\tGerando arquivo...\n");
                    bmp_hblur(&img);
                    printf("Versão desfocada do arquivo '%s' gerada com sucesso!\n", img.filename);
                    break;
                }
            }
            getch();
            system("cls");
        }
        else if(e == '0')
        {
            printf("\nAdeus, mundo! - Iota\n");
            return 0;
        }
        else {
            printf("Função não encontrada.\n");
            getch();
            system("cls");
            fflush(stdin);
        }
    }
    while (e != '0');
    return 1;
}
