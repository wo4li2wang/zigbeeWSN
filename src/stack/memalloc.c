#include "compiler.h"               //compiler specific
#include <string.h>                 // has memset function
#include "lrwpan_config.h"
#include "hal.h"        //for global interrupt enable/disable
#include "halStack.h"
#include "memalloc.h"




//LRWPAN_HEAPSIZE must be < 64K, can allocate blocks up to this size
// format of block is
// free bit/15 bit size block_UINT8s | free bit/15 bit size block_UINT8s | etc..
// heap merging done on FREE, free adjacent blocks are merged.



//static UINT8 mem_heap[LRWPAN_HEAPSIZE];
#include "halHeapSpace.h"

typedef UINT16 MEMHDR;//内存分配机制，内存头('标志位'+'内存大小'，一共两个字节)+内存空间

/*MEMHDR_FREE标志取值含义          MEMHDR_SIZE含义*/
/*'1'表示后面存储空间为空          有多少空间可用*/
/*'0'表示后面存储空间已用          占用多少空间*/
#define MEMHDR_FREE_MASK 0x80   //定义MEMHDR_FREE_MASK
#define MEMHDR_SIZE_MASK 0x7FFF //定义MEMHDR_SIZE_MASK

#define MEMHDR_GET_FREE(x) (*(x+1)&MEMHDR_FREE_MASK)  //获得MEMHDR_FREE标志位
#define MEMHDR_CLR_FREE(x)  *(x+1) = *(x+1)&(~MEMHDR_FREE_MASK)  //清除MEMHDR_FREE标志位
#define MEMHDR_SET_FREE(x)  *(x+1) = *(x+1)|MEMHDR_FREE_MASK  //置MEMHDR_FREE标志位为1，低7位保持不变


UINT16 memhdr_get_size (UINT8 *ptr) {  //获得指针ptr指向的UINT16，只要低15位(*(ptr+1)*256+*ptr)
	UINT16 x;

	x = (UINT8) *ptr;    //指针PTR指向的内容存放在16位x的低八位
	x += ((UINT16) *(ptr+1)<< 8);  //先将指针+1,再将新指针的内容左移8位，放在刚才的x的高八位
	x = x & 0x7FFF;   //与操作，保留低15位作为内存大小值
	return(x);
}

void memhdr_set_size (UINT8 *ptr, UINT16 size) { //将值(size)存到ptr

	*ptr = (UINT8) size;   //指针初始化，强制取size的低8位
	ptr++;
	*ptr = *ptr & 0x80;  //clear size field 指针指向的内容最高位保持不变，低7位清0
	*(ptr) += (size >> 8);  //add in size. 将size的高8位右移8位后加到指针的内容中去
}

/*
#if 0   //下面的这段结构体此处不用到，如果需要时再定义位1
typedef struct _MEMHDR {  //定义结构体_MEMHDR
    UINT16 val;

    unsigned int free:1;  //free表示val的最高位为标志位
	unsigned int size:15;  //size表示val的低15位为内存大小值
}MEMHDR;
#endif
*/

#define MINSIZE 16+sizeof(MEMHDR)   //定义最小尺寸，最小尺寸为16＋2=18字节


void MemInit (void) {  //内存初始化，
	memset(mem_heap,0,LRWPAN_HEAPSIZE);  //用0填充该内存，数量为1024，指针指向mem_heap
	MEMHDR_SET_FREE(((UINT8 *)&mem_heap[0]));  //将指针mem_heap[1]的内容最高位置1(有什么含义???)，其他不变
    memhdr_set_size(((UINT8 *)&mem_heap[0]),(LRWPAN_HEAPSIZE - sizeof(MEMHDR)));
	//将空内存大小1022分别存到mem_heap[0]和它的下一个指针指向的内容中,mem_heap[0]=0xFE,mem_heap[1]=0x83
}


/*函数功能:申请内存函数，最小申请空间是16，如果空间不够，返回NULL*/
UINT8 * MemAlloc (UINT16 size) {  //内存分配

	UINT8 *free_blk, *next_blk;
	UINT16 offset;
	UINT16 remainder;
    BOOL  gie_status;



    if (!size) return(NULL);      //illegal size 如果size为空,返回NULL值
	if (size < MINSIZE) size = MINSIZE;  //取size值，当它比18字节还小时，取18字节

        SAVE_AND_DISABLE_GLOBAL_INTERRUPT(gie_status);  //保存中断标志并禁止中断
	free_blk = mem_heap;   //初始化指针空block
	offset = 0;  //偏移量初始化
	while (1) {
		if (MEMHDR_GET_FREE(((UINT8 *)free_blk)) &&
			(memhdr_get_size((UINT8 *)free_blk) >= size)) break; //当空block标志位为1，空block的大小大于等于size，跳出循环
		//found block   advance to next block
		offset = offset + memhdr_get_size((UINT8 *)free_blk) + sizeof(MEMHDR);  //取offset，大小为空block的大小加上2字节
                if (offset >= LRWPAN_HEAPSIZE) {  //当偏移量大于等于1024时
                  RESTORE_GLOBAL_INTERRUPT(gie_status);  //恢复全局中断
                  return(NULL); // no free blocks  没有空的block
                }
		free_blk = mem_heap + offset;  //重置指针空block
	}
	remainder =  memhdr_get_size((UINT8 *)free_blk) - size;  //初始化剩余内存大小，大小为空block的大小减去size
	if (remainder < MINSIZE) {  //当剩余内存小于最小尺寸时候
		//found block, mark as not-free   //建立block,标志为非空
		MEMHDR_CLR_FREE((UINT8 *)free_blk);  //清除空block的标志位。标志为非空
        RESTORE_GLOBAL_INTERRUPT(gie_status);  //恢复全局中断
		return(free_blk + sizeof(MEMHDR));  //返回一个值，该值的大小为空block的大小＋2字节
	}
	//remainder is large enough to support a new block   剩余空间足够大来支持一个新的block
	//adjust allocated block to requested size   调整已经分配的block到要求的大小
	memhdr_set_size(((UINT8 *)free_blk),size);  //获得size的大小
	//format next blk    设置下一个block
	next_blk = free_blk+size+sizeof(MEMHDR);    //指向下一个block，
	MEMHDR_SET_FREE((UINT8 *) next_blk);   //设置下一个block的标志位为1，标志为空
	memhdr_set_size(((UINT8 *) next_blk), (remainder - sizeof(MEMHDR)));  //获得下一个block的大小

	MEMHDR_CLR_FREE((UINT8 *)free_blk); //mark allocated block as non-free   标志已分配的block为非空
    RESTORE_GLOBAL_INTERRUPT(gie_status);  //恢复全局中断
	return(free_blk + sizeof(MEMHDR));      //return new block  返回一个值，该值的大小为空block的大小＋2字节
}

void MemFree(UINT8 *ptr) {  //释放内存
	UINT8 *hdr;
	UINT16 offset, tmp;
        BOOL  gie_status;


    if (ptr == NULL) return;  //指针为空，返回
    SAVE_AND_DISABLE_GLOBAL_INTERRUPT(gie_status);   //保存
	hdr = ptr - sizeof(MEMHDR); //初始化指针hdr，大小为ptr-2字节
	//free this block   释放该block
	MEMHDR_SET_FREE((UINT8 *)hdr);   //设置hdr指针指向内容标志位为1，标志为空
	//now merge    //合并
	offset = 0;    //初始化offset
	hdr = mem_heap;   //重置指针hdr
	//loop until blocks that can be merged are merged   //循环直到能合并的block都合并了
	while (1) {
		if (MEMHDR_GET_FREE((UINT8 *)hdr)) {  //定义获得标志位，表示为空，即标志指针mem_heap为空
			//found a free block, see if we can merge with next block   建立一个空block，看能否与下一个block合并
			tmp = offset +  memhdr_get_size((UINT8 *)hdr) + sizeof(MEMHDR);  //设置暂存值，大小为offset+2字节＋mem_heap大小
			if (tmp >= LRWPAN_HEAPSIZE) break; //at end of heap, exit loop 暂存值大于1024，跳出循环
			ptr = mem_heap + tmp; //point at next block   指针指向下一个block
			if (MEMHDR_GET_FREE((UINT8 *)ptr)) {   //标志下一个block为空
				//next block is free, do merge by adding size of next block  下一个block为空，合并
	            memhdr_set_size(((UINT8 *)hdr),(memhdr_get_size((UINT8 *)hdr)+ memhdr_get_size((UINT8 *)ptr)
					                            + sizeof(MEMHDR)));  //设置合并后的内存大小，存放在指针hdr中
				// after merge, do not change offset, try to merge again 合并该block后，尝试再合并下一个
				//next time through loop
				continue; //back to top of loop   回到循环开始
			}			
		}
		// next block
		offset = offset + memhdr_get_size((UINT8 *)hdr) + sizeof(MEMHDR);  //取新的block下offset值
		if (offset >= LRWPAN_HEAPSIZE) break;  //at end of heap, exit loop   offset大于等于1024，跳出循环
		hdr = mem_heap + offset;  //重置指针hdr
	}
	 RESTORE_GLOBAL_INTERRUPT(gie_status);  //恢复全局中断
}

#ifdef LRWPAN_COMPILER_NO_RECURSION  //编译器没有递归方式

//this supports the HI-TECH compiler, which does not support recursion  该编译器支持HI-TECH编译器，不支持递归
//下面这段程序执行的过程和上面已经注释的一样，具体注释参见以上程序

UINT16 ISR_memhdr_get_size (UINT8 *ptr) {
	UINT16 x;

	x = (UINT8) *ptr;
	x += ((UINT16) *(ptr+1)<< 8);
	x = x & 0x7FFF;
	return(x);
}

void ISR_memhdr_set_size (UINT8 *ptr, UINT16 size) {

	*ptr = (UINT8) size;
	ptr++;
	*ptr = *ptr & 0x80;  //clear size field
	*(ptr) += (size >> 8);  //add in size.
}



UINT8 * ISRMemAlloc (UINT16 size) {

	UINT8 *free_blk, *next_blk;
	UINT16 offset;
	UINT16 remainder;
        BOOL  gie_status;



        if (!size) return(NULL);      //illegal size
	if (size < MINSIZE) size = MINSIZE;

        SAVE_AND_DISABLE_GLOBAL_INTERRUPT(gie_status);
	free_blk = mem_heap;
	offset = 0;
	while (1) {
		if (MEMHDR_GET_FREE(((UINT8 *)free_blk)) &&
			(ISR_memhdr_get_size((UINT8 *)free_blk) >= size)) break; //found block
		//advance to next block
		offset = offset + ISR_memhdr_get_size((UINT8 *)free_blk) + sizeof(MEMHDR);
                if (offset >= LRWPAN_HEAPSIZE) {
                  RESTORE_GLOBAL_INTERRUPT(gie_status);
                  return(NULL); // no free blocks
                }
		free_blk = mem_heap + offset;
	}
	remainder =  ISR_memhdr_get_size((UINT8 *)free_blk) - size;
	if (remainder < MINSIZE) {
		//found block, mark as not-free
		MEMHDR_CLR_FREE((UINT8 *)free_blk);
        RESTORE_GLOBAL_INTERRUPT(gie_status);
		return(free_blk + sizeof(MEMHDR));
	}
	//remainder is large enough to support a new block
	//adjust allocated block to requested size
	ISR_memhdr_set_size(((UINT8 *)free_blk),size);
	//format next blk
	next_blk = free_blk+size+sizeof(MEMHDR);
	MEMHDR_SET_FREE((UINT8 *) next_blk);
	ISR_memhdr_set_size(((UINT8 *) next_blk), (remainder - sizeof(MEMHDR)));

	MEMHDR_CLR_FREE((UINT8 *)free_blk); //mark allocated block as non-free
    RESTORE_GLOBAL_INTERRUPT(gie_status);
	return(free_blk + sizeof(MEMHDR));      //return new block
}

void ISRMemFree(UINT8 *ptr) {
	UINT8 *hdr;
	UINT16 offset, tmp;
        BOOL  gie_status;


        SAVE_AND_DISABLE_GLOBAL_INTERRUPT(gie_status);
	hdr = ptr - sizeof(MEMHDR);
	//free this block
	MEMHDR_SET_FREE((UINT8 *)hdr);
	//now merge
	offset = 0;
	hdr = mem_heap;
	//loop until blocks that can be merged are merged
	while (1) {
		if (MEMHDR_GET_FREE((UINT8 *)hdr)) {
			//found a free block, see if we can merge with next block
			tmp = offset +  ISR_memhdr_get_size((UINT8 *)hdr) + sizeof(MEMHDR);
			if (tmp >= LRWPAN_HEAPSIZE) break; //at end of heap, exit loop
			ptr = mem_heap + tmp; //point at next block
			if (MEMHDR_GET_FREE((UINT8 *)ptr)) {
				//next block is free, do merge by adding size of next block
	            ISR_memhdr_set_size(((UINT8 *)hdr),(ISR_memhdr_get_size((UINT8 *)hdr)+ ISR_memhdr_get_size((UINT8 *)ptr)
					                            + sizeof(MEMHDR)));
				// after merge, do not change offset, try to merge again
				//next time through loop
				continue; //back to top of loop
			}			
		}
		// next block
		offset = offset + ISR_memhdr_get_size((UINT8 *)hdr) + sizeof(MEMHDR);
		if (offset >= LRWPAN_HEAPSIZE) break;  //at end of heap, exit loop
		hdr = mem_heap + offset;
	}
	 RESTORE_GLOBAL_INTERRUPT(gie_status);
}




#endif







