// Example for usage of libriff
//
// We open a potential RIFF file, recursively traverse through all chunks and print the chunk header info with indentation.
//


#include <stdio.h>
#include <string.h>

#include "riff.h"





int nlist = 0;  //number of sub lists (LIST chunks)
int nchunk = 0; //number of chunks




void test_traverse_rec(riff_reader *rr){
	int err;
	char indent[512*8];  //indentation
	
	//identation for pretty output
	strcpy(indent, "");
	for(int i = 0; i < rr->ls_level; i++)
		sprintf(indent, "%s ", indent);
	
	if(rr->ls_level == 0) {
		printf("CHUNK_ID: TOTAL_CHUNK_SIZE [CHUNK_DATA_FROM_TO_POS]\n");
		//output RIFF file header
		printf("%s%s: %d [%d..%d]\n", indent, rr->h_id, rr->h_size, rr->pos_start, rr->pos_start + rr->size);
		printf(" %sType: %s\n", indent, rr->h_type);
	}
	else {
		//output type of parent list chunk
		struct riff_levelStackE *ls = rr->ls + rr->ls_level - 1;
		//type ID of sub list is only read, after stepping into it
		printf(" %sType: %s\n", indent, ls->c_type);
	}
	
	strcat(indent, " ");
	
	int k = 0;
	
	while(1){
		printf("%s%s: %d [%d..%d]\n", indent, rr->c_id, rr->c_size, rr->c_pos_start,  rr->c_pos_start + 8 + rr->c_size + rr->pad - 1);
		
		//if current chunk not a chunk list
		if(strcmp(rr->c_id, "LIST") != 0  &&  strcmp(rr->c_id, "RIFF") != 0){
		}
		else {
			//getchar(); //uncomment to press ENTER to continue after a printed chunk
			{
				err = riff_readerSeekLevelSub(rr);
				if (err){
				}
				else
					nlist++;
				test_traverse_rec(rr); //recursive call
			}
		}
		k++;
		
		err = riff_readerSeekNextChunk(rr);
		if(err >= RIFF_ERROR_CRITICAL) {
			printf("%s", riff_readerErrorToString(err));
			break;
		}
		else if (err < RIFF_ERROR_CRITICAL  &&  err != RIFF_ERROR_NONE) {
			//printf("last chunk in level %d %d .. %d %s\n", rr->ls_level, rr->c_pos_start, rr->c_pos_start + 8 + rr->c_size + rr->pad, rr->c_id);
			
			//go back from sub level
			riff_readerLevelParent(rr);
			//file pos has not changed by going a level back, we are now within that parent's data
			break;
		}
		else
			nchunk++;
	}
}




void test(FILE *f){
	//get size
	fseek(f, 0, SEEK_END);
	int fsize = ftell(f);
	fseek(f, 0, SEEK_SET);
	
	//allocate initialized handle struct
	riff_reader *rr = riff_readerAllocate();
	
	//after allocation rr->fp_fprintf == fprintf
	//you can change the rr->fp_fprintf function pointer here for error output
	//rr->fp_fprint = NULL;  //set to NULL to disable any error printing
	
	//open file, use build in input wrappers for file
	//open RIFF file via file handle -> reads RIFF header and first chunk header
	if(riff_reader_open_file(rr, f, fsize) != RIFF_ERROR_NONE){
		return;
	}
	nchunk++; //header can be seen as chunk
	
	test_traverse_rec(rr);
	printf("\nlist chunks: %d\nchunks: %d\n", nlist, nchunk);
	
	int r;
	
	
	//current list level
	printf("\n");
	printf("Current pos: %d\n", rr->pos);
	printf("Current list level: %d\n", rr->ls_level);
	
	
	//read a byte
	printf("Reading a byte of chunk data ...\n");
	char buf[1];
	r = riff_readInChunk(rr, buf, 1);
	printf("Bytes read: %d of %d\n", r, 1);
	printf("Current pos: %d\n", rr->pos);
	printf("Current list level: %d\n", rr->ls_level);
	
	
	printf("seeking a byte forward in chunk data ...\n");
	r = riff_readerSeekInChunk(rr, rr->c_pos + 1);
	if(r != RIFF_ERROR_NONE)
		printf("Seek failed!\n");
	printf("Current pos: %d\n", rr->pos);
	printf("Offset in current chunk data: %d\n", rr->c_pos);
	printf("Current list level: %d\n", rr->ls_level);
	
	
	//rewind to first chunk's data pos 0
	printf("Rewind to first chunk in file ...\n");
		r = riff_readerRewind(rr);
	if(r != RIFF_ERROR_NONE)
		printf("Error: %s\n", riff_readerErrorToString(r));
	printf("Current pos: %d (expected: %d)\n", rr->pos, rr->pos_start + RIFF_HEADER_SIZE + RIFF_CHUNK_DATA_OFFSET);
	printf("Current list level: %d\n", rr->ls_level);
	
	
	riff_readerFree(rr);
	
	//find and visit all LIST chunks
	
	//load file to mem and do same again
}




int main(int argc, char *argv[] ){
	if(argc < 2){
		printf("Need path to input RIFF file!\n");
		return -1;
	}
	
	printf("Opening file: %s\n", argv[1]);
	FILE *f = fopen(argv[1],"rb");
	
	if(f == NULL) {
		printf("Failed to open file!\n");
		return -1;
	}
	
	test(f);
	
	fclose(f);
	
	return 0;
}
