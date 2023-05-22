#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "udp.h"
#include "mfs.h"

MFS_CR_t *p_cr = NULL;
int fd = -1;

int get_imap(int inum, MFS_Imap_t* imap) {
  int imap_index = inum / IMAP_PIECE_SIZE;
  int imap_offset = p_cr->imap[imap_index];
  if (imap_offset == -1)
  {
    printf("get_inode - invalid inum: imap");
    return -1;
  }

  lseek(fd, imap_offset, SEEK_SET);
  read(fd, imap, sizeof(MFS_Imap_t));
  return 0;
}

int get_inode(int inum, MFS_Inode_t* inode) {
  int imap_index = inum / IMAP_PIECE_SIZE;
  int imap_offset = p_cr->imap[imap_index];
  if (imap_offset == -1)
  {
    printf("get_inode - invalid inum: imap");
    return -1;
  }

  MFS_Imap_t imap;
  lseek(fd, imap_offset, SEEK_SET);
  read(fd, &imap, sizeof(MFS_Imap_t));

  int inode_index = inum % IMAP_PIECE_SIZE;
  int inode_offset = imap.inodes[inode_index];
  if (inode_offset == -1)
  {
    printf("get_inode - invalid inum: inode");
    return -1;
  }

  lseek(fd, inode_offset, SEEK_SET);
  read(fd, inode, sizeof(MFS_Inode_t));
  return 0;
}

int server_read(int inum, char *buffer, int block)
{
  printf("server_read: inum: %d, buffer: %s, block: %d\n", inum, buffer, block);

  MFS_Inode_t inode;
  int retval = get_inode(inum, &inode);
  if (retval == -1) {
    return -1;
  }

  if (!(inode.type == MFS_REGULAR_FILE || inode.type == MFS_DIRECTORY))
  {
    printf("server_read: invalid inode type");
    return -1;
  }

  int block_offset = inode.data[block];
  lseek(fd, block_offset, SEEK_SET);
  read(fd, buffer, MFS_BLOCK_SIZE);

  return 0;
}

int server_stat(int inum, MFS_Stat_t *m)
{
  printf("server_stat: inum: %d\n", inum);

  MFS_Inode_t inode;
  int retval = get_inode(inum, &inode);
  if (retval == -1) {
    return -1;
  }

  m->size = inode.size;
  m->type = inode.type;
  return 0;
}

int server_write(int inum, char *buffer, int block)
{
  printf("server_write: inum: %d, buffer: %s, block: %d\n", inum, buffer, block);
  char write_block[MFS_BLOCK_SIZE];
  char *bp = buffer;
  int index = 0;
  while (index < MFS_BLOCK_SIZE) 
  {
    if (bp != NULL)
    {
      write_block[index] = *bp;
      bp += 1;
    }
    else if (bp == NULL)
    {
      write_block[index] = '\0';
    }
    index += 1;
  }

  int offset = p_cr->end_log;
  int imap_index = inum / IMAP_PIECE_SIZE;
  int imap_offset = p_cr->imap[imap_index];
  int old_imap_exists = 0;
  MFS_Imap_t imap;
  int inode_offset = -1;
  int inode_index = -1;
  if (imap_offset != -1)
  {
    old_imap_exists = 1;
    lseek(fd, imap_offset, SEEK_SET);
    read(fd, &imap, sizeof(MFS_Imap_t));
    inode_index = inum % IMAP_PIECE_SIZE;
    inode_offset = imap.inodes[inode_index];
  }

  int old_inode_exists = 0;
  int block_offset = -1;
  MFS_Inode_t inode;
  if (inode_offset != -1 && old_imap_exists)
  {
    old_inode_exists = 1;
    lseek(fd, inode_offset, SEEK_SET);
    read(fd, &inode, sizeof(MFS_Inode_t));
    if (inode.type != MFS_REGULAR_FILE)
    {
      printf("server_write: not a regular file_4");
      return -1;
    }

    block_offset = inode.data[block];
  }

  int old_block_exists = 0;
  if (block_offset != -1 && old_inode_exists && old_imap_exists)
  {
    old_block_exists = 1;
    offset = block_offset;
  }
  p_cr->end_log += MFS_BLOCK_SIZE;
  lseek(fd, offset, SEEK_SET);
  write(fd, write_block, MFS_BLOCK_SIZE);

  MFS_Inode_t new_inode;
  if (old_inode_exists == 1)
  {
    new_inode.size = (block + 1) * MFS_BLOCK_SIZE; //TODO: confirm this
    new_inode.type = inode.type;
    for (int i = 0; i < 14; i++)
      new_inode.data[i] = inode.data[i];
    new_inode.data[block] = offset;
  }
  else if (old_inode_exists == 0)
  {
    new_inode.size = 0; //TODO: confirm this
    new_inode.type = MFS_REGULAR_FILE;
    for (int i = 0; i < 14; i++)
      new_inode.data[i] = -1;
    new_inode.data[block] = offset;
  }

  offset = p_cr->end_log;
  p_cr->end_log += sizeof(MFS_Inode_t);
  lseek(fd, offset, SEEK_SET);
  write(fd, &new_inode, sizeof(MFS_Inode_t));

  MFS_Imap_t new_imap;
  if (old_imap_exists == 1)
  {
    for (int i = 0; i < IMAP_PIECE_SIZE; i++)
      new_imap.inodes[i] = imap.inodes[i];
    new_imap.inodes[inode_index] = offset;
  }
  else if (old_imap_exists == 0)
  {
    for (int i = 0; i < IMAP_PIECE_SIZE; i++)
      new_imap.inodes[i] = -1;
    new_imap.inodes[inode_index] = offset;
  }

  offset = p_cr->end_log;
  p_cr->end_log += sizeof(MFS_Imap_t);
  lseek(fd, offset, SEEK_SET);
  write(fd, &new_imap, sizeof(MFS_Imap_t));

  p_cr->imap[imap_index] = offset;
  lseek(fd, 0, SEEK_SET);
  write(fd, p_cr, sizeof(MFS_CR_t));

  fsync(fd);
  return 0;
}

int server_lookup(int pinum, char *name)
{
  printf("server_lookup: pinum: %d, name: %s\n", pinum, name);

  MFS_Inode_t inode;
  int retval = get_inode(pinum, &inode);
  if (retval == -1) {
    return -1;
  }

  if (inode.type != MFS_DIRECTORY)
  {
    printf("server_lookup: invalid pinum - not a directory");
    return -1;
  }

  char data_block[MFS_BLOCK_SIZE];
  for (int i = 0; i < NUM_INODE_POINTERS; i++)
  {
    int block_offset = inode.data[i];
    if (block_offset == -1)
      continue;

    lseek(fd, block_offset, SEEK_SET);
    read(fd, data_block, MFS_BLOCK_SIZE);

    MFS_DirDataBlock_t *dir_buf = (MFS_DirDataBlock_t *)data_block;
    for (int j = 0; j < NUM_DIR_ENTRIES; j++)
    {
      MFS_DirEnt_t *p_de = &dir_buf->entries[j];
      if (strcmp(p_de->name, name) == 0)
        return p_de->inum;
    }
  }
  return -1;
}

int server_creat(int pinum, int type, char *name)
{
  printf("server_creat: pinum: %d, type: %d, name: %s\n", pinum, type, name);
  int i = 0, j = 0;
  int offset = 0;

  if (server_lookup(pinum, name) != -1)
  {
    return 0;
  }

  int parent_imap_index = pinum / IMAP_PIECE_SIZE;
  int parent_imap_offset = p_cr->imap[parent_imap_index];
  if (parent_imap_offset == -1)
  {
    printf("server_creat: invalid pinum_2");
    return -1;
  }

  int inode_index = pinum % IMAP_PIECE_SIZE;
  MFS_Imap_t parent_imap;
  lseek(fd, parent_imap_offset, SEEK_SET);
  read(fd, &parent_imap, sizeof(MFS_Imap_t));
  int inode_offset = parent_imap.inodes[inode_index];
  if (inode_offset == -1)
  {
    printf("server_creat: invalid pinum_3");
    return -1;
  }

  MFS_Inode_t parent_inode;
  lseek(fd, inode_offset, SEEK_SET);
  read(fd, &parent_inode, sizeof(MFS_Inode_t));
  if (parent_inode.type != MFS_DIRECTORY)
  {
    printf("server_creat: invalid pinum_4");
    return -1;
  }

  int free_inum = -1;
  int is_free_inum_found = 0;
  for (i = 0; i < NUM_IMAP; i++)
  {

    int imap_offset = p_cr->imap[i];

    if (imap_offset != -1)
    {
      MFS_Imap_t parent_imap;
      lseek(fd, imap_offset, SEEK_SET);
      read(fd, &parent_imap, sizeof(MFS_Imap_t));
      for (j = 0; j < IMAP_PIECE_SIZE; j++)
      {
        inode_offset = parent_imap.inodes[j];
        if (inode_offset == -1)
        {
          free_inum = i * IMAP_PIECE_SIZE + j;
          is_free_inum_found = 1;
          break;
        }
      }
    }
    else
    {

      MFS_Imap_t new_imap;
      for (j = 0; j < IMAP_PIECE_SIZE; j++)
        new_imap.inodes[j] = -1;

      offset = p_cr->end_log;
      p_cr->end_log += sizeof(MFS_Imap_t);
      lseek(fd, offset, SEEK_SET);
      write(fd, &new_imap, sizeof(MFS_Imap_t));

      p_cr->imap[i] = offset;
      lseek(fd, 0, SEEK_SET);
      write(fd, p_cr, sizeof(MFS_CR_t));

      fsync(fd);

      for (j = 0; j < IMAP_PIECE_SIZE; j++)
      {
        inode_offset = new_imap.inodes[j];
        if (inode_offset == -1)
        {
          free_inum = i * IMAP_PIECE_SIZE + j;
          is_free_inum_found = 1;
          break;
        }
      }
    }

    if (is_free_inum_found)
      break;
  }

  if (free_inum == -1 || free_inum > TOTAL_NUM_INODES - 1)
  {
    printf("server_creat: cannot find free inode_5 ");
    return -1;
  }

  char data_buf[MFS_BLOCK_SIZE];
  MFS_DirDataBlock_t *dir_buf = NULL;
  int flag_found_entry = 0;
  int block_par = 0;
  MFS_Inode_t *p_nd_pr = &parent_inode;
  for (i = 0; i < NUM_INODE_POINTERS; i++)
  {
    int parent_block_offset = p_nd_pr->data[i];
    block_par = i;
    if (parent_block_offset == -1)
    {
      MFS_DirDataBlock_t *p_dir = (MFS_DirDataBlock_t *)data_buf;
      int index = 0;
      while(index < NUM_DIR_ENTRIES)
      {
        strcpy(p_dir->entries[index].name, "\0");
        p_dir->entries[index].inum = -1;
	index += 1;
      }
      
      offset = p_cr->end_log;
      p_cr->end_log += MFS_BLOCK_SIZE;
      lseek(fd, offset, SEEK_SET);
      write(fd, p_dir, sizeof(MFS_DirDataBlock_t));

      parent_block_offset = offset;

      MFS_Inode_t new_dir_inode;
      new_dir_inode.size = parent_inode.size; 
      new_dir_inode.type = MFS_DIRECTORY;
      for (i = 0; i < 14; i++)
        new_dir_inode.data[i] = parent_inode.data[i];
      new_dir_inode.data[block_par] = offset;
      p_nd_pr = &new_dir_inode;

      offset = p_cr->end_log;
      p_cr->end_log += sizeof(MFS_Inode_t);
      lseek(fd, offset, SEEK_SET);
      write(fd, &new_dir_inode, sizeof(MFS_Inode_t));

      MFS_Imap_t new_dir_imap;
      for (i = 0; i < IMAP_PIECE_SIZE; i++)
        new_dir_imap.inodes[i] = parent_imap.inodes[i];
      new_dir_imap.inodes[inode_index] = offset;

      offset = p_cr->end_log;
      p_cr->end_log += sizeof(MFS_Imap_t);
      lseek(fd, offset, SEEK_SET);
      write(fd, &new_dir_imap, sizeof(MFS_Imap_t));

      p_cr->imap[parent_imap_index] = offset;
      lseek(fd, 0, SEEK_SET);
      write(fd, p_cr, sizeof(MFS_CR_t));

      fsync(fd);
    }

    lseek(fd, parent_block_offset, SEEK_SET);
    read(fd, data_buf, MFS_BLOCK_SIZE);

    dir_buf = (MFS_DirDataBlock_t *)data_buf;
    for (j = 0; j < NUM_DIR_ENTRIES; j++)
    {
      MFS_DirEnt_t *p_de = &dir_buf->entries[j];
      if (p_de->inum == -1)
      {
        p_de->inum = free_inum;
        strcpy(p_de->name, name);
        flag_found_entry = 1;
        break;
      }
      else
      {
	continue;
      }
    }

    if (flag_found_entry)
      break;
  }

  if (!flag_found_entry)
    return -1;


  offset = p_cr->end_log;
  p_cr->end_log += MFS_BLOCK_SIZE;
  lseek(fd, offset, SEEK_SET);
  write(fd, dir_buf, sizeof(MFS_DirDataBlock_t));

  MFS_Inode_t new_parent_inode;
  new_parent_inode.size = p_nd_pr->size;
  new_parent_inode.type = MFS_DIRECTORY;
  for (i = 0; i < 14; i++)
    new_parent_inode.data[i] = p_nd_pr->data[i];
  new_parent_inode.data[block_par] = offset;

  offset = p_cr->end_log;
  p_cr->end_log += sizeof(MFS_Inode_t);
  lseek(fd, offset, SEEK_SET);
  write(fd, &new_parent_inode, sizeof(MFS_Inode_t));

  MFS_Imap_t mp_par_new;
  for (i = 0; i < IMAP_PIECE_SIZE; i++)
    mp_par_new.inodes[i] = parent_imap.inodes[i];
  mp_par_new.inodes[inode_index] = offset;

  offset = p_cr->end_log;
  p_cr->end_log += sizeof(MFS_Imap_t);
  lseek(fd, offset, SEEK_SET);
  write(fd, &mp_par_new, sizeof(MFS_Imap_t));

  p_cr->imap[parent_imap_index] = offset;
  lseek(fd, 0, SEEK_SET);
  write(fd, p_cr, sizeof(MFS_CR_t));

  fsync(fd);

  char *ip = NULL;
  char wr_buffer[MFS_BLOCK_SIZE];
  for (i = 0, ip = wr_buffer; i < MFS_BLOCK_SIZE; i++)
  {
    wr_buffer[i] = '\0';
  }

  int inum = free_inum;
  if (type == MFS_DIRECTORY)
  {
    MFS_DirDataBlock_t *p_dir = (MFS_DirDataBlock_t *)wr_buffer;
    for (i = 0; i < NUM_DIR_ENTRIES; i++)
    {
      strcpy(p_dir->entries[i].name, "\0");
      p_dir->entries[i].inum = -1;
    }
    strcpy(p_dir->entries[0].name, ".\0");
    p_dir->entries[0].inum = inum;
    strcpy(p_dir->entries[1].name, "..\0");
    p_dir->entries[1].inum = pinum;

    offset = p_cr->end_log;
    p_cr->end_log += MFS_BLOCK_SIZE;
    lseek(fd, offset, SEEK_SET);
    write(fd, wr_buffer, MFS_BLOCK_SIZE);
  }

  int imap_index = inum / IMAP_PIECE_SIZE;
  int imap_offset = p_cr->imap[imap_index];
  MFS_Imap_t imap;
  int is_old_imap = 0;
  if (imap_offset != -1)
  {
    is_old_imap = 1;
    inode_index = inum % IMAP_PIECE_SIZE;
    lseek(fd, imap_offset, SEEK_SET);
    read(fd, &imap, sizeof(MFS_Imap_t));
  }

  MFS_Inode_t nd_new;
  nd_new.size = 0; 
  nd_new.type = type; 
  for (i = 0; i < 14; i++)
    nd_new.data[i] = -1; 
  if (type == MFS_DIRECTORY)
    nd_new.data[0] = offset; 

  offset = p_cr->end_log;
  p_cr->end_log += sizeof(MFS_Inode_t);
  lseek(fd, offset, SEEK_SET);
  write(fd, &nd_new, sizeof(MFS_Inode_t));

  MFS_Imap_t new_imap;
  if (is_old_imap == 1)
  {
    for (i = 0; i < IMAP_PIECE_SIZE; i++)
      new_imap.inodes[i] = imap.inodes[i];
    new_imap.inodes[inode_index] = offset;
  }
  else if (is_old_imap == 0)
  {
    for (i = 0; i < IMAP_PIECE_SIZE; i++)
      new_imap.inodes[i] = -1;
    new_imap.inodes[inode_index] = offset;
  }

  offset = p_cr->end_log;
  p_cr->end_log += sizeof(MFS_Imap_t);
  lseek(fd, offset, SEEK_SET);
  write(fd, &new_imap, sizeof(MFS_Imap_t));

  p_cr->imap[imap_index] = offset;
  lseek(fd, 0, SEEK_SET);
  write(fd, p_cr, sizeof(MFS_CR_t));

  fsync(fd);

  printf("\nserver_creat: File created, name: %s, inum: %d\n", name, inum);
  return 0;
}

int server_unlink(int pinum, char *name)
{
  printf("server_unlink: pinum: %d, name: %s\n", pinum, name);

  int inum = server_lookup(pinum, name);
  if (inum == -1)
    return 0;

  int imap_index = inum / IMAP_PIECE_SIZE;
  if (p_cr->imap[imap_index] == -1)
  {
    printf("server_unlink: invalid inum 1");
    return -1;
  }
  int imap_offset = p_cr->imap[imap_index];
  MFS_Imap_t imap;
  lseek(fd, imap_offset, SEEK_SET);
  read(fd, &imap, sizeof(MFS_Imap_t));
  
  int inode_index = inum % IMAP_PIECE_SIZE;
  int inode_offset = imap.inodes[inode_index];
  if (inode_offset == -1)
  {
    printf("server_unlink: invalid inum 2");
    return -1;
  }

  MFS_Inode_t inode;
  lseek(fd, inode_offset, SEEK_SET);
  read(fd, &inode, sizeof(MFS_Inode_t));

  if (inode.type == MFS_DIRECTORY)
  {
    char data_buf[MFS_BLOCK_SIZE];

    for (int i = 0; i < NUM_INODE_POINTERS; i++)
    {
      int block_offset = inode.data[i];
      if (block_offset == -1)
        continue;

      lseek(fd, block_offset, SEEK_SET);
      read(fd, data_buf, MFS_BLOCK_SIZE);

      MFS_DirDataBlock_t *dir_buf = (MFS_DirDataBlock_t *)data_buf;
      for (int j = 0; j < NUM_DIR_ENTRIES; j++)
      {
        MFS_DirEnt_t *p_de = &dir_buf->entries[j];
        if (!(p_de->inum == pinum || p_de->inum == inum || p_de->inum == -1))
        {
          printf("server_unlink: directory is not empty");
          return -1;
        }
      }
    }
  }

  MFS_Imap_t new_imap;
  for (int i = 0; i < IMAP_PIECE_SIZE; i++)
    new_imap.inodes[i] = imap.inodes[i];
  new_imap.inodes[inode_index] = -1;

  int is_new_imap_empty = 1;
  for (int i = 0; i < IMAP_PIECE_SIZE; i++)
  {
    if (new_imap.inodes[i] != -1)
    {
      is_new_imap_empty = 0;
      break;
    }
    else
    {
      continue;
    }
  }

  if (is_new_imap_empty)
  {
    p_cr->imap[imap_index] = -1;
    lseek(fd, 0, SEEK_SET);
    write(fd, p_cr, sizeof(MFS_CR_t));

    fsync(fd);
  }
  else
  {
    int offset = p_cr->end_log;
    p_cr->end_log += sizeof(MFS_Imap_t);
    lseek(fd, offset, SEEK_SET);
    write(fd, &new_imap, sizeof(MFS_Imap_t));

    p_cr->imap[imap_index] = offset;
    lseek(fd, 0, SEEK_SET);
    write(fd, p_cr, sizeof(MFS_CR_t));

    fsync(fd);
  }

  int parent_imap_index = pinum / IMAP_PIECE_SIZE;
  int parent_imap_offset = p_cr->imap[parent_imap_index];
  if (parent_imap_offset == -1)
  {
    printf("server_unlink: invalid pinum 1");
    return -1;
  }

  MFS_Imap_t parent_imap;
  lseek(fd, parent_imap_offset, SEEK_SET);
  read(fd, &parent_imap, sizeof(MFS_Imap_t));
  int parent_inode_index = pinum % IMAP_PIECE_SIZE;
  int parent_inode_offset = parent_imap.inodes[parent_inode_index];
  if (parent_inode_offset == -1)
  {
    printf("server_unlink: invalid pinum 2");
    return -1;
  }

  MFS_Inode_t parent_inode;
  lseek(fd, parent_inode_offset, SEEK_SET);
  read(fd, &parent_inode, sizeof(MFS_Inode_t));
  if (parent_inode.type != MFS_DIRECTORY)
  {
    printf("server_unlink: invalid pinum 3");
    return -1;
  }

  char data_buf[MFS_BLOCK_SIZE];
  MFS_DirDataBlock_t *dir_buf = NULL;
  int found_dir_entry = 0;
  int parent_block_index = 0;
  for (int i = 0; i < NUM_INODE_POINTERS; i++)
  {

    int parent_block_offset = parent_inode.data[i];
    if (parent_block_offset == -1)
      continue;
    parent_block_index = i;
    lseek(fd, parent_block_offset, SEEK_SET);
    read(fd, data_buf, MFS_BLOCK_SIZE);

    dir_buf = (MFS_DirDataBlock_t *)data_buf;
    for (int j = 0; j < NUM_DIR_ENTRIES; j++)
    {
      MFS_DirEnt_t *p_de = &dir_buf->entries[j];
      if (p_de->inum == inum)
      {
        p_de->inum = -1;
        strcpy(p_de->name, "\0");
        found_dir_entry = 1;
        break;
      }
    }

    if (found_dir_entry)
      break;
  }

  if (!found_dir_entry)
    return 0;

  int offset = p_cr->end_log;
  p_cr->end_log += MFS_BLOCK_SIZE;
  lseek(fd, offset, SEEK_SET);
  write(fd, dir_buf, sizeof(MFS_DirDataBlock_t));

  MFS_Inode_t new_parent_inode;
  new_parent_inode.size = parent_inode.size - MFS_BLOCK_SIZE > 0 ? parent_inode.size - MFS_BLOCK_SIZE : 0;

  new_parent_inode.type = MFS_DIRECTORY;
  for (int i = 0; i < 14; i++)
    new_parent_inode.data[i] = parent_inode.data[i];
  new_parent_inode.data[parent_block_index] = offset;

  offset = p_cr->end_log;
  p_cr->end_log += sizeof(MFS_Inode_t);
  lseek(fd, offset, SEEK_SET);
  write(fd, &new_parent_inode, sizeof(MFS_Inode_t));

  MFS_Imap_t new_parent_imap;
  for (int i = 0; i < IMAP_PIECE_SIZE; i++)
    new_parent_imap.inodes[i] = parent_imap.inodes[i];
  new_parent_imap.inodes[parent_inode_index] = offset;

  offset = p_cr->end_log;
  p_cr->end_log += sizeof(MFS_Imap_t);
  lseek(fd, offset, SEEK_SET);
  write(fd, &new_parent_imap, sizeof(MFS_Imap_t));

  p_cr->imap[parent_imap_index] = offset;
  lseek(fd, 0, SEEK_SET);
  write(fd, p_cr, sizeof(MFS_CR_t));

  fsync(fd);

  return 0;
}

void server_shutdown()
{
  printf("server_shutdown");
  fsync(fd);
  exit(0);
}

void create_root(MFS_DirDataBlock_t root_block) {
 for (int i = 0; i < NUM_DIR_ENTRIES; i++)
     {
       strcpy(root_block.entries[i].name, "\0");
       root_block.entries[i].inum = -1;
     }
     strcpy(root_block.entries[0].name, ".\0");
     root_block.entries[0].inum = 0;
     strcpy(root_block.entries[1].name, "..\0");
     root_block.entries[1].inum = 0;
}

void create_root_inode(MFS_Inode_t root_inode, int offset)
{
  root_inode.type = MFS_DIRECTORY;
  for (int i = 1; i < 14; i++)
    root_inode.data[i] = -1;
  root_inode.data[0] = offset;
  offset = p_cr->end_log;
  p_cr->end_log += sizeof(MFS_Inode_t);
  lseek(fd, offset, SEEK_SET);
  write(fd, &root_inode, sizeof(MFS_Inode_t));
}

void create_imap(MFS_Imap_t firstimap, int offset)
{
  for (int i = 1; i < IMAP_PIECE_SIZE; i++)
      firstimap.inodes[i] = -1;
  firstimap.inodes[0] = offset;

  offset = p_cr->end_log;
  p_cr->end_log += sizeof(MFS_Imap_t);
  lseek(fd, offset, SEEK_SET);
  write(fd, &firstimap, sizeof(MFS_Imap_t));
}


int main(int argc, char *argv[])
{

  if (argc < 3)
  {
    printf("server [portnum] [file-system-image]\n");
    return 1;
  }

  int port = atoi(argv[1]);
  char *file_system_image = argv[2];

  p_cr = (MFS_CR_t *)malloc(sizeof(MFS_CR_t));

  fd = open(file_system_image, O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if (fd == -1)
  {
    printf("file does not exist\n");
    fd = open(file_system_image, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd == -1)
    {
      printf("failed to create file system image\n");
      return 1;
    }

    p_cr->end_log = sizeof(MFS_CR_t);
    for (int i = 0; i < NUM_IMAP; i++)
      p_cr->imap[i] = -1;

    lseek(fd, 0, SEEK_SET);
    write(fd, p_cr, sizeof(MFS_CR_t));

    // create root directory data block
    MFS_DirDataBlock_t root_block;
    for (int i = 0; i < NUM_DIR_ENTRIES; i++)
    {
      strcpy(root_block.entries[i].name, "\0");
      root_block.entries[i].inum = -1;
    }
    strcpy(root_block.entries[0].name, ".\0");
    root_block.entries[0].inum = 0;
    strcpy(root_block.entries[1].name, "..\0");
    root_block.entries[1].inum = 0;

    int offset = p_cr->end_log;
    p_cr->end_log += sizeof(MFS_DirDataBlock_t);
    lseek(fd, offset, SEEK_SET);
    write(fd, &root_block, sizeof(MFS_DirDataBlock_t));

    // create root inode block
    MFS_Inode_t root_inode;
    root_inode.type = MFS_DIRECTORY;
    for (int i = 1; i < 14; i++)
      root_inode.data[i] = -1;
    root_inode.data[0] = offset;

    offset = p_cr->end_log;
    p_cr->end_log += sizeof(MFS_Inode_t);
    lseek(fd, offset, SEEK_SET);
    write(fd, &root_inode, sizeof(MFS_Inode_t));

    // create first inode map
    MFS_Imap_t firstimap;
    for (int i = 1; i < IMAP_PIECE_SIZE; i++)
      firstimap.inodes[i] = -1;
    firstimap.inodes[0] = offset;

    offset = p_cr->end_log;
    p_cr->end_log += sizeof(MFS_Imap_t);
    lseek(fd, offset, SEEK_SET);
    write(fd, &firstimap, sizeof(MFS_Imap_t));

    //update critical region
    p_cr->imap[0] = offset;
    lseek(fd, 0, SEEK_SET);
    write(fd, p_cr, sizeof(MFS_CR_t));

    fsync(fd);
  }
  else
  {
    lseek(fd, 0, SEEK_SET);
    read(fd, p_cr, sizeof(MFS_CR_t));
  }

  int sd = UDP_Open(port);
  assert(sd > -1);
  struct sockaddr_in addr;
  MFS_Message_t req_pk, res_pk;
  while (1)
  {

    printf("server:: waiting...\n");
    int rc = UDP_Read(sd, &addr, (char *)&req_pk, sizeof(MFS_Message_t));
    printf("server:: read message [size:%d request_type:%d]\n", rc, req_pk.request);
    if (rc > 0)
    {
      if (req_pk.request == REQ_LOOKUP)
      {
        res_pk.inum = server_lookup(req_pk.inum, req_pk.name);
        res_pk.request = REQ_RESPONSE;
        UDP_Write(sd, &addr, (char *)&res_pk, sizeof(MFS_Message_t));
      }
      else if (req_pk.request == REQ_STAT)
      {
        res_pk.inum = server_stat(req_pk.inum, &(res_pk.stat));
        res_pk.request = REQ_RESPONSE;
        UDP_Write(sd, &addr, (char *)&res_pk, sizeof(MFS_Message_t));
      }
      else if (req_pk.request == REQ_WRITE)
      {
        res_pk.inum = server_write(req_pk.inum, req_pk.buffer, req_pk.block);
        res_pk.request = REQ_RESPONSE;
        UDP_Write(sd, &addr, (char *)&res_pk, sizeof(MFS_Message_t));
      }
      else if (req_pk.request == REQ_READ)
      {
        res_pk.inum = server_read(req_pk.inum, res_pk.buffer, req_pk.block);
        res_pk.request = REQ_RESPONSE;
        UDP_Write(sd, &addr, (char *)&res_pk, sizeof(MFS_Message_t));
      }
      else if (req_pk.request == REQ_CREAT)
      {
        res_pk.inum = server_creat(req_pk.inum, req_pk.type, req_pk.name);
        res_pk.request = REQ_RESPONSE;
        UDP_Write(sd, &addr, (char *)&res_pk, sizeof(MFS_Message_t));
      }
      else if (req_pk.request == REQ_UNLINK)
      {
        res_pk.inum = server_unlink(req_pk.inum, req_pk.name);
        res_pk.request = REQ_RESPONSE;
        UDP_Write(sd, &addr, (char *)&res_pk, sizeof(MFS_Message_t));
      }
      else if (req_pk.request == REQ_SHUTDOWN)
      {
        res_pk.request = REQ_RESPONSE;
        UDP_Write(sd, &addr, (char *)&res_pk, sizeof(MFS_Message_t));
        server_shutdown();
      }
      else if (req_pk.request == REQ_RESPONSE)
      {
        res_pk.request = REQ_RESPONSE;
        UDP_Write(sd, &addr, (char *)&res_pk, sizeof(MFS_Message_t));
      }
    }
  }
  return 0;
}
