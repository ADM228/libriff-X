// take care: whenever we call rh->fp_read() or rh->fp_seek()
//   we must adjust rh->c_pos and rh->pos
//   => to simplify user wrappers we update the positions outside


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <stdarg.h> //function with variable number of arguments

#include "riff.h"


#define RIFF_LEVEL_ALLOC 16  //number of stack elements allocated per step lock more when needing to enlarge (step)

#define checkValidRiffHandleWithRetVal(rh, retval) if (rh == NULL) return retval
#define checkValidRiffHandle(rh) checkValidRiffHandleWithRetVal(rh, RIFF_ERROR_INVALID_HANDLE)

// Table to translate error codes to strings, corresponds to RIFF_ERROR_... macros
static const char *riff_es[] = {
	//0
	"No error",
	//1
	"End of chunk",
	//2
	"End of chunk list",
	//3
	"Excess bytes at end of file",
	
	//4
	"Illegal four character id",
	//5
	"Chunk size exceeds list level or file",
	//6
	"End of RIFF file",
	//7
	"File access failed",
	//8
	"Invalid riff_handle",
	
	
	//9
	//all other
	"Unknown RIFF error"  
};




//*** default access FP setup ***

/*****************************************************************************/
//default print function
int riff_printf(const char *format, ... ){
	va_list args;
	va_start(args, format);
	int r = vfprintf(stderr, format, args);
	va_end (args);
	return r;
}


//** FILE **


/*****************************************************************************/
size_t read_file(riff_handle *rh, void *ptr, size_t size){
	return fread(ptr, 1, size, (FILE*)(rh->fh));
}

/*****************************************************************************/
riff_ufs_t seek_file(riff_handle *rh, riff_ufs_t pos){
	__RIFF_FSEEK((FILE*)(rh->fh), pos, SEEK_SET);
	return pos;
}

/*****************************************************************************/
//description: see header file
int riff_open_file(riff_handle *rh, FILE *f, riff_ufs_t size){
	checkValidRiffHandle(rh);
	rh->fh = f;
	rh->size = size;
	rh->cl_pos_start = __RIFF_FTELL(f); //current file offset of stream considered as start of RIFF file
	
	rh->fp_read = &read_file;
	rh->fp_seek = &seek_file;
	
	return riff_readHeader(rh);
}



//** memory **


/*****************************************************************************/
size_t read_mem(riff_handle *rh, void *ptr, size_t size){
	memcpy(ptr, ((uint8_t*)rh->fh+rh->pos), size);
	return size;
}

/*****************************************************************************/
riff_ufs_t seek_mem(riff_handle *rh, riff_ufs_t pos){
	return pos; //instant in memory
}

/*****************************************************************************/
//description: see header file
int riff_open_mem(riff_handle *rh, const void *ptr, riff_ufs_t size){
	checkValidRiffHandle(rh);
	
	rh->fh = (void *)ptr;
	rh->size = size;
	rh->cl_pos_start = 0; // passed memory pointer is always expected to point to start of riff file
	
	rh->fp_read = &read_mem;
	rh->fp_seek = &seek_mem;
	
	return riff_readHeader(rh);
}



// **** internal ****



/*****************************************************************************/
//pass pointer to 32 bit LE value and convert, return in native byte order
uint32_t convUInt32LE(const void *p){
	const uint8_t *c = (const uint8_t*)p;
	return c[0] | (c[1] << 8) | (c[2] << 16) | (c[3] << 24);
}


/*****************************************************************************/
//read 32 bit LE from file via FP and return as native
uint32_t readUInt32LE(riff_handle *rh){
	char buf[4] = "";	// Init to 0
	rh->fp_read(rh, buf, 4);
	rh->pos += 4;
	rh->c_pos += 4;
	return convUInt32LE(buf);
}


/*****************************************************************************/
//read chunk header
//return error code
int riff_readChunkHeader(riff_handle *rh){
	checkValidRiffHandle(rh);

	char buf[8];
	
	int n = rh->fp_read(rh, buf, 8);
	
	if(n != 8){
		if(rh->fp_printf)
			rh->fp_printf("Failed to read header, %d of %d bytes read!\n", n, 8);
		return RIFF_ERROR_EOF; //return error code
	}
	
	rh->c_pos_start = rh->pos;
	rh->pos += n;
	
	memcpy(rh->c_id, buf, 4);
	rh->c_size = convUInt32LE(buf + 4);
	rh->pad = rh->c_size & 0x1; //pad byte present if size is odd
	rh->c_pos = 0;
	
	
	//verify valid chunk ID, must contain only printable ASCII chars
	int i;
	for(i = 0; i < 4; i++) {
		if(rh->c_id[i] < 0x20  ||  rh->c_id[i] > 0x7e) {
			if(rh->fp_printf)
				rh->fp_printf("Invalid chunk ID (FOURCC) of chunk at file pos %d: 0x%02x,0x%02x,0x%02x,0x%02x\n", rh->c_pos_start, rh->c_id[0], rh->c_id[1], rh->c_id[2], rh->c_id[3]);
			return RIFF_ERROR_ILLID;
		}
	}
	
	
	//check if chunk fits into current list level and file, value could be corrupt
	riff_ufs_t cposend = rh->c_pos_start + RIFF_CHUNK_DATA_OFFSET + rh->c_size + rh->pad;
	riff_ufs_t listend = rh->cl_pos_start + RIFF_CHUNK_DATA_OFFSET + rh->cl_size;
	
	if(cposend > listend){
		if(rh->fp_printf)
			rh->fp_printf("Chunk size exceeds list size! At least one size value must be corrupt!");
		//chunk data must be considered as cut off, better skip this chunk
		return RIFF_ERROR_ICSIZE;
	}
	
	//check chunk size against file size
	if((rh->size > 0)  &&  (cposend > rh->size)){
		if(rh->fp_printf)
			rh->fp_printf("Chunk size exceeds file size! At least one size value must be corrupt!");
		return RIFF_ERROR_EOF; //Or better RIFF_ERROR_ICSIZE?
	}
	
	return RIFF_ERROR_NONE;
}


/*****************************************************************************/
//pop from level stack
//when returning we are positioned inside the parent chunk ()
void stack_pop(riff_handle *rh){
	if(rh->ls_level <= 0)
		return;
	
	rh->ls_level--;
	riff_levelStackEntry *ls = rh->ls + rh->ls_level;

	// Actually exchange the params
	// First put the current chunk level's data back into the chunk fields:
	memcpy(rh->c_id, rh->cl_id, 4);
	rh->c_size = rh->cl_size;
	rh->c_pos_start = rh->cl_pos_start;
	// Then get the parent level data from the stack:
	memcpy(rh->cl_id, ls->cl_id, 4);
	rh->cl_size = ls->cl_size;
	memcpy(rh->cl_type, ls->cl_type, 4);
	rh->cl_pos_start = ls->cl_pos_start;
	
	rh->pad = rh->c_size & 0x1; //pad if chunk sizesize is odd
	
	rh->c_pos = rh->pos - rh->c_pos_start - RIFF_CHUNK_DATA_OFFSET;
}


/*****************************************************************************/
//push to level stack
void stack_push(riff_handle *rh, const char *type){
	//need to enlarge stack?
	if(rh->ls_size < rh->ls_level + 1){
		size_t ls_size_new = rh->ls_size * 2; //double size
		if(ls_size_new == 0)
			ls_size_new = RIFF_LEVEL_ALLOC; //default stack allocation
		
		riff_levelStackEntry *lsnew = calloc(ls_size_new, sizeof(riff_levelStackEntry));
		rh->ls_size = ls_size_new;
		
		//need to copy?
		if(rh->ls_level > 0){
			memcpy(lsnew, rh->ls, rh->ls_level * sizeof(riff_levelStackEntry));
		}
		
		//free old
		if(rh->ls != NULL)
			free(rh->ls);
		rh->ls = lsnew;
	}
	
	riff_levelStackEntry *ls = rh->ls + rh->ls_level;
	// Actually exchange the params
	// First move the parent level data to the stack:
	memcpy(ls->cl_id, rh->cl_id, 4);
	ls->cl_size = rh->cl_size;
	memcpy(ls->cl_type, rh->cl_type, 4);
	ls->cl_pos_start = rh->cl_pos_start;
	// Then put the current chunk's data into the chunk level fields:
	memcpy(rh->cl_id, rh->c_id, 4);
	rh->cl_size = rh->c_size;
	memcpy(rh->cl_type, type, 4);
	rh->cl_pos_start = rh->c_pos_start;

	//printf("list size %d\n", (rh->ls[rh->ls_level].size));
	rh->ls_level++;
}


//**** user access ****


/*****************************************************************************/
//description: see header file
riff_handle *riff_handleAllocate(){
	riff_handle *rh = calloc(1, sizeof(riff_handle));
	if(rh != NULL){
		rh->fp_printf = riff_printf;
	}
	return rh;
}

/*****************************************************************************/
//description: see header file
//Deallocate riff_handle and contained stack, file source (memory) is not closed or freed
void riff_handleFree(riff_handle *rh){
	if(rh == NULL)
		return;
	//free stack
	if(rh->ls != NULL)
		free(rh->ls);
	//free struct
	free(rh);
}

/*****************************************************************************/
//description: see header file
//shall be called only once by the open-function
int riff_readHeader(riff_handle *rh){
	checkValidRiffHandle(rh);

	char buf[RIFF_HEADER_SIZE];
	
	if(rh->fp_read == NULL) {
		if(rh->fp_printf)
			rh->fp_printf("I/O function pointer not set\n"); //fatal user error
		return RIFF_ERROR_INVALID_HANDLE;
	}
	
	size_t n = rh->fp_read(rh, buf, RIFF_HEADER_SIZE);
	rh->pos += n;
	
	if(n != RIFF_HEADER_SIZE){
		if(rh->fp_printf)
			rh->fp_printf("Read error, failed to read RIFF header\n");
		//printf("%d", n);
		return RIFF_ERROR_EOF; //return error code
	}
	memcpy(rh->cl_id, buf, 4);
	rh->cl_size = convUInt32LE(buf + 4);
	memcpy(rh->cl_type, buf + 8, 4);


	if(memcmp(rh->cl_id, "RIFF", 4) != 0 && memcmp(rh->cl_id, "BW64", 4) != 0) {
		if(rh->fp_printf)
			rh->fp_printf("Invalid RIFF header\n");
		return RIFF_ERROR_ILLID;
	}

	int r = riff_readChunkHeader(rh);
	if(r != RIFF_ERROR_NONE)
		return r;

	#if RIFF_64BIT_FILESIZE_SUPPORT

		if (rh->cl_size == 0xFFFFFFFF && !memcmp(rh->c_id, "ds64", 4)) {
			// It's a 64-bit sized file
			// Specification can be found at
			// https://www.itu.int/dms_pubrec/itu-r/rec/bs/R-REC-BS.2088-1-201910-I!!PDF-E.pdf
			
			// Buffer already used, so it can be reused
			size_t r_ = riff_readInChunk(rh, buf, 8);
			if (r_ != 8) {
				if (rh->fp_printf) {
					rh->fp_printf("ds64 chunk too small to contain any meaningful information.\n");
				}
				return RIFF_ERROR_ICSIZE;
			}
			rh->cl_size = ((riff_ufs_t)convUInt32LE(buf+4) << 32) | convUInt32LE(buf);
		}

	#endif
	
	//compare with given file size
	if(rh->size != 0){
		if(rh->size != rh->cl_size + RIFF_CHUNK_DATA_OFFSET){
			if(rh->fp_printf)
				rh->fp_printf("RIFF header chunk size %" __RIFF_FS_FMT " doesn't match file size %" __RIFF_FS_FMT "!\n", rh->cl_size + RIFF_CHUNK_DATA_OFFSET, rh->size);
			if(rh->size >= rh->cl_size + RIFF_CHUNK_DATA_OFFSET)
				return RIFF_ERROR_EXDAT;
			else
				//end isn't reached yet and you can parse further
				//but file seems to be cut off or given file size (via open-function) was too small -> we are not allowed to read beyond
				return RIFF_ERROR_EOF;
		}
	}

	return RIFF_ERROR_NONE;
}



// **** external ****



//make use of user defined functions via FPs

/*****************************************************************************/
//read to memory block, returns number of successfully read bytes
//keep track of position, do not read beyond end of chunk, pad byte is not read
size_t riff_readInChunk(riff_handle *rh, void *to, size_t size){
	riff_ufs_t left = rh->c_size - rh->c_pos;
	if(left < size)
		size = left;
	size_t n = rh->fp_read(rh, to, size);
	rh->pos += n;
	rh->c_pos += n;
	return n;
}

/*****************************************************************************/
//seek byte position in current chunk data from start of chunk data, return error on failure
//keep track of position
//c_pos: relative offset from chunk data start
int riff_seekInChunk(riff_handle *rh, riff_ufs_t c_pos){
	checkValidRiffHandle(rh);
	//seeking behind last byte is valid, next read at that pos will fail
	if(c_pos < 0  ||  c_pos > rh->c_size){
		return RIFF_ERROR_EOC;
	}
	rh->pos = rh->c_pos_start + RIFF_CHUNK_DATA_OFFSET + c_pos;
	rh->c_pos = c_pos;
	riff_ufs_t r = rh->fp_seek(rh, rh->pos); //seek never fails, but pos might be invalid to read from
	return RIFF_ERROR_NONE;
}


/*****************************************************************************/
//description: see header file
int riff_seekNextChunk(riff_handle *rh){
	checkValidRiffHandle(rh);

	riff_ufs_t posnew = rh->c_pos_start + RIFF_CHUNK_DATA_OFFSET + rh->c_size + rh->pad; //expected pos of following chunk
	
	riff_ufs_t listend = rh->cl_pos_start + RIFF_CHUNK_DATA_OFFSET + rh->cl_size; //at level 0
	
	//printf("listend %" __RIFF_FS_FMT "  posnew %" __RIFF_FS_FMT "\n", listend, posnew);  //debug
	
	//if no more chunks in the current sub list level
	if(listend < posnew + RIFF_CHUNK_DATA_OFFSET){
		//there shouldn't be any pad bytes at the list end, since the containing chunks should be padded to even number of bytes already
		//we consider excess bytes as non critical file structure error
		if(listend > posnew){
			if(rh->fp_printf)
				rh->fp_printf("%" __RIFF_FS_FMT " excess bytes at pos %" __RIFF_FS_FMT " at end of chunk list!\n", listend - posnew, posnew);
			return RIFF_ERROR_EXDAT;
		}
		return RIFF_ERROR_EOCL;
	}
	
	rh->pos = posnew;
	rh->c_pos = 0; 
	rh->fp_seek(rh, posnew);
	
	return riff_readChunkHeader(rh);
}


/*****************************************************************************/
int riff_seekChunkStart(riff_handle *rh){
	checkValidRiffHandle(rh);

	//seek data offset 0 in current chunk
	rh->pos = rh->c_pos_start + RIFF_CHUNK_DATA_OFFSET;
	rh->c_pos = 0;
	rh->fp_seek(rh, rh->pos);
	return RIFF_ERROR_NONE;
}


/*****************************************************************************/
int riff_rewind(riff_handle *rh){
	checkValidRiffHandle(rh);

	//pop stack as much as possible
	while(rh->ls_level > 0) {
		stack_pop(rh);
	}
	return riff_seekLevelStart(rh);
}

/*****************************************************************************/
int riff_seekLevelStart(riff_handle *rh){
	checkValidRiffHandle(rh);
			
	rh->pos = rh->cl_pos_start + RIFF_CHUNK_DATA_OFFSET + 4; //pos after type ID of chunk list
	rh->c_pos = 0;
	rh->fp_seek(rh, rh->pos);

	//read first chunk header, so we have the right values
	int r = riff_readChunkHeader(rh);
	
	//check possible?
	return r;
}


/*****************************************************************************/
//description: see header file
int riff_seekLevelSub(riff_handle *rh){
	checkValidRiffHandle(rh);

	//according to "https://en.wikipedia.org/wiki/Resource_Interchange_File_Format" only RIFF and LIST chunk IDs can contain subchunks
	if(memcmp(rh->c_id, "LIST", 4) != 0  && memcmp(rh->c_id, "RIFF", 4) != 0
	#if RIFF_64BIT_FILESIZE_SUPPORT
	&& memcmp(rh->c_id, "BW64", 4) != 0
	#endif
	) {
		if(rh->fp_printf)
			#if RIFF_64BIT_FILESIZE_SUPPORT
				rh->fp_printf("%s() failed for chunk ID \"%s\", only RIFF, BW64 or LIST chunks can contain subchunks", __func__, rh->c_id);
			#else
				rh->fp_printf("%s() failed for chunk ID \"%s\", only RIFF or LIST chunks can contain subchunks", __func__, rh->c_id);
			#endif
		return RIFF_ERROR_ILLID;
	}
	
	//check size of parent chunk data, must be at least 4 for type ID (is empty list allowed?)
	if(rh->c_size < 4){
		if(rh->fp_printf)
			rh->fp_printf("Chunk too small to contain sub level chunks\n");
		return RIFF_ERROR_ICSIZE;
	}
	
	//seek to chunk start if not there, required to read type ID
	if(rh->c_pos > 0) {
		rh->fp_seek(rh, rh->c_pos_start + RIFF_CHUNK_DATA_OFFSET);
		rh->pos = rh->c_pos_start + RIFF_CHUNK_DATA_OFFSET;
		rh->c_pos = 0;
	}
	//read type ID
	char type[5] = "";	// Init to 0
	rh->fp_read(rh, type, 4);
	rh->pos += 4;
	//verify type ID
	int i;
	for(i = 0; i < 4; i++) {
		if(type[i] < 0x20  ||  type[i] > 0x7e) {
			if(rh->fp_printf)
				rh->fp_printf("Invalid chunk type ID (FOURCC) of chunk at file pos %d: 0x%02x,0x%02x,0x%02x,0x%02x\n", rh->c_pos_start, type[0], type[1], type[2], type[3]);
			return RIFF_ERROR_ILLID;
		}
	}
	
	//add parent chunk data to stack
	//push
	stack_push(rh, type);
	
	return riff_readChunkHeader(rh);
}


/*****************************************************************************/
//description: see header file
int riff_levelParent(riff_handle *rh){
	checkValidRiffHandle(rh);

	if(rh->ls_level <= 0)
		return -1;  //not critical error, we don't have or need a macro for that
	stack_pop(rh);
	return RIFF_ERROR_NONE;
}

/*****************************************************************************/
int riff_seekLevelParentStart(riff_handle *rh){
	checkValidRiffHandle(rh);

	int r;
	if ((r = riff_levelParent(rh)) != RIFF_ERROR_NONE) return r;
	return riff_seekChunkStart(rh);
}

/*****************************************************************************/
int riff_seekLevelParentNext(riff_handle *rh){
	checkValidRiffHandle(rh);

	int r;
	if ((r = riff_levelParent(rh)) != RIFF_ERROR_NONE) return r;
	return riff_seekNextChunk(rh);
}


/*****************************************************************************/
int riff_levelValidate(riff_handle *rh){
	checkValidRiffHandle(rh);

	int r;
	//seek to start of current list
	if((r = riff_seekLevelStart(rh)) != RIFF_ERROR_NONE)
		return r;
	
	//seek all chunks of current list level
	while(1){
		r = riff_seekNextChunk(rh);
		if(r != RIFF_ERROR_NONE){
			if(r == RIFF_ERROR_EOCL) //just end of list
				break;
			//error occured, was probably printed already
			return r;
		}
	}
	return RIFF_ERROR_NONE;
}

/*****************************************************************************/

// Internal function, do not use
int riff_recursiveLevelValidate(riff_handle *rh){
	int r;
	while (1) {
		r = riff_seekNextChunk(rh);
		if (r != RIFF_ERROR_NONE) {
			if (r == RIFF_ERROR_EOCL) {
				// End of chunk list, time to come back
				return riff_levelParent(rh);
			} else return r; // Otherwise, some shit occured
		}
		if (!(memcmp(rh->c_id, "LIST", 4) != 0 && memcmp(rh->c_id, "RIFF", 4) != 0 && memcmp(rh->c_id, "BW64", 4) != 0)) { // If the chunk can contain subchunks
			r = riff_seekLevelSub(rh);
			if (r != RIFF_ERROR_NONE) return r;
			r = riff_recursiveLevelValidate(rh);
			if (r != RIFF_ERROR_NONE) return r;
		}
	}
	return RIFF_ERROR_NONE;
}

int riff_fileValidate(riff_handle *rh){
	checkValidRiffHandle(rh);

	int r;
	//seek to start of file
	if((r = riff_rewind(rh)) != RIFF_ERROR_NONE)
		return r;

	//seek all chunks
	return riff_recursiveLevelValidate(rh);
}

/*****************************************************************************/
riff_sfs_t riff_amountOfChunksInLevel(riff_handle *rh){
	checkValidRiffHandle(rh);

	riff_sfs_t counter = 0;
	int r;
	//seek to start of current list
	if((r = riff_seekLevelStart(rh)) != RIFF_ERROR_NONE)
		return (riff_sfs_t)-1;
	
	//seek all chunks of current list level
	while(1){
		counter++;
		r = riff_seekNextChunk(rh);
		if(r != RIFF_ERROR_NONE){
			if(r == RIFF_ERROR_EOCL)  //just end of list
				break;
			//error occured
			return -1;
		}
	}
	return counter;
}

/*****************************************************************************/
riff_sfs_t riff_amountOfChunksInLevelWithID(riff_handle *rh, const char * id){
	checkValidRiffHandle(rh);

	riff_sfs_t counter = 0;
	int r;
	//seek to start of current list
	if((r = riff_seekLevelStart(rh)) != RIFF_ERROR_NONE)
		return -1;
	
	//seek all chunks of current list level
	while(1){
		if (!memcmp(rh->c_id, id, 4)) counter++;
		r = riff_seekNextChunk(rh);
		if(r != RIFF_ERROR_NONE){
			if(r == RIFF_ERROR_EOCL) //just end of list
				break;
			//error occured
			return -1;
		}
	}
	return counter;
}

/*****************************************************************************/
riff_levelStackEntry * getChunkLevelInfo (riff_handle * rh, int level) {
	checkValidRiffHandleWithRetVal(rh, NULL);
	if (level > rh->ls_level || level > rh->ls_size) return NULL;

	if (level == rh->ls_level) return (riff_levelStackEntry *)rh;	// The same structure
	// level < rh->ls_level
	return rh->ls+level;
}

/*****************************************************************************/
//description: see header file
const char *riff_errorToString(int e){
	//map error to error string
	//Make sure mapping is correct!
	if (e >= 0 && e <= RIFF_ERROR_MAX) return riff_es[e];
	else return riff_es[9];
}

