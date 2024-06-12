// Example for usage of libriff
//
// We open a potential RIFF file, recursively traverse through all chunks and print the chunk header info with indentation.
//


#include <cstdio>
#include <cstring>

#include <ios>
#include <iostream>

#include "riff.hpp"





int nlist = 0;  //number of sub lists (LIST chunks)
int nchunk = 0; //number of chunks




void test_traverse_rec(RIFF::RIFFFile & rh){
	int err;
	std::string indent = "";  //indentation
	
	//identation for pretty output
	for(int i = 0; i < rh().ls_level; i++)
		indent += " ";
	
	if(rh().ls_level == 0) {
		std::cout << "CHUNK_ID: TOTAL_CHUNK_SIZE [CHUNK_DATA_FROM_TO_POS]" << std::endl;
		//output RIFF file header
		std::cout << indent << rh().h_id << ": " << rh().h_size << " [" << rh().pos_start << ".." << rh().pos_start + rh().size << "]" << std::endl;
		std::cout << indent << "Type: " << rh().h_type << std::endl;
	}
	else {
		//output type of parent list chunk
		struct riff_levelStackE *ls = rh().ls + rh().ls_level - 1;
		//type ID of sub list is only read, after stepping into it
		std::cout << indent << "Type: " << ls->c_type << std::endl;
	}
	
	indent += " ";
	
	int k = 0;
	
	while(1){
		std::cout << indent << rh().c_id << ": " <<  rh().c_size << " [" << rh().c_pos_start << ".." << rh().c_pos_start + 8 + rh().c_size + rh().pad - 1 << "]" << std::endl;
		
		//if current chunk not a chunk list
		if(strcmp(rh().c_id, "LIST") != 0  &&  strcmp(rh().c_id, "RIFF") != 0){
		}
		else {
			//getchar(); //uncomment to press ENTER to continue after a printed chunk
			{
				err = rh.seekLevelSub();
				if (err){
				}
				else
					nlist++;
				test_traverse_rec(rh); //recursive call
			}
		}
		k++;
		
		err = rh.seekNextChunk();
		if(err >= RIFF_ERROR_CRITICAL) {
			std::cout << riff_errorToString(err) << std::endl;
			break;
		}
		else if (err < RIFF_ERROR_CRITICAL  &&  err != RIFF_ERROR_NONE) {
			//printf("last chunk in level %d %d .. %d %s\n", rh().ls_level, rh().c_pos_start, rh().c_pos_start + 8 + rh().c_size + rh().pad, rh().c_id);
			
			//go back from sub level
			rh.levelParent();
			//file pos has not changed by going a level back, we are now within that parent's data
			break;
		}
		else
			nchunk++;
	}
}




void test(std::fstream & f){
	//get size
	f.seekg(0, std::ios_base::end);
	int fsize = f.tellg();
	f.seekg(0, std::ios_base::beg);
	
	//allocate initialized handle struct
	auto rh = RIFF::RIFFFile();
	
	//after allocation rh->fp_fprintf == fprintf
	//you can change the rh->fp_fprintf function pointer here for error output
	//rh->fp_fprint = NULL;  //set to NULL to disable any error printing
	
	//open file, use build in input wrappers for file
	//open RIFF file via file handle -> reads RIFF header and first chunk header
	if(rh.open(f, fsize) != RIFF_ERROR_NONE){
		return;
	}
	nchunk++; //header can be seen as chunk
	
	test_traverse_rec(rh);
	std::cout << std::endl << "list chunks: " << nlist << ", chunks:" << nchunk << std::endl << std::endl ;
	
	int r;
	
	
	//current list level
	std::cout << "Current pos: " << rh().pos << std::endl;
	std::cout << "Current list level: " << rh().ls_level << std::endl;
	
	
	//read a byte
	std::cout << "Reading a byte of chunk data ..." << std::endl;
	char buf[1];
	r = rh.readInChunk(buf, 1);
	std::cout << "Bytes read: " << r << " of 1" << std::endl;
	std::cout << "Current pos: " << rh().pos << std::endl;
	std::cout << "Current list level: " << rh().ls_level << std::endl;
	
	
	std::cout << "seeking a byte forward in chunk data ..." << std::endl;
	r = rh.seekInChunk(rh().c_pos + 1);
	if(r != RIFF_ERROR_NONE)
		std::cout << "Seek failed!" << std::endl;
	std::cout << "Current pos: " << rh().pos << std::endl;
	std::cout << "Offset in current chunk data: " << rh().c_pos << std::endl;
	std::cout << "Current list level: " << rh().ls_level << std::endl;
	
	
	//rewind to first chunk's data pos 0
	std::cout << "Rewind to first chunk in file ..." << std::endl;
		r = rh.rewind();
	if(r != RIFF_ERROR_NONE)
		std::cout << "Error: " << riff_errorToString(r) << std::endl;
	std::cout << "Current pos: " << rh().pos << " (expected: " << (rh().pos_start + RIFF_HEADER_SIZE + RIFF_CHUNK_DATA_OFFSET) << ")" << std::endl;
	std::cout << "Current list level: " << rh().ls_level << std::endl;
	
	
	// delete rh;
	
	//find and visit all LIST chunks
	
	//load file to mem and do same again
}




int main(int argc, char *argv[] ){
	if(argc < 2){
		std::cout << "Need path to input RIFF file!" << std::endl;
		return -1;
	}
	
	std::cout << "Opening file: " << argv[1] << std::endl;
	std::fstream f = std::fstream(argv[1], std::ios_base::in|std::ios_base::binary);
	
	if(!f.is_open()) {
		std::cout << "Failed to open file!" << std::endl;
		return -1;
	}
	
	test(f);
	
	f.close();
	
	return 0;
}
