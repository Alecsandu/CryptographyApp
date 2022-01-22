#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

typedef struct
{
    unsigned char pR,pG,pB;
}pixel;

/*
    Purpose: this function generates random numbers using the xorshift32 generator and stores them in an array
    Informations: 
	- seed is used to initialize the xorshift32 generator, also it is the first number from secret_key.txt
	- elements_array contains the generated values (2*image_width*image_height numbers)
    - the elements are sent back by using the elements_array variable
    - https://en.wikipedia.org/wiki/Xorshift
*/
void xorshift32(unsigned int **elements_array, unsigned int image_width, unsigned int image_height, unsigned int seed)
{
    unsigned int aux = seed;
    *elements_array = malloc(sizeof(unsigned int) * 2 * image_width * image_height);
    for(unsigned int i = 0; i < 2 * image_width * image_height; i++)
    {
        aux = aux^aux << 13;
        aux = aux^aux >> 17;
        aux = aux^aux << 5;
        (*elements_array)[i] = aux;
    }
}

/*
    Purpose: generate random permutation using the numbers generated by xorshift32 function
    Info:
    - it is a more modern variant of the Fisher Yantes Shuffle
    - https://en.wikipedia.org/wiki/Fisher%E2%80%93Yates_shuffle
*/
void durstenfeld_permutation(unsigned int **permutation, unsigned int image_width, unsigned int image_height,
                                unsigned int *xorshift32_generated_array)
{
    *permutation = (unsigned int*)malloc(sizeof(unsigned int) * image_width * image_height);
    for(unsigned int i = 0; i < image_width * image_height; i++)
	{
        (*permutation)[i] = i;
	}

    for(unsigned int i = image_width * image_height - 1; i >= 1; i--)
    {
        unsigned int poz = xorshift32_generated_array[i]%(i+1);
        unsigned int aux = (*permutation)[poz];
        (*permutation)[poz] = (*permutation)[i];
        (*permutation)[i] = aux;
    }
}

/*
    Purpose: generate the inverse of the durstenfeld permutation
    Info:
    - initial_permutation is the one generated using the durstenfeld algorithm
*/
void inverse_permutation(unsigned int **final_permutation, unsigned int *initial_permutation, unsigned int image_width, unsigned int image_height)
{
    *final_permutation = (unsigned int *)malloc(sizeof(unsigned int) * image_width * image_height);
    for(unsigned int i = 0; i < image_width * image_height; i++)
        (*final_permutation)[initial_permutation[i]] = i;
}

/*
    Purpose: read and store the image pixels in an array of width*height size instead of a matrix with same size
    Infos:
    - this works only for .bmp images with header size of 54
*/
void image_linearization(pixel **image_pixels, unsigned int image_width, unsigned int image_height, FILE *image_file_handle)
{
    unsigned char pixel_rgb[3];
    *image_pixels = (pixel *)malloc(sizeof(pixel) * image_width * image_height);
    fseek(image_file_handle, 54, SEEK_SET);
    for(unsigned int i = 0; i < image_width * image_height; i++)
    {
        fread(pixel_rgb, 3, 1, image_file_handle);
        (*image_pixels)[i].pR = pixel_rgb[0];
        (*image_pixels)[i].pG = pixel_rgb[1];
        (*image_pixels)[i].pB = pixel_rgb[2];
    }
}

/*
    Purpose: permutes the pixels stored in the array initialized at image_linearization step
    Infos:
    - the image must be linearized otherwise it will give wrong results
*/
void permute_image_pixels(pixel **linearized_and_permuted_form, pixel **linearized_form, unsigned int *permutations,
                                unsigned int image_width, unsigned int image_height)
{
    *linearized_and_permuted_form = (pixel *)malloc(sizeof(pixel) * image_width * image_height);
    for(unsigned int i = 0; i < image_width * image_height; i++)
        (*linearized_and_permuted_form)[*(permutations+i)] = (*linearized_form)[i];
}

/*
    Purpose: For each pixel a substitution is made in order to encrypt the image
    Infos:
    - the Secret_Values is the second value from secret_key.txt file
    - this will create the encrypted image pixels which will be used to create a new image
    - if any function gave wrong results this final step might create an image which can not be decrypted and might not be even safe to use
        because it might not have a strong encryption
*/
void final_image_encryption(pixel **final_image, pixel **linearized_form, unsigned int *xorshift32_array, unsigned int image_width,
                             unsigned int image_height, unsigned int Secret_Value)
{
    *final_image = (pixel *)malloc(sizeof(pixel) * image_width * image_height);

    int octet_2 = (Secret_Value>>16)&0xff;
    int octet_1 = (Secret_Value>>8)&0xff;
    int octet_0 = Secret_Value&0xff;

    unsigned int i = 0, j = image_width * image_height;
    (*final_image)[i].pR = octet_0^(*linearized_form)[i].pR^(*(xorshift32_array+j));
    (*final_image)[i].pG = octet_1^(*linearized_form)[i].pG^(*(xorshift32_array+j));
    (*final_image)[i].pB = octet_2^(*linearized_form)[i].pB^(*(xorshift32_array+j));
    
    for(i = 1; i < image_width * image_height; i++)
    {
        j++;
        (*final_image)[i].pR = (*final_image)[i-1].pR ^ (*linearized_form)[i].pR ^ (*(xorshift32_array+j));
        (*final_image)[i].pG = (*final_image)[i-1].pG ^ (*linearized_form)[i].pG ^ (*(xorshift32_array+j));
        (*final_image)[i].pB = (*final_image)[i-1].pB ^ (*linearized_form)[i].pB ^ (*(xorshift32_array+j));
    }
}

/*
	Purpose: use the generated data to inverse the substitudion made at encryption
*/
void aplicate_xor(pixel **xored_image, pixel **linearized_from_image_pixels, unsigned int *sir_xorshift32_decr, unsigned int image_width,
                  unsigned image_height, unsigned int Secret_Value)
{
    *xored_image = (pixel *)malloc(sizeof(pixel) * image_width * image_height);

    int b = (Secret_Value>>16)&0xff;      // third octet 16-23
    int g = (Secret_Value>>8)&0xff;       // second octet 8-15
    int r = Secret_Value&0xff;            // first octet 0-7

    unsigned int i = 0, j = image_width * image_height;
    (*xored_image)[i].pR = r ^ (*linearized_from_image_pixels)[i].pR ^ (*(sir_xorshift32_decr+j));
    (*xored_image)[i].pG = g ^ (*linearized_from_image_pixels)[i].pG ^ (*(sir_xorshift32_decr+j));
    (*xored_image)[i].pB = b ^ (*linearized_from_image_pixels)[i].pB ^ (*(sir_xorshift32_decr+j));

    for(i = 1; i < image_width * image_height; i++)
    {
        j++;
        (*xored_image)[i].pR = (*linearized_from_image_pixels)[i-1].pR ^ (*linearized_from_image_pixels)[i].pR ^ (*(sir_xorshift32_decr+j));
        (*xored_image)[i].pG = (*linearized_from_image_pixels)[i-1].pG ^ (*linearized_from_image_pixels)[i].pG ^ (*(sir_xorshift32_decr+j));
        (*xored_image)[i].pB = (*linearized_from_image_pixels)[i-1].pB ^ (*linearized_from_image_pixels)[i].pB ^ (*(sir_xorshift32_decr+j));
    }
}

/*
    Purpose: here are all the used steps in order to encrypt the image
*/
void criptare(char *input_image_name, char *output_image_name, char *secret_key_file_name)
{
    FILE *secret_key_file_handle, *input_image_file_handle, *output_image_file_handle;
    secret_key_file_handle = fopen(secret_key_file_name, "r");
    if(secret_key_file_handle == NULL)
    {
        printf("Could not open/find the file with the secret key!\n");
        return;
    }
    unsigned int Generator_Seed = 0,Starting_Value = 0;
    fscanf(secret_key_file_handle, "%d", &Generator_Seed);
    fscanf(secret_key_file_handle, "%d", &Starting_Value);
    fclose(secret_key_file_handle);

    input_image_file_handle = fopen(input_image_name, "rb+");
    if(input_image_file_handle == NULL)
    {
        printf("The image which you wanted to encrypt could not be found!\n");
        return;
    }

    unsigned int image_size, image_width, image_height;
    fseek(input_image_file_handle, 2, SEEK_SET);
    fread(&image_size, sizeof(unsigned int), 1, input_image_file_handle);

    fseek(input_image_file_handle, 18, SEEK_SET);
    fread(&image_width, sizeof(unsigned int), 1, input_image_file_handle);
    fread(&image_height, sizeof(unsigned int), 1, input_image_file_handle);

    output_image_file_handle = fopen(output_image_name, "wb+");
    int j = 0;
    unsigned c;
    fseek(input_image_file_handle, 0, SEEK_SET);
    while(fread(&c, 1, 1, input_image_file_handle)==1 && j<54)
    {
        j++;
        fwrite(&c, 1, 1, output_image_file_handle);
    }

    unsigned int *xorshift32_generated_values_array;
    xorshift32(&xorshift32_generated_values_array, image_width, image_height, Generator_Seed);

    unsigned int *permutation;
    durstenfeld_permutation(&permutation, image_width, image_height, xorshift32_generated_values_array);

    pixel *liearized_form;
    image_linearization(&liearized_form, image_width, image_height, input_image_file_handle);

    pixel *linearized_and_permuted_form;
    permute_image_pixels(&linearized_and_permuted_form, &liearized_form, permutation, image_width, image_height);
	free(permutation);
	free(liearized_form);

    pixel *ciphered_image;
    final_image_encryption(&ciphered_image, &linearized_and_permuted_form, xorshift32_generated_values_array, image_width, image_height, Starting_Value);
	free(xorshift32_generated_values_array);
	free(linearized_and_permuted_form);

    int padding = 0;
    if(image_width % 4 != 0)
    {
        padding = 4 - (3 * image_width) % 4;
    }

    for(unsigned int i = 0; i < image_height; i++)
    {
        for(unsigned int j = 0; j < image_width; j++)
        {
            fwrite(ciphered_image + (image_height*i + j), 3, 1, output_image_file_handle);
        }
        fseek(output_image_file_handle, padding, SEEK_CUR);
    }
    printf("The image was succesfully encrypted.\n");
	free(ciphered_image);

    fclose(input_image_file_handle);
    fclose(output_image_file_handle);
}

/*
    Purpose: uses the functions needed to decrypt an image
*/
void decrypt(char *encrypted_image_name, char *decrypted_image_name, char *secret_key_file_name)
{
    FILE *input_image_file_handle, *output_image_file_handle, *secret_key_file_handle;
    input_image_file_handle = fopen(encrypted_image_name, "rb+");
    output_image_file_handle = fopen(decrypted_image_name, "wb+");
    secret_key_file_handle = fopen(secret_key_file_name, "r");

    if(input_image_file_handle == NULL)
    {
        printf("The given image was not found!\n");
        return;
    }

    if(secret_key_file_handle == NULL)
    {
        printf("Error at file open!\nCheck if you correctly spelled its name, or if it is present in the same directory as the executable file.");
        return;
    }

    unsigned int Generator_Seed = 0, Starting_Value = 0;
    fscanf(secret_key_file_handle,"%d",&Generator_Seed);
    fscanf(secret_key_file_handle,"%d",&Starting_Value);
    fclose(secret_key_file_handle);

    unsigned int image_size = 0, image_width = 0, image_height = 0;
    fseek(input_image_file_handle, 2, SEEK_SET);
    fread(&image_size, sizeof(unsigned int), 1, input_image_file_handle);
    fseek(input_image_file_handle, 18, SEEK_SET);
    fread(&image_width,sizeof(unsigned int), 1, input_image_file_handle);
    fread(&image_height,sizeof(unsigned int), 1, input_image_file_handle);

    //	COPY THE ENCRYPTED IMAGE HEADER
    int j = 0;
    unsigned c;
    fseek(input_image_file_handle,0,SEEK_SET);
    while(fread(&c, 1, 1, input_image_file_handle) == 1 && j < 54)
    {
        j++;
        fwrite(&c, 1, 1, output_image_file_handle);
    }

    unsigned int *xorshift32_generated_values_array;
    xorshift32(&xorshift32_generated_values_array, image_width, image_height, Generator_Seed);

    unsigned int *durstenfeld_permutation_elements;
    durstenfeld_permutation(&durstenfeld_permutation_elements, image_width, image_height, xorshift32_generated_values_array);

    unsigned int *final_permutation;
    inverse_permutation(&final_permutation, durstenfeld_permutation_elements, image_width, image_height);
	free(durstenfeld_permutation_elements);

    pixel *linearized_encrypted_image_pixels;
    image_linearization(&linearized_encrypted_image_pixels, image_width, image_height, input_image_file_handle);

    pixel *decoded_image;
    aplicate_xor(&decoded_image, &linearized_encrypted_image_pixels, xorshift32_generated_values_array, image_width, image_height, Starting_Value);
	free(xorshift32_generated_values_array);
	free(linearized_encrypted_image_pixels);

    pixel *permuted_decoded_image;
    permute_image_pixels(&permuted_decoded_image, &decoded_image, final_permutation, image_width, image_height);
	free(final_permutation);
	free(decoded_image);

    int padding = 0;
    if(image_width % 4 != 0)
	{
        padding = 4 - (3 * image_width) % 4;
    }
	
    for(unsigned int i = 0; i < image_height; i++)
    {
        for(unsigned int j = 0; j < image_width; j++)
        {
            fwrite(permuted_decoded_image + (image_width*i + j), 3, 1, output_image_file_handle);
        }
        fseek(output_image_file_handle, padding, SEEK_CUR);
    }
    printf("The image was succesfully decrypted.\n");
	free(permuted_decoded_image);

    fclose(input_image_file_handle);
    fclose(output_image_file_handle);
}

void chi_patrat(char *image_name)
{
	unsigned int image_size, image_width, image_height, f_bar, i = 0, j = 0, *pixel1, *pixel2, *pixel3;
    double nr1 = 0, nr2 = 0, nr3 = 0;
    FILE *file_handle;
    file_handle = fopen(image_name,"rb+");
    fseek(file_handle, 2, SEEK_SET);
    fread(&image_size, sizeof(unsigned int), 1, file_handle);
    fseek(file_handle, 18, SEEK_SET);
    fread(&image_width, sizeof(unsigned int), 1, file_handle);
    fread(&image_height, sizeof(unsigned int), 1, file_handle);

    pixel1 = (unsigned int *)malloc(sizeof(unsigned int)*256);
    pixel2 = (unsigned int *)malloc(sizeof(unsigned int)*256);
    pixel3 = (unsigned int *)malloc(sizeof(unsigned int)*256);

    for(i = 0; i < 256; i++)
    {
        *(pixel1+i) = 0;
        *(pixel2+i) = 0;
        *(pixel3+i) = 0;
    }
	
    fseek(file_handle, 0, SEEK_SET);
    pixel *imagine_liniarizata;
    image_linearization(&imagine_liniarizata, image_width, image_height, file_handle);
    f_bar = (image_height*image_width) / 256;

    for(j = 0; j < 256; j++)
    {
        for(i = 0; i < image_height * image_width; i++)
        {
            if(((*(imagine_liniarizata + i)).pR) == j)
                (*(pixel1+j)) = (*(pixel1+j)) + 1;
            if(((*(imagine_liniarizata + i)).pG) == j)
                (*(pixel2+j)) = (*(pixel2+j)) + 1;
            if(((*(imagine_liniarizata + i)).pB) == j)
                (*(pixel3+j)) = (*(pixel3+j)) + 1;
        }
    }

    for(i = 0; i < 256; i++)
    {
        nr1 += (double)(((*(pixel1+i))-f_bar) * ((*(pixel1+i))-f_bar)) / f_bar;
        nr2 += (double)(((*(pixel2+i))-f_bar) * ((*(pixel2+i))-f_bar)) / f_bar;
        nr3 += (double)(((*(pixel3+i))-f_bar) * ((*(pixel3+i))-f_bar)) / f_bar;
    }

    printf("Chi_patrat results:\n%.2f %.2f %.2f\n", nr3, nr2, nr1);

    fclose(file_handle);
}

int main()
{
    char input_image_name[101];
    char encrypted_image_name[101];
    char secret_key_file_name[101];

    printf("Name of the image that you want to encrypt(perhaps peppers.bmp): ");
    fgets(input_image_name, 101, stdin);
    input_image_name[strlen(input_image_name)-1] = '\0';

    printf("\nName of the new encrypted image(perhaps encodedpeppers.bmp): ");
    fgets(encrypted_image_name, 101, stdin);
    encrypted_image_name[strlen(encrypted_image_name)-1] = '\0';

    printf("\nName of the file which contains the secret key(perhaps secret_key.txt): ");
    fgets(secret_key_file_name, 101, stdin);
    secret_key_file_name[strlen(secret_key_file_name)-1] = '\0';

    criptare(input_image_name, encrypted_image_name, secret_key_file_name);

    printf("The name of the decrypted image is 'decodedpeppers.bmp'\n");
    char nume_imagine_decodificata[101] = "decodedpeppers.bmp";
    decrypt(encrypted_image_name, nume_imagine_decodificata, secret_key_file_name);

    chi_patrat(input_image_name);
    chi_patrat(encrypted_image_name);
    printf("\n");
	getchar();
	
    return 0;
}
