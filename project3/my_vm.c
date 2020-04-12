#include "my_vm.h"

int first_time_flag = 5;
typedef unsigned char byte_t; //represents 1 byte
byte_t * physical_mem; //pointer to physical memory
int num_physical_pages; //no of pages in physical memory
int num_virtual_pages; //no of pages in virtual memory
byte_t * physical_bitmap; //to check whether page of physical memory is free or not
byte_t * virtual_bitmap; //to check whether page of virtual memory is free or not
double page_offset_bits; //d
double phy_page_bits; //f
double out_page_bits; //p1
double inn_page_bits; //p2
/*
Function responsible for allocating and setting your physical memory 
*/
void set_physical_mem() {

    //Allocate physical memory using mmap or malloc; this is the total size of
    //your memory you are simulating

    	physical_mem = (byte_t *) malloc(sizeof(byte_t) * MEMSIZE);
    
    //HINT: Also calculate the number of physical and virtual pages and allocate
    //virtual and physical bitmaps and initialize them

	//calculate the number of physical and virtual pages
	num_physical_pages = MEMSIZE / PGSIZE; 
	num_virtual_pages = MAX_MEMSIZE / PGSIZE;

	//allocate virtual and physical bitmaps
	//divide number of pages by 8 to get number of bytes
	physical_bitmap = (byte_t *) malloc(sizeof(byte_t) * (num_physical_pages / 8));
	virtual_bitmap = (byte_t *) malloc(sizeof(byte_t) * (num_virtual_pages / 8));

	//initialize bitmaps
	int i = 0;
	for(i = 0; i < (num_physical_pages / 8); i++)
		physical_bitmap[i] = 0; //initially all the pages are free
	
	for(i = 0; i < (num_virtual_pages / 8); i++)
		virtual_bitmap[i] = 0; //initially all the pages are free

	//calculting the num of bits in offset (d) w.r.t. page size
	page_offset_bits = log2 (PGSIZE);
	double va_bits = log2 (MAX_MEMSIZE);

	//calculating the num of bits in p1 and p2
	int n = va_bits - page_offset_bits; //32 - 12 = 20
	//double p1_bits, p2_bits; //p1_bits is the number of bits for outer page table and p2_bits if for inner
	//divide them into half for each
	out_page_bits = n/2;
	inn_page_bits = n/2;
	
	if(va_bits > (page_offset_bits + out_page_bits + inn_page_bits))
		inn_page_bits++; //some odd number occurs, give extra bit to p1_bits
	
	phy_page_bits = log2 (MEMSIZE) - page_offset_bits;
	
	page_directory = (pde_t *) malloc(sizeof(pde_t) * out_page_bits);
	page_table = (pte_t **) malloc(sizeof(pte_t *) * out_page_bits);
	for(i=0; i<out_page_bits; i++) {
		page_table[i] = (pte_t *) malloc(sizeof(pte_t) * inn_page_bits);
	}

	free_frame = 0;
	free_page = 0;
	dir_entry = 0;
}


/*
 * Part 2: Add a virtual to physical page translation to the TLB.
 * Feel free to extend the function arguments or return type.
 */
int
add_TLB(void *va, void *pa)
{

    /*Part 2 HINT: Add a virtual to physical page translation to the TLB */

    return -1;
}


/*
 * Part 2: Check TLB for a valid translation.
 * Returns the physical page address.
 * Feel free to extend this function and change the return type.
 */
pte_t *
check_TLB(void *va) {

    /* Part 2: TLB lookup code here */

}


/*
 * Part 2: Print TLB miss rate.
 * Feel free to extend the function arguments or return type.
 */
void
print_TLB_missrate()
{
    double miss_rate = 0;	

    /*Part 2 Code here to calculate and print the TLB miss rate*/




    fprintf(stderr, "TLB miss rate %lf \n", miss_rate);
}



/*
The function takes a virtual address and page directories starting address and
performs translation to return the physical address
*/
pte_t *translate(pde_t *pgdir, void *va) {
    /* Part 1 HINT: Get the Page directory index (1st level) Then get the
    * 2nd-level-page table index using the virtual address.  Using the page
    * directory index and page table index get the physical address.*/

	//p1, p2 and d makes the virtual address where p1 is the outper page table
	//p2 is the page table and d is the offset

	pde_t v_addr = (pde_t) va; //get the virtual address

	//initially all p1, p2 and d have all 1's in it
	pte_t d = (pte_t) physical_mem; //it is 64 bit number
	pde_t outer = 0xFFFFFFFF; 
	pte_t inner = 0xFFFFFFFF;

	//extract p1, p2 and d from virtual address
	//d >>= (int) (32 - page_offset_bits); //making a mask
	//d = d & v_addr;
	
	inner <<= (int) page_offset_bits;
	inner <<= (int) (32 - (inn_page_bits + page_offset_bits));
	inner >>= (int) (32 - (inn_page_bits + page_offset_bits));
	inner = inner & v_addr;
	inner >>= (int) page_offset_bits;

	outer <<= (int) (inn_page_bits + page_offset_bits);
	outer <<= (int) (32 - (inn_page_bits + page_offset_bits + out_page_bits));
	outer >>= (int) (32 - (inn_page_bits + page_offset_bits + out_page_bits));
	outer = outer & v_addr;
	outer >>= (int) (page_offset_bits + inn_page_bits);

	//extracting frame number (physical mem page) from page tables
	pte_t f = page_table[pgdir[outer]][inner];
	
	//making physical address from frame number f and offset d
	//f <<= (int) page_offset_bits;
	//pte_t p_addr = f | d;

	d += f*PGSIZE;
	
	return (void *) d;

    /* Part 2 HINT: Check the TLB before performing the translation. If
    * translation exists, then you can return physical address from the TLB.
    */


    //If translation not successfull
    //return NULL; 
}


/*
The function takes a page directory address, virtual address, physical address
as an argument, and sets a page table entry. This function will walk the page
directory to see if there is an existing mapping for a virtual address. If the
virtual address is not present, then a new entry will be added
*/
int
page_map(pde_t *pgdir, void *va, void *pa)
{

    /*HINT: Similar to translate(), find the page directory (1st level)
    and page table (2nd-level) indices. If no mapping exists, set the
    virtual to physical mapping */

	pde_t v_addr = (pde_t) va; //get the virtual address

	//initially all p1, p2 and d have all 1's in it
	pte_t d = 0xFFFFFFFF; //it is 32 bit number
	pde_t outer = 0xFFFFFFFF; 
	pte_t inner = 0xFFFFFFFF;

	//extract p1, p2 and d from virtual address
	d >>= (int) (32 - page_offset_bits); //making a mask
	d = d & v_addr;
	
	inner <<= (int) page_offset_bits;
	inner <<= (int) (32 - (inn_page_bits + page_offset_bits));
	inner >>= (int) (32 - (inn_page_bits + page_offset_bits));
	inner = inner & v_addr;
	inner >>= (int) page_offset_bits;

	outer <<= (int) (inn_page_bits + page_offset_bits);
	outer <<= (int) (32 - (inn_page_bits + page_offset_bits + out_page_bits));
	outer >>= (int) (32 - (inn_page_bits + page_offset_bits + out_page_bits));
	outer = outer & v_addr;
	outer >>= (int) (page_offset_bits + inn_page_bits);

	
	pte_t f = (pde_t) pa;
	f >>= (int) page_offset_bits;
	page_table[pgdir[outer]][inner] = f;
}


/*Function that gets the next available page
*/
void *get_next_avail(int num_pages) {

    //Use virtual address bitmap to find the next free page
	int i=0;
	pte_t page = free_page;
	pte_t start = free_page;
	int notFound = 0;
	for(i=0; i<num_pages; i++) {
		
		int byte_ind = free_page/8;
		int bit_offset = free_page % 8;

		byte_t check_byte = virtual_bitmap[byte_ind];

		byte_t mask = 0x80;
		mask >>= bit_offset;

		byte_t temp = check_byte & mask; //temp=0 -> free page
		if(temp != 0) {
			i=0;
			free_page++;
			page = free_page++;
			continue;
		}
		free_page = (free_page + 1) % num_virtual_pages; // make it circular
		if(free_page == start) {
			notFound = 1;
			break;
		}
	}
	if(notFound == 1) {
		return NULL;
	}
	pte_t * pp = &page;
	return pp;
}


/* Function responsible for allocating pages
and used by the benchmark
*/
void *a_malloc(unsigned int num_bytes) {

    /* 
     * HINT: If the physical memory is not yet initialized, then allocate and initialize.
     */

   /* 
    * HINT: If the page directory is not initialized, then initialize the
    * page directory. Next, using get_next_avail(), check if there are free pages. If
    * free pages are available, set the bitmaps and map a new page. Note, you will 
    * have to mark which physical pages are used. 
    */

	if(first_time_flag != 0) {
		set_physical_mem();
		first_time_flag = 0;
	}

	int pages_required = (num_bytes / PGSIZE) + 1; //get pages required

	pte_t * page = (pte_t *) get_next_avail(pages_required);

	//setting virtual page bitmap
	int i=0;
	for(i=0; i<pages_required; i++) {
		int bit_offset = (*page + i) % 8;
		int byte_ind = (*page + i) / 8;
		byte_t mask = 0x80;
		mask >>= (int) bit_offset;
		virtual_bitmap[byte_ind] |= (int) mask;
	}
	
	//setting pysical page bitmap
	for(i=0; i<pages_required; i++) {
		int bit_offset = (free_frame + i) % 8;
		int byte_ind = (free_frame + i) / 8;
		byte_t mask = 0x80;
		mask >>= (int) bit_offset;
		physical_bitmap[byte_ind] |= (int) mask;
		free_frame = (free_frame + 1) % num_physical_pages;
	}

	//Page Mapping
	pde_t outer = (*page) / 1024;
	pte_t inner = (*page) % 1024;
	page_directory[dir_entry] = outer;
	dir_entry = (dir_entry + 1) % 1024;
	pte_t va = 0;
	outer <<= (int) (inn_page_bits + page_offset_bits);
	inner <<= (int) page_offset_bits;
	va |= (int) outer;
	va |= (int) inner; //got virtul address

	pte_t f = free_frame;
	f <<= (int) page_offset_bits;
	pte_t pa = 0;
	pa |= (int) f;

	page_map(page_directory, va, pa);

    return (void *) va;
}

/* Responsible for releasing one or more memory pages using virtual address (va)
*/
void a_free(void *va, int size) {

    /* Part 1: Free the page table entries starting from this virtual address
     * (va). Also mark the pages free in the bitmap. Perform free only if the 
     * memory from "va" to va+size is valid.
     */
	int pages_required = (size / PGSIZE) + 1; //get pages required

	pde_t v_addr = (pde_t) va; //get the virtual address

	//initially all p1, p2 and d have all 1's in it
	pde_t outer = 0xFFFFFFFF; 
	pte_t inner = 0xFFFFFFFF;
	
	inner <<= (int) page_offset_bits;
	inner <<= (int) (32 - (inn_page_bits + page_offset_bits));
	inner >>= (int) (32 - (inn_page_bits + page_offset_bits));
	inner = inner & v_addr;
	inner >>= (int) page_offset_bits;

	outer <<= (int) (inn_page_bits + page_offset_bits);
	outer <<= (int) (32 - (inn_page_bits + page_offset_bits + out_page_bits));
	outer >>= (int) (32 - (inn_page_bits + page_offset_bits + out_page_bits));
	outer = outer & v_addr;
	outer >>= (int) (page_offset_bits + inn_page_bits);
	
	pte_t page = (outer*1024) + inner;
	dir_entry--;
	free_page -= pages_required;
	free_frame -= pages_required;
	
	//setting virtual page bitmap
	int i=0;
	for(i=0; i<pages_required; i++) {
		int bit_offset = (page + i) % 8;
		int byte_ind = (page + i) / 8;
		byte_t mask = 0x80;
		mask = ~mask;
		mask >>= (int) bit_offset;
		virtual_bitmap[byte_ind] &= (int) mask;
	}
	
	//setting pysical page bitmap
	for(i=0; i<pages_required; i++) {
		int bit_offset = (free_frame + i) % 8;
		int byte_ind = (free_frame + i) / 8;
		byte_t mask = 0x80;
		mask = ~mask;
		mask >>= (int) bit_offset;
		physical_bitmap[byte_ind] &= (int) mask;
		free_frame = (free_frame + 1) % num_physical_pages;
	}
	/*
     * Part 2: Also, remove the translation from the TLB
     */
}


/* The function copies data pointed by "val" to physical
 * memory pages using virtual address (va)
*/
void put_value(void *va, void *val, int size) {

    /* HINT: Using the virtual address and translate(), find the physical page. Copy
     * the contents of "val" to a physical page. NOTE: The "size" value can be larger 
     * than one page. Therefore, you may have to find multiple pages using translate()
     * function.
     */
	int i=0;
	unsigned int pages = (size/PGSIZE)+1;
	for(i=0;i<pages;i++){

		void* pa = translate(page_directory,va);
		//unsigned int address = (unsigned int)val+(i*PGSIZE);
		//handles remainder cases
		//printf("%p pa %p address %d sizePGSIZE\n", pa, address, size%PGSIZE);
		if(i==(pages-1)){
			memcpy(pa, val, size%PGSIZE);
			/*char * newpa = (char *) pa;
			int * newval = (int *) val;
			int g = *newval;
			*newpa = g;*/
			break;
		}
		else{
			memcpy(pa,val,PGSIZE);
		}
	}
}

/*Given a virtual address, this function copies the contents of the page to val*/
void get_value(void *va, void *val, int size) {

    /* HINT: put the values pointed to by "va" inside the physical memory at given
    * "val" address. Assume you can access "val" directly by derefencing them.
    */
    	int i=0;
	unsigned int pages = (size/PGSIZE)+1;
	for(i=0;i<pages;i++){

		void* pa = translate(page_directory,va);
		unsigned int address = (unsigned int)val+(i*PGSIZE);
		//handles remainder cases
		if(i==(pages-1)){
			memcpy((void*)address, pa,size%PGSIZE);
			break;
		}
		else{
			memcpy((void*)address, pa,PGSIZE);
		}
	}


}



/*
This function receives two matrices mat1 and mat2 as an argument with size
argument representing the number of rows and columns. After performing matrix
multiplication, copy the result to answer.
*/
void mat_mult(void *mat1, void *mat2, int size, void *answer) {

    /* Hint: You will index as [i * size + j] where  "i, j" are the indices of the
     * matrix accessed. Similar to the code in test.c, you will use get_value() to
     * load each element and perform multiplication. Take a look at test.c! In addition to 
     * getting the values from two matrices, you will perform multiplication and 
     * store the result to the "answer array"
     */
     int i=0, j=0, k=0;
    for (i = 0; i < size; ++i) {
        for (j = 0; j < size; ++j) {
		int a = 0;
		put_value(answer + (i*size*sizeof(int)) + (j*sizeof(int)), &a, sizeof(int));
        }
    }

	// Multiplying first and second matrices and storing in mult.
	int res =0;
    for (i = 0; i < size; ++i) {
        for (j = 0; j < size; ++j) {
            for (k = 0; k < size; ++k) {
		int first, second, prev;
		get_value(mat1 + (i*size*sizeof(int)) + (k*sizeof(int)), &first, sizeof(int));
		get_value(mat2 + (k*size*sizeof(int)) + (j*sizeof(int)), &second, sizeof(int));
		//get_value(answer + (i*size*sizeof(int)) + (j*sizeof(int)), &prev, sizeof(int));
		//int res = prev + (first*second);
		res+=(first*second);
            }
            put_value(answer + (i*size*sizeof(int)) + (j*sizeof(int)), &res, sizeof(int));
            res = 0;
        }
    }
   
}


