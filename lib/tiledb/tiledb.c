#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "tiledb.h"
#include "tiledb_endian.h"
#include "tiledb_file_io.h"
#include "tiledb_file_locking.h"
#include "tiledb_internal.h"

#if LOG_DB
#define db_printf(...) printf(__VA_ARGS__)
#else
#define db_printf(...) while(0){}
#endif

size_t tiledb_get_data_size(DB_Handle* db_handle) {
	return db_handle->current_data ? db_handle->current_size : 0;
}

unsigned char *tiledb_get_data_ptr(DB_Handle* db_handle) {
	return db_handle->current_data;
}

DB_Handle *tiledb_open(char *filepath, int flags) {
	db_printf("tiledb_open(%s)\n", filepath);

	DB_Handle *db_handle = (DB_Handle *)malloc(sizeof(DB_Handle));
	memset(db_handle, 0, sizeof(DB_Handle));

	if (flags & CLEAR_CACHE) {
		db_printf("Clear cache\n");
		tiledb_create_new_cache(filepath);
	}

	int filepath_len = strlen(filepath);
	char db_path[filepath_len+4+1]; //add '.idx|.dat' and \0
	strcpy((char*)&db_path, filepath);

	strcpy(((char*)&db_path)+filepath_len, ".idx");
	db_handle->index_file = open((char*)&db_path, O_RDWR | O_SYNC);
	if (db_handle->index_file == -1) {
		if (flags & CREATE_IF_NOT_EXISTS) {
			db_printf("Create cache\n");
			tiledb_create_new_cache(filepath);
		}
		db_handle->index_file = open((char*)&db_path, O_RDWR | O_SYNC);
		if (db_handle->index_file == -1) {
			db_error("on open index");
			exit(-1);
		}
	}

	strcpy(((char*)&db_path)+filepath_len, ".dat");
	db_handle->data_file = open((char*)&db_path, O_RDWR | O_SYNC);
	if (db_handle->data_file == -1) {
		db_error("on open data");
		exit(-1);
	}

	if (tiledb_read_settings(db_handle) != TILEDB_OK) {
		tiledb_close(db_handle);
		return NULL;
	}

	if (db_handle->version == 0) {
		db_handle->index_page_size = sizeof(tiledb_index_entry_v0);
		db_handle->data_page_size = 512;
	} else {
		tiledb_close(db_handle);
		return NULL;
	}

	return db_handle;
}

void tiledb_close(DB_Handle* db_handle) {
	if (close(db_handle->index_file) == -1) {
		db_error("on close index");
		exit(-1);
	}

	if (close(db_handle->data_file) == -1) {
		db_error("on close data");
		exit(-1);
	}

	if (db_handle->current_data) {
		free(db_handle->current_data);
	}

	free(db_handle);
}

void tiledb_create_new_cache(char *filepath)
{
	DB_Handle handle;

	int filepath_len = strlen(filepath);
	char db_path[filepath_len+4+1]; //add '.idx|.dat' and \0
	strcpy((char*)&db_path, filepath);

	strcpy(((char*)&db_path)+filepath_len, ".idx");
	handle.index_file = open((char*)&db_path, O_WRONLY | O_CREAT | O_TRUNC | O_SYNC, 0644);
	if (handle.index_file == -1) {
		db_error("on creating new index file");
		exit(-1);
	}

	strcpy(((char*)&db_path)+filepath_len, ".dat");
	handle.data_file = open((char*)&db_path, O_WRONLY | O_CREAT | O_TRUNC | O_SYNC, 0644);
	if (handle.index_file == -1) {
		db_error("on creating new index file");
		exit(-1);
	}

	tiledb_create_new_db_v0(&handle);

	if (close(handle.index_file) == -1) {
		db_error("on close index");
		exit(-1);
	}
	if (close(handle.data_file) == -1) {
		db_error("on close data");
		exit(-1);
	}
}

tiledb_error tiledb_put(DB_Handle* db_handle, uint32_t x, uint32_t y, uint32_t level, char* data, size_t size)
{
	db_printf("tiledb_put(x=%d, y=%d, level=%d, size=%d)\n", x, y, level, size);

	if (db_handle->version == 0) {
		return tiledb_put_v0(db_handle, x, y, level, data, size);
	} else {
		return TILEDB_UNSUPPORTED_DB_VERSION;
	}
}

tiledb_error tiledb_get(DB_Handle* db_handle, uint32_t x, uint32_t y, uint32_t level)
{
	db_printf("tiledb_get(x=%d, y=%d, level=%d)\n", x, y, level);

	if (db_handle->version == 0) {
		return tiledb_get_v0(db_handle, x, y, level);
	} else {
		return TILEDB_UNSUPPORTED_DB_VERSION;
	}
}

tiledb_error tiledb_remove(DB_Handle* db_handle, unsigned int x, unsigned int y, unsigned int level) {
	db_printf("tiledb_remove(x=%d, y=%d, level=%d)\n", x, y, level);

	if (db_handle->version == 0) {
		return tiledb_remove_v0(db_handle, x, y, level);
	} else {
		return TILEDB_UNSUPPORTED_DB_VERSION;
	}
}

tiledb_error tiledb_enable_lazylock(DB_Handle* db_handle) {
	db_printf("tiledb_enable_lazylock\n");
	//lock index page 0, put/get of other processes will block
	acquire_index_page_lock(db_handle, 0);
	//other processes, could have locked other pages in the index_file
	//acquire lock on hole file
	size_t size = tiledb_get_file_size(db_handle->index_file);
	acquire_file_lock(db_handle->index_file, 0, size);

	db_handle->lazy_locking = 1;
	return TILEDB_OK;
}

tiledb_error tiledb_disable_lazylock(DB_Handle* db_handle) {
	db_printf("tiledb_disable_lazylock\n");

	db_handle->lazy_locking = 0;

	//release hole file lock
	size_t size = tiledb_get_file_size(db_handle->index_file);
	acquire_file_lock(db_handle->index_file, 0, size);
	//release index lock
	release_index_page_lock(db_handle, 0);

	return TILEDB_OK;
}

tiledb_error tiledb_defragment_data_file(DB_Handle* db_handle) {
	//TODO implement defragment
	// 	lock_complete_database();
	// 	data_entry = 0;
	// 	next_data_entry = 0;
	// 	while (next_data_entry < filesize(db_handle->data_file)) {
	// 		if (next_data_entry->mtime == 0 || (next_data_entry->index_page->objects[next_data_entry->index].offset != next_data_entry) {
	// 			//data_entry is garbage
	// 			next_data_entry = search_next_non_garbage_entry_starting_with(next_data_entry);
	// 			filecpy(data_entry, next_data_entry); //entry must not overlap
	// 			//update index
	// 			data_entry->index_page->objects[data_entry->index].offset = data_entry;
	// 			store_index();
	// 			next_data_entry += data_entry.size + padding;
	// 		}
	// 		data_entry += data_entry.size + padding;
	// 	}
	// 	delete_file_space_after(data_entry)
	// 	unlock complete database
	return TILEDB_OK;
}