/*
 * gaia - opensource 3D interface to the planet
 * Copyright (C) 2006 gaia project
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "tiledb_internal.h"

const char *tiledb_strerror(int err) {
	switch (err) {
	case TILEDB_OK:
		return "No error";
	case TILEDB_MALLOC_FAILED:
		return "Malloc failed";
	case TILEDB_SMALL_BUFFER:
		return "Internal buffer is too small (may be bug in library)";
	case TILEDB_SYSCALL_ERROR:	/* see errno() */
		return "Syscall failed";
	case TILEDB_NOT_FOUND:
		return "Not found";
	default:
		return "Unknown error";
	}
}

int tiledb_getlasterrno(tiledb *handle) {
	return handle->lasterrno;
}
