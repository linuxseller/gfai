/**
 * main.c
 * Copyright (c) 2024 Alexandr Iurchenko
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stddef.h>
#include <ctype.h>
#include <assert.h>
#include <time.h>

#define EXIT_69(msg) { \
    fprintf(stderr, msg"\n"); \
    exit(69); \
}
#define SVFMT "%.*s"
#define SVVARG(sv) (int)sv.size, sv.data

typedef struct {
    char *data;
    int size;
} Sv;

typedef struct {
    Sv word;
    size_t count;
} WordCountPair;

typedef struct {
    Sv word;
    size_t pairs_count;
    WordCountPair *pairs;
} MarkovEntry;

bool svcmp(Sv a, Sv b)
{
    return strncmp(a.data, b.data, a.size);
}

bool hasNext(char *data)
{
    while(*data++){
        if(isalnum(*data)){
           return true;
        }
    }
    return false;
}

Sv nextSv(char **data)
{
    while(!isalnum(**data)){
        *data+=1;
    }
    Sv sv = {.size=0, .data=*data};
    while(isalnum(**data) && **data!='\0'){
        **data = tolower(**data);
        sv.size++;
        *data+=1;
    }
    return sv;
}

int inPairsIndex(WordCountPair *arr, size_t size, Sv sv)
{
    for (size_t i = 0; i < size; i++) {
        if(svcmp(arr[i].word, sv)==0){
            return i;
        }
    }
    return -1;
}

int inMarkovChainIndex(MarkovEntry *arr, size_t size, Sv sv)
{
    for (size_t i = 0; i < size; i++) {
        if(svcmp(arr[i].word, sv)==0){
            return i;
        }
    }
    return -1;
}

int main(int argc, char **argv)
{
    srand(time(NULL));
    char *program = argv[0];
    (void) program;
    if(argc < 2){
        EXIT_69("No markov src file provided");
    }
    FILE *markov_src_file = fopen(argv[1],"r");
    if(markov_src_file==NULL){
        EXIT_69("Could not provided markov src file");
    }
    fseek(markov_src_file, 0, SEEK_END);
    long fsize = ftell(markov_src_file);
    fseek(markov_src_file, 0, SEEK_SET);
    char *markov_src = malloc(fsize+1);
    fread(markov_src, fsize, 1, markov_src_file);
    size_t source_alloc_size = 10000, source_size=1;
    Sv *source = malloc(sizeof(*source) * source_alloc_size);
    memset(source, 0, sizeof(*source) * source_alloc_size);
    source[0] = nextSv(&markov_src);
    for (size_t i = 1; true; i++) {
        if(i>source_alloc_size){
            source_alloc_size += 50;
            source = realloc(source, source_alloc_size);
        }
        source[i] = nextSv(&markov_src);
        source_size=i;
        if(!hasNext(markov_src)){
            break;
        }
    }
    source_size++;
    printf("Source size %zd\n", source_size);
    // Markov generating should start here
    MarkovEntry *markov_chain = malloc(sizeof(MarkovEntry)*source_size);
    for(size_t i=0; i<source_size-1; i++){
        markov_chain[i].word = source[i];
        markov_chain[i].pairs = malloc(sizeof(WordCountPair)*10); // TODO: fix
        for(size_t j=0; j<source_size; j++){
            if(svcmp(markov_chain[i].word, source[j]) == 0){
                int in_pairs_index = inPairsIndex(markov_chain[i].pairs, markov_chain[i].pairs_count, source[j+1]);
                if(in_pairs_index == -1){
                    markov_chain[i].pairs[markov_chain[i].pairs_count].word = source[j+1];
                    markov_chain[i].pairs_count++;
                    in_pairs_index = markov_chain[i].pairs_count-1;
                }
                markov_chain[i].pairs[in_pairs_index].count++;
            }
        }
    }
    for (size_t i = 0; i < source_size-1; i++) {
        size_t sum = 0;
        for(size_t j=0; j < markov_chain[i].pairs_count; j++){
            sum+=markov_chain[i].pairs[j].count;
        }
        for(size_t j=0; j < markov_chain[i].pairs_count; j++){
            markov_chain[i].pairs[j].count=markov_chain[i].pairs[j].count*100/sum;
        }
    }
    //-Calculated probabilityes-
    Sv startWord = {.data="love", .size=4};
    (void) startWord;
    int cur_word_index = 0;
    printf(SVFMT" ", SVVARG(markov_chain[cur_word_index].word));
    for(int i=0; i<20; i++){
        size_t random = rand()%100;
        MarkovEntry cur_word = markov_chain[cur_word_index];
        size_t left_edge = 0;
        for(size_t j=0; j<cur_word.pairs_count; j++){
            if(left_edge < random && random < left_edge + cur_word.pairs[j].count){
                printf(SVFMT" ", SVVARG(cur_word.pairs[j].word));
                cur_word_index = inMarkovChainIndex(markov_chain, source_size, cur_word.pairs[j].word);
                if(cur_word_index==-1){
                    cur_word_index = rand()%source_size;
                }
                break;
            }
            left_edge+=cur_word.pairs[j].count;
        }
    }
    printf("\n");
goto defer;
defer:
    free(source);
}
