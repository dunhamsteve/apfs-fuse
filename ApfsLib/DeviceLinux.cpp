/*
This file is part of apfs-fuse, a read-only implementation of APFS
(Apple File System) for FUSE.
Copyright (C) 2017 Simon Gander

Apfs-fuse is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Apfs-fuse is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with apfs-fuse.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#if !defined(__APPLE__)
#include <linux/fs.h>
#endif
#include <stdio.h>

#include "DeviceLinux.h"
#include "Global.h"

#if defined(__APPLE__)
#include <sys/disk.h>

#define fstat64 fstat
#define stat64 stat
#define pread64 pread
#define O_LARGEFILE 0
#endif

DeviceLinux::DeviceLinux()
{
	m_device = -1;
	m_size = 0;
}

DeviceLinux::~DeviceLinux()
{
	Close();
}

bool DeviceLinux::Open(const char* name)
{
	m_device = open(name, O_RDONLY | O_LARGEFILE);

	if (m_device == -1)
		return false;

	struct stat64 st;

	fstat64(m_device, &st);

	if (S_ISREG(st.st_mode))
	{
		m_size = st.st_size;
	}
	else if (S_ISBLK(st.st_mode))
	{
#if defined(__APPLE__)
		uint32_t block_size;
		uint64_t block_count;
		ioctl(m_device, DKIOCGETBLOCKSIZE, &block_size);
		ioctl(m_device, DKIOCGETBLOCKCOUNT, &block_count);

        m_size = block_count*block_size;
#else		
		// Hmmm ...
		ioctl(m_device, BLKGETSIZE64, &m_size);
#endif
	}

	if (g_debug > 0)
		printf("Device %s opened. Size is %zu bytes.\n", name, m_size);

	return m_device != -1;
}

void DeviceLinux::Close()
{
	if (m_device != -1)
		close(m_device);
	m_device = -1;
	m_size = 0;
}

bool DeviceLinux::Read(void* data, uint64_t offs, uint64_t len)
{
	size_t nread;

	nread = pread64(m_device, data, len, offs);

	// TODO: Bessere Fehlerbehandlung ...
	return nread == len;
}
