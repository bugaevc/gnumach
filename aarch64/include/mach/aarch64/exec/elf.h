/*
 * Copyright (c) 2023-2024 Free Software Foundation.
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

/* TODO: This should not be a public header.  */

#ifndef _MACH_AARCH64_EXEC_ELF_H_
#define _MACH_AARCH64_EXEC_ELF_H_

typedef unsigned int	Elf32_Addr;
typedef unsigned short	Elf32_Half;
typedef unsigned int	Elf32_Off;
typedef signed int	Elf32_Sword;
typedef unsigned int	Elf32_Word;

typedef uint64_t	Elf64_Addr;
typedef uint64_t	Elf64_Off;
typedef int32_t		Elf64_Shalf;
typedef int32_t		Elf64_Sword;
typedef uint32_t	Elf64_Word;
typedef int64_t		Elf64_Sxword;
typedef uint64_t	Elf64_Xword;
typedef uint16_t	Elf64_Half;


#define MY_ELF_CLASS	ELFCLASS64
#define MY_EI_DATA	ELFDATA2LSB
#define MY_E_MACHINE	EM_AARCH64

#endif /* _MACH_AARCH64_EXEC_ELF_H_ */
