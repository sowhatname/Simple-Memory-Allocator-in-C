#include <stdio.h>
#include <stdlib.h>

// 预先分配60KB内存池
#define MEMORY_SIZE 60 * 1024
// 内存块元数据
#define BLOCK_SIZE sizeof(MemoryBlock) //单位字节

// 内存块元数据（地址，大小，状态）
typedef struct MemoryBlock {
    size_t start_addr;          //起始地址
    size_t size;                //无符号整数类型 数据区大小（字节）
    int is_free;                //1为空闲 0为已分配
    struct MemoryBlock *next;   //后继节点
} MemoryBlock;

// 静态内存池
static unsigned char memory_pool[MEMORY_SIZE];

// 链表表头
MemoryBlock* head = NULL;

//初始化内存链表  640KB的空闲块
void initialize_memory() {
    head = (MemoryBlock*)memory_pool;
    head->size = MEMORY_SIZE - BLOCK_SIZE;
    head->start_addr =0;
    head->is_free = 1;
    head->next = NULL;
}

// 首次适应算法分配
MemoryBlock* allocate_first_fit(size_t size) {
    MemoryBlock* current = head;
    while(current) {
        if (current->is_free && current->size >= size + BLOCK_SIZE) {
            current->is_free = 0;
            return current;
        }
        current = current->next;
    }
    return NULL;
}

//最佳适应算法1（直接寻找满足需求大小的最小空闲块）
MemoryBlock* allocate_best_fit(size_t size) {
    MemoryBlock* current = head;
    MemoryBlock* best = NULL;
    while(current) {
        if(current->is_free && current->size >= size + BLOCK_SIZE) {
            if(best == NULL || current->size < best->size)
                best = current;
        }
        current = current->next;
    }
    return best;
}

//最坏适应算法1(直接寻找满足需求大小的最大空闲块)
MemoryBlock* allocate_worst_fit(size_t size) {
    MemoryBlock* current = head;
    MemoryBlock* worst = NULL;
    while(current) {
        if(current->is_free && current->size >= size + BLOCK_SIZE) {
            if(worst == NULL || current->size > worst->size)
                worst = current;
        }
        current = current->next;
    }
    return worst;
}

//分割空闲块
void split_MemoryBlock(MemoryBlock* target, size_t size) {
    
    MemoryBlock* new_block = (MemoryBlock*)((char*)target + BLOCK_SIZE + size);

    new_block->start_addr = target->start_addr + size;
    new_block->size = target->size - size - BLOCK_SIZE;
    new_block->is_free = 1;
    new_block->next = target->next;
    target->next = new_block;
    target->size = size;
}

//分配内存
int allocate_memory(size_t size, int algorithm) {
    if(size <= 0) {
        printf("❌ 错误：分配大小错误 \n");
        return -1;
    }
    if(size < 8) {
        size = 8;
        printf("分配需求过小，按8字节请求");
    }
    MemoryBlock* target = NULL;  //满足内存大小目标块
    switch (algorithm) {
    case 1:  //首次适应
        target = allocate_first_fit(size);
        break;
    case 2:  //最佳适应
        target = allocate_best_fit(size);
        break;
    case 3:  //最差适应
        target = allocate_worst_fit(size);
        break;
    default:
        printf("❌ 错误：未知算法类型 \n");
        break;
    }

    if(target == NULL) {  //没有满足大小的空闲区 分配失败
        printf("❌ 错误：内存不足，无法分配 %zu KB \n", size);
        return -1;
    }

    if(target->size - size - BLOCK_SIZE > BLOCK_SIZE) {
        split_MemoryBlock(target, size);
    }  

    target->is_free = 0;
    printf("✅成功分配 %zu KB，起始地址：%zu KB \n", size, (target->start_addr)/1024);
    return 0;
}

//回收内存并合并相邻块
void free_memory(size_t start_addr) {   //传入目标释放块地址
    MemoryBlock *current = head;
    MemoryBlock *prev = NULL;

    // 查找目标块
    while (current != NULL && current->start_addr != start_addr) {
        prev = current;
        current = current->next;
    }

    if (current == NULL) {
        printf("❌错误：未找到起始地址为 %zu KB 的内存块 \n", start_addr/1024);
        return;
    }

    if (current->is_free) {
        printf("❌错误：起始地址 %zu KB 的内存块已被释放 \n", start_addr/1024);
        return;
    }

    current->is_free = 1;
    printf("✅已释放起始地址 %zu KB 的内存块（大小：%zu KB）。\n", (current->start_addr)/1024, (current->size)/1024);


    // 合并前驱空闲块
    if (prev && prev->is_free) {
        prev->size = prev->size + current->size + BLOCK_SIZE;
        prev->next = current->next;
        current = prev;  // 让 current 指向合并后的块
    }

    // 合并后继空闲块
    MemoryBlock *next_block = current->next;
    if (next_block && next_block->is_free) {
        current->size = current->size + next_block->size + BLOCK_SIZE;
        current->next = next_block->next;
    }


}

void print_stats() {
    size_t total_free = 0;       // 空闲内存（仅用户可用部分）
    size_t total_used = 0;       // 已用内存（用户数据 + 元数据）
    size_t total_metadata = 0;   // 所有块的元数据总大小
    size_t total_available = 0;  // 用户可用的总内存（空闲 + 已分配）

    MemoryBlock* current = head;
    while (current) {
        if (current->is_free) {
            total_free += current->size;
        } else {
            total_used += current->size;  // 用户已分配的部分
        }
        total_metadata += BLOCK_SIZE;  // 每个块都有元数据
        total_available += current->size;       // 用户可用的总内存（不含元数据）
        current = current->next;
    }

    printf("\n内存统计：\n");
    printf("✅ 空闲内存（用户可用）: %zu KB\n", total_free/1024);
    printf("🟥 已用内存（用户数据）: %zu KB\n", total_used/1024);
    printf("📊 元数据占用: %zu 字节\n", total_metadata);
    printf("🔍 总可用内存（用户）: %zu KB\n", total_available/1024);
    printf("💾 内存池总大小: %u KB\n", MEMORY_SIZE/1024);
}

//内存分区情况 可视化
void display_memory() {
    //条形图视图
    MemoryBlock *current = head;

    printf("\n内存分区图：\n");
    printf("地址\t大小\t状态\t图形表示\n");
    current = head;
    while (current) {
        printf("%zuKB\t%zu\t", (current->start_addr)/1024, (current->size)/1024);
        printf("%s\t", current->is_free ? "空闲" : "已分配");
        
        int display_length = current->size / 8;
        if (display_length > 20) display_length = 20;
        if (display_length < 1) display_length = 1;
        
        for (int i = 0; i < display_length; i++) {
            printf(current->is_free ? "🟩" : "🟥");
        }
        printf(" (%zuKB)\n", (current->size)/1024);
        current = current->next;
    }
    printf("\n");
    print_stats();
}

int main() {
    initialize_memory();
    int algorithm;

    while (1) {
        printf("\n操作选项：\n1. 分配内存\n2. 释放内存\n3. 查看内存分区情况\n4. 退出\n");
        int choice;
        scanf("%d", &choice);

        if(choice == 1) {
            printf("请选择分配算法：\n1. 首次适应\n2. 最佳适应\n3. 最坏适应\n");
            scanf("%d", &algorithm);
            if (algorithm < 1 || algorithm > 3) {
                printf("无效算法选项！\n");
                exit(1);
            }
            size_t size;
            printf("请输入要分配的内存大小（KB）：");
            scanf("%zu", &size);
            allocate_memory(size*1024, algorithm);
            display_memory();
        } else if(choice == 2) {
            size_t addr;
            printf("请输入要释放的内存块起始地址（KB）：");
            scanf("%zu", &addr);
            free_memory(addr*1024);
            display_memory();
        } else if(choice == 3) {
            display_memory();
        } else if(choice == 4) {
            break;
        } else {
            printf("无效选项 \n");
        }
    }

    return 0;
}