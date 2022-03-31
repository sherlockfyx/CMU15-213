/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "Fangyx",
    /* First member's full name */
    "Fang",
    /* First member's email address */
    "fangyx@seu.edu.cn",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

typedef unsigned int uint32_t;

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

//字大小和双字大小
#define WSIZE 4
#define DSIZE 8

//当堆内存不够时，向内核申请的堆空间
#define INITCHUNKSIZE (1<<6)
#define CHUNKSIZE (1<<12)
#define LISTMAX    16

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

//获得头部和脚部的编码
#define PACK(size, alloc) ((size) | (alloc))

//将val放入p开始的4字节中
#define GET(p) (*(unsigned int *)(p))
#define PUT(p,val) (*(unsigned int *)(p) = (val))

//将p指针指向的地址值设为ptr的地址
#define SET_PTR(p, ptr) (*(unsigned int* *)(p) = (unsigned int *)(ptr))

//获取与设置存储的指针
#define GET_PTR(p) ((unsigned int *)(long)(GET(p)))
#define PUT_PTR(p, ptr) (*(unsigned int *)(p) = ((long)ptr))

//从头部或脚部获得块大小和已分配位
#define GET_SIZE(p) (*(unsigned int *)(p) & ~0x7)
#define GET_ALLO(p) (*(unsigned int *)(p) & 0x1)

//获得块的头部和脚部
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

//获得上一个块和下一个块
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE((char *)(bp) - DSIZE))
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)))

//获得指向前驱和后继的指针的地址
#define PRED_PTR(ptr) ((char *)(ptr))
#define SUCC_PTR(ptr) ((char *)(ptr) + WSIZE)

//获得前驱和后继的指针
#define PRED(ptr) (*(char* *)(ptr))
#define SUCC(ptr) (*(char* *)(SUCC_PTR(ptr)))


/* 分离空闲表 */
void *segregated_free_lists[LISTMAX];

/* 扩展推 */
static void *extend_heap(size_t size);

/* 合并相邻的Free block */
static void *coalesce(void *bp);

/* 在bp所指向的free block块中allocate size大小的块，如果剩下的空间大于2*DWSIZE，则将其分离后放入Free list */
static void *place(void *bp, size_t size);

/* 将bp所指向的free block插入到分离空闲表中 */
static void insert_node(void *bp, size_t size);

/* 将bp所指向的块从分离空闲表中删除 */
static void delete_node(void *bp);


//  扩展堆
static void *extend_heap(size_t size) {

    void *bp;
    /* 内存对齐 */
    size = ALIGN(size);

    /* 系统调用“sbrk”扩展堆 */
    if ((bp = mem_sbrk(size)) == (void *)-1)
        return NULL;

    /* 设置刚刚扩展的free块的头和尾 */
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));

    /* 注意这个块是堆的结尾，所以还要设置一下结尾 */
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    /* 设置好后将其插入到分离空闲表中 */
    insert_node(bp, size);
    /* 另外这个free块的前面也可能是一个free块，可能需要合并 */
    return coalesce(bp);

}

static void insert_node(void *bp, size_t size) { // 插入一个块

    int i = 0;
    void *cur_bp = NULL;
    void *pre_bp = NULL;

    /* 通过块的大小找到对应的链 */
    //size >>= 3;
    while ((i < LISTMAX - 1) && (size > 1)) {
        size >>= 1;
        i++;
    }

    /* 找到对应的链后，在该链中继续寻找对应的插入位置，以此保持链中块由小到大的特性 */
    cur_bp = segregated_free_lists[i];
    while ((cur_bp != NULL) && (size > GET_SIZE(HDRP(cur_bp)))) {
        pre_bp = cur_bp;
        cur_bp = PRED(cur_bp);
    }

    /* 循环后有四种情况 */
    if (cur_bp != NULL) {
        if (pre_bp != NULL) {
            /* 1. ->xx->insert->xx 在中间插入*/
            SET_PTR(PRED_PTR(bp), cur_bp);
            SET_PTR(SUCC_PTR(bp), pre_bp);
            SET_PTR(SUCC_PTR(cur_bp), bp);
            SET_PTR(PRED_PTR(pre_bp), bp);
        } else {
            /* 2. [i]->insert->xx 在开头插入，而且后面有之前的free块*/
            SET_PTR(PRED_PTR(bp), cur_bp);
            SET_PTR(SUCC_PTR(bp), NULL);
            SET_PTR(SUCC_PTR(cur_bp), bp);
            segregated_free_lists[i] = bp;

            // segregated_free_lists[i] = bp;
            // SET_PTR(PRED_PTR(bp), NULL);
            // SET_PTR(SUCC_PTR(bp), cur_bp);

            // SET_PTR(PRED_PTR(cur_bp), bp);
        }
    } else { // cur_bp == NULL
        if (pre_bp != NULL) { 
            /* 3. ->xxxx->insert 在结尾插入*/
            SET_PTR(PRED_PTR(bp), NULL);
            SET_PTR(SUCC_PTR(bp), pre_bp);
            SET_PTR(PRED_PTR(pre_bp), bp);
        } else { 
            /* 4. [i]->insert 该链为空，这是第一次插入 */
            SET_PTR(PRED_PTR(bp), NULL);
            SET_PTR(SUCC_PTR(bp), NULL);
            segregated_free_lists[i] = bp;
        }
    }
}

static void delete_node(void *bp) {
    int i = 0;
    size_t size = GET_SIZE(HDRP(bp));

    /* 通过块的大小找到对应的链 */
    while ((i < LISTMAX - 1) && (size > 1)) {
        size >>= 1;
        i++;
    }

    /* 根据这个块的情况分四种可能性 */
    if (PRED(bp) != NULL) {
        /* 1. xxx-> bp -> xxx */
        if (SUCC(bp) != NULL) {
            SET_PTR(SUCC_PTR(PRED(bp)), SUCC(bp));
            SET_PTR(PRED_PTR(SUCC(bp)), PRED(bp));
        }
        /* 2. [i] -> bp -> xxx */
        else {
            SET_PTR(SUCC_PTR(PRED(bp)), NULL);
            segregated_free_lists[i] = PRED(bp);
        }
    } else {
        /* 3. [i] -> xxx -> bp */
        if (SUCC(bp) != NULL) {
            SET_PTR(PRED_PTR(SUCC(bp)), NULL);
        }
        /* 4. [i] -> bp */
        else {
            segregated_free_lists[i] = NULL;
        }
    }
}

static void *coalesce(void *bp) {
    uint32_t is_prev_alloc = GET_ALLO(HDRP(PREV_BLKP(bp)));
    uint32_t is_next_alloc = GET_ALLO(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    /* 根据bp所指向块前后相邻块的情况，可以分为四种可能性 */
    /* 另外注意到由于我们的合并和申请策略，不可能出现两个相邻的free块 */

    /* 1.前后均为allocated块，不做合并，直接返回 */
    if (is_prev_alloc && is_next_alloc) {
        return bp;
    } else if (is_prev_alloc && !is_next_alloc) {
    /* 2.前面的块是allocated，但是后面的块是free的，这时将两个free块合并 */
        //  先从空闲链表中删除
        delete_node(bp);
        delete_node(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    } else if (!is_prev_alloc && is_next_alloc) {
        /* 3.后面的块是allocated，但是前面的块是free的，这时将两个free块合并 */
        delete_node(bp);
        delete_node(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    } else {/* 4.前后两个块都是free块，这时将三个块同时合并 */
        delete_node(bp);
        delete_node(PREV_BLKP(bp));
        delete_node(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    /* 将合并好的free块加入到空闲链接表中 */
    insert_node(bp, size);
    return bp;
}

static void *place(void *bp, size_t size) {
    size_t bp_size = GET_SIZE(HDRP(bp));
    /* allocate size大小的空间后剩余的大小 */
    size_t remainder = bp_size - size;

    delete_node(bp);

    /* 如果剩余的大小小于最小块，则不分离原块 */
    if (remainder < DSIZE * 2) {
        PUT(HDRP(bp), PACK(bp_size, 1));
        PUT(FTRP(bp), PACK(bp_size, 1));
    }

    /* 否则分离原块，但这里要注意这样一种情况（在binary-bal.rep和binary2-bal.rep有体现）： 
    *  如果每次allocate的块大小按照小、大、小、大的连续顺序来的话，我们的free块将会被“拆”成以下这种结构：
    *  其中s代表小的块，B代表大的块

 s      B      s       B     s      B      s     B
+--+----------+--+----------+-+-----------+-+---------+
|  |          |  |          | |           | |         |
|  |          |  |          | |           | |         |
|  |          |  |          | |           | |         |
+--+----------+--+----------+-+-----------+-+---------+

    *  这样看起来没什么问题，但是如果程序后来free的时候不是按照”小、大、小、大“的顺序来释放的话就会出现“external fragmentation”
    *  例如当程序将大的块全部释放了，但小的块依旧是allocated：

 s             s             s             s
+--+----------+--+----------+-+-----------+-+---------+
|  |          |  |          | |           | |         |
|  |   Free   |  |   Free   | |   Free    | |   Free  |
|  |          |  |          | |           | |         |
+--+----------+--+----------+-+-----------+-+---------+

    *  这样即使我们有很多free的大块可以使用，但是由于他们不是连续的，我们不能将它们合并，如果下一次来了一个大小为B+1的allocate请求
    *  我们就还需要重新去找一块Free块
    *  与此相反，如果我们根据allocate块的大小将小的块放在连续的地方，将达到开放在连续的地方：

 s  s  s  s  s  s      B            B           B
+--+--+--+--+--+--+----------+------------+-----------+
|  |  |  |  |  |  |          |            |           |
|  |  |  |  |  |  |          |            |           |
|  |  |  |  |  |  |          |            |           |
+--+--+--+--+--+--+----------+------------+-----------+

    *  这样即使程序连续释放s或者B，我们也能够合并free块，不会产生external fragmentation
    *  这里“大小”相对判断是根据binary-bal.rep和binary2-bal.rep这两个文件设置的，我这里在96附近能够达到最优值
    *  
    */
    else if (size >= 96) {
        PUT(HDRP(bp), PACK(remainder, 0));
        PUT(FTRP(bp), PACK(remainder, 0));
        PUT(HDRP(NEXT_BLKP(bp)), PACK(size, 1));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 1));
        insert_node(bp, remainder);
        return NEXT_BLKP(bp);
    } else {
        PUT(HDRP(bp), PACK(size, 1));
        PUT(FTRP(bp), PACK(size, 1));
        PUT(HDRP(NEXT_BLKP(bp)), PACK(remainder, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(remainder, 0));
        insert_node(NEXT_BLKP(bp), remainder);
    }
    return bp;

}

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void) {   
    int i;
    char *heap; 

    /* 初始化分离空闲链表 */
    for (i = 0; i < LISTMAX; i++) {
        segregated_free_lists[i] = NULL;
    }

    /* 初始化堆 申请4字空间*/
    if ((long)(heap = mem_sbrk(4 * WSIZE)) == -1)
        return -1;
    
    /* 堆的起始和结束结构 */
    PUT(heap, 0);                               //填充块
    PUT(heap + (1 * WSIZE), PACK(DSIZE, 1));    //序言块 head
    PUT(heap + (2 * WSIZE), PACK(DSIZE, 1));    //序言块 foot
    PUT(heap + (3 * WSIZE), PACK(0, 1));        //结尾块

    // /* 扩展堆 放到free_list上*/
    if (extend_heap(INITCHUNKSIZE) == NULL)
        return -1;
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
// void *mm_malloc(size_t size)
// {
//     int newsize = ALIGN(size + SIZE_T_SIZE);
//     void *p = mem_sbrk(newsize);
//     if (p == (void *)-1)
// 	    return NULL;
//     else {
//         *(size_t *)p = size;
//         return (void *)((char *)p + SIZE_T_SIZE);
//     }
// }

void *mm_malloc(size_t size) {

    //至少16字节, 8字节对齐
    if (size == 0)
        return NULL;
    /* 内存对齐 */
    if (size <= DSIZE) {
        size = 2 * DSIZE;
    } else {
        size = ALIGN(size + DSIZE);
    }

    int i = 0;
    size_t searchsize = size;
    void *bp = NULL;

    while (i < LISTMAX) {
        /* 寻找对应链 */
        if (((searchsize <= 1) && (segregated_free_lists[i] != NULL))) {
            bp = segregated_free_lists[i];
            /* 在该链寻找大小合适的free块 */
            while ((bp != NULL) && ((size > GET_SIZE(HDRP(bp))))) {
                bp = PRED(bp);
            }
            /* 找到对应的free块 */
            if (bp != NULL)
                break;
        }
        searchsize >>= 1;
        i++;
    }

    /* 没有找到合适的free块，扩展堆 */
    if (bp == NULL) {
        if ((bp = extend_heap(MAX(size, CHUNKSIZE))) == NULL)
            return NULL;
    }

    /* 在free块中allocate size大小的块 */
    bp = place(bp, size);

    return bp;
}


/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp) {

    size_t size = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));

    /* 插入分离空闲链表 */
    insert_node(bp, size);
    /* 注意合并 */
    coalesce(bp);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
// void *mm_realloc(void *bp, size_t size)
// {
//     void *oldbp = bp;
//     void *newbp;
//     size_t copySize;
    
//     newbp = mm_malloc(size);
//     if (newbp == NULL)
//       return NULL;
//     copySize = *(size_t *)((char *)oldbp - SIZE_T_SIZE);
//     if (size < copySize)
//       copySize = size;
//     memcpy(newbp, oldbp, copySize);
//     mm_free(oldbp);
//     return newbp;
// }


void *mm_realloc(void *bp, size_t size)
{
    void *new_block = bp;
    int remainder;

    if (size == 0)
        return NULL;

    /* 内存对齐 */
    if (size <= DSIZE) {
        size = 2 * DSIZE;
    } else {
        size = ALIGN(size + DSIZE);
    }

    /* 如果size小于原来块的大小，直接返回原来的块 */
    if ((remainder = GET_SIZE(HDRP(bp)) - size) >= 0) {
        return bp;
    } else if (!GET_ALLO(HDRP(NEXT_BLKP(bp))) || !GET_SIZE(HDRP(NEXT_BLKP(bp)))) {
        /* 否则先检查地址连续下一个块是否为free块或者该块是堆的结束块，因为我们要尽可能利用相邻的free块，
        以此减小“external fragmentation” */
        /* 即使加上后面连续地址上的free块空间也不够，需要扩展块 */
        if ((remainder = GET_SIZE(HDRP(bp)) + GET_SIZE(HDRP(NEXT_BLKP(bp))) - size) < 0) {
            if (extend_heap(MAX(-remainder, CHUNKSIZE)) == NULL)
                return NULL;
            remainder += MAX(-remainder, CHUNKSIZE);
        }
        /* 删除刚刚利用的free块并设置新块的头尾 */
        delete_node(NEXT_BLKP(bp));
        PUT(HDRP(bp), PACK(size + remainder, 1));
        PUT(FTRP(bp), PACK(size + remainder, 1));
        
    } else {
        /* 没有可以利用的连续free块，而且size大于原来的块，这时只能申请新的不连续的free块、复制原块内容、释放原块 */
        new_block = mm_malloc(size);
        memmove(new_block, bp, GET_SIZE(HDRP(bp)));
        mm_free(bp);
    }

    return new_block;
}











