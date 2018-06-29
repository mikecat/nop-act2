#include "memory.h"

#define PAGE_SIZE 0x1000

#define BUFFER_START 0x00800000

struct memory_region_t {
	struct memory_region_t* next;
	unsigned int start_address, size;
};

static struct memory_region_t *free_regions, *occupied_regions, *region_pool;

static unsigned int allocated_size, pooled_num;

static struct memory_region_t* region_new(void) {
	if (region_pool == 0) {
		/* no region available in the recycle pool */
		if (sizeof(struct memory_region_t) * (pooled_num + 1) > allocated_size) {
			/* no space for region, allocate more */
			if (free_regions == 0 || free_regions->start_address != BUFFER_START + allocated_size ||
			free_regions->size < PAGE_SIZE) {
				/* no free space at the beginning of buffer */
				return 0;
			}
			free_regions->start_address += PAGE_SIZE;
			free_regions->size -= PAGE_SIZE;
			allocated_size += PAGE_SIZE;
			if (free_regions->size == 0) {
				/* removed region that became empty and recycle */
				struct memory_region_t* to_drop = free_regions;
				free_regions = free_regions->next;
				return to_drop;
			}
		}
		return &((struct memory_region_t*)BUFFER_START)[pooled_num++];
	} else {
		struct memory_region_t* ret = region_pool;
		region_pool = region_pool-> next;
		return ret;
	}
}

void memory_init(void) {
	unsigned int memory_limit = 0x01000000;
	occupied_regions = region_pool = 0;
	allocated_size = PAGE_SIZE;
	pooled_num = 1;
	free_regions = (struct memory_region_t*)BUFFER_START;
	free_regions->next = 0;
	free_regions->start_address = BUFFER_START + PAGE_SIZE;
	free_regions->size = memory_limit - BUFFER_START - PAGE_SIZE;
}

void* memory_allocate(unsigned int size) {
	struct memory_region_t **itr = &free_regions, **last_valid = 0;
	struct memory_region_t *new_region;
	while (*itr != 0) {
		if ((*itr)->size >= size) last_valid = itr;
		itr = &(*itr)->next;
	}
	if (last_valid == 0) {
		/* no enough region available */
		return 0;
	}
	new_region = region_new();
	if (new_region == 0) {
		/* failed to allocate a region */
		return 0;
	}
	new_region->start_address = (*last_valid)->start_address + (*last_valid)->size - size;
	(*last_valid)->size -= size;
	if ((*last_valid)->size == 0) {
		/* this region became empty, recycle */
		struct memory_region_t *to_remove = *last_valid;
		*last_valid = (*last_valid)->next;
		to_remove->next = region_pool;
		region_pool = to_remove;
	}
	new_region->size = size;
	new_region->next = occupied_regions;
	occupied_regions = new_region;
	return (void*)new_region->start_address;
}

void memory_free(void* addr) {
	struct memory_region_t **itr = &occupied_regions, *to_remove;
	unsigned int start_address, size;
	while (*itr != 0 && (void*)(*itr)->start_address != addr) {
		itr = &(*itr)->next;
	}
	if (*itr == 0) {
		/* no such region known */
		return;
	}
	start_address = (*itr)->start_address;
	size = (*itr)->size;
	/* remove the region from occupied region list */
	to_remove = *itr;
	*itr = (*itr)->next;
	to_remove->next = region_pool;
	region_pool = to_remove;
	/* return freed region to free region list */
	itr = &free_regions;
	for (;;) {
		if (*itr == 0 || start_address + size < (*itr)->start_address) {
			/* return to where touching with no other free regions */
			struct memory_region_t* new_region = region_new();
			new_region->next = *itr;
			new_region->start_address = start_address;
			new_region->size = size;
			*itr = new_region;
			break;
		} else if (start_address + size == (*itr)->start_address) {
			/* return to front of a region */
			(*itr)->start_address = start_address;
			(*itr)->size += size;
			break;
		} else if ((*itr)->start_address + (*itr)->size == start_address) {
			/* return to back of a regioon */
			(*itr)->size += size;
			if ((*itr)->next != 0 && (*itr)->start_address + (*itr)->size == (*itr)->next->start_address) {
				/* merge to the next region */
				to_remove = (*itr)->next;
				(*itr)->size += (*itr)->next->size;
				(*itr)->next = (*itr)->next->next;
				to_remove->next = region_pool;
				region_pool = to_remove;
			}
			break;
		} else {
			/* continue scanning */
			itr = &(*itr)->next;
		}
	}
}
