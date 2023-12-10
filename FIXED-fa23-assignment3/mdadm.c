#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jbod.h"
#include "mdadm.h"

/*Use your mdadm code*/

int mdadm_mount(void) {
  // Complete your code here
  int reply = jbod_operation(JBOD_MOUNT,NULL);  //Create the mount command. Since JBOD_MOUNT = 0, we don't have to shift bits.
  if (reply == 0){
    return 1;
  }
  return -1;
}

int mdadm_unmount(void) {
  uint32_t unmount = (0 | (uint8_t)JBOD_UNMOUNT << 12);   //Create the unmount command. We must shift 12 bits to go to the opcode. 
  int reply =  jbod_operation(unmount,NULL);
  if (reply == 0) {
    return 1;
  }
   
  return -1;
}

int mdadm_write_permission(void){
  uint32_t write_permission = (0 | (uint8_t)JBOD_WRITE_PERMISSION << 12);   //Create the unmount command. We must shift 12 bits to go to the opcode. 
  int reply =  jbod_operation(write_permission,NULL);
  if (reply == 0) {
    return 1;
  }
  return -1;
}


int mdadm_revoke_write_permission(void){

  uint32_t revoke_write_permission = (0 | (uint8_t)JBOD_REVOKE_WRITE_PERMISSION << 12);   //Create the unmount command. We must shift 12 bits to go to the opcode. 
  int reply =  jbod_operation(revoke_write_permission,NULL);
  if (reply == 0) {
    return 1;
  }
  return -1;
}



void get_location(uint32_t start_addr, uint32_t *disk_id, uint32_t *block_id) {
  int d_id = start_addr / JBOD_DISK_SIZE;                                               //Disk index
  int b_id = (start_addr % JBOD_DISK_SIZE) / JBOD_BLOCK_SIZE;                           //Block index
  
  //Sets disk and block id pointers
  *disk_id = d_id;                                                            
  *block_id = b_id;
}




int mdadm_read(uint32_t start_addr, uint32_t read_len, uint8_t *read_buf) {
  
  //Read requirments precheck
  if (read_len > 1024) return -1;
  if(!read_buf && read_len > 0) return -1;
  if( jbod_operation(JBOD_ALREADY_UNMOUNTED << 12,NULL) == -1) return -1;

  //Gets location of start and end disk and block 
  uint32_t sdisk_id, sblock_id, srem_bytes; 
  uint32_t ldisk_id, lblock_id, lrem_bytes; 
  get_location(start_addr, &sdisk_id, &sblock_id);
  get_location(start_addr + read_len - 1, &ldisk_id, &lblock_id);

  // Gets exact byte locations in regards to the block of the start byte and last byte
  srem_bytes = (start_addr % JBOD_DISK_SIZE) % JBOD_BLOCK_SIZE; 
  lrem_bytes = ((start_addr + read_len - 1) % JBOD_DISK_SIZE) % JBOD_BLOCK_SIZE; 


  //Gets current disk and block id
  uint32_t current_disk, current_block;
  get_location(start_addr, &current_disk, &current_block);
  uint16_t offset = 0;


  while (current_disk <= ldisk_id) {                                                      //Loops through until disks are same
    uint32_t op = (0 | JBOD_SEEK_TO_DISK << 12) | (current_disk);
    jbod_operation(op, NULL);                                                         //Seeks disk

    int tester;
    if (current_disk == ldisk_id) {
      tester = lblock_id;
    }
    else {
      tester = JBOD_NUM_BLOCKS_PER_DISK-1;
    }
    while (current_block <= tester) {                                                //Checks block ID for specific disk
      uint8_t temp_buf[JBOD_BLOCK_SIZE];                                                                                //Cache is not enabled
        uint32_t op = (0 | JBOD_SEEK_TO_BLOCK << 12) | (current_block << 4);              
        jbod_operation(op, NULL);                                                                            
        op = (0 | JBOD_READ_BLOCK << 12);
        jbod_operation(op, temp_buf);
      
        //Follows above implemenataion, just without cache being enabled
        int start_copy = 0;               
        int end_copy = JBOD_BLOCK_SIZE - 1;
        if ((current_disk == sdisk_id) && (current_block == sblock_id)) start_copy = srem_bytes;
        if ((current_disk == ldisk_id) && (current_block == lblock_id)) end_copy = lrem_bytes;
        memcpy(read_buf+offset, &temp_buf[start_copy], (end_copy-start_copy+1));
        offset += end_copy-start_copy+1;
        current_block++;
      
    }
    current_block = 0;                  
    current_disk++;                     //Increments current disk by 1
  }
    return read_len; 
}


int mdadm_write(uint32_t start_addr, uint32_t write_len, const uint8_t *write_buf) {

  //Write requirments precheck
  if (write_len > 1024) return -1; 
  if(!write_buf && write_len > 0) return -1;
  if(jbod_operation(JBOD_ALREADY_UNMOUNTED << 12,NULL)) return -1;
  

  //Gets location of start and end disk and block 
  uint32_t sdisk_id, sblock_id, srem_bytes;
  uint32_t ldisk_id, lblock_id, lrem_bytes;
  get_location(start_addr, &sdisk_id, &sblock_id);
  get_location(start_addr + write_len - 1, &ldisk_id, &lblock_id);


  // Gets exact byte locations in regards to the block of the start byte and last byte
  srem_bytes = (start_addr % JBOD_DISK_SIZE) % JBOD_BLOCK_SIZE;
  lrem_bytes = ((start_addr + write_len - 1) % JBOD_DISK_SIZE) % JBOD_BLOCK_SIZE;

  //Gets current disk and block id
  uint32_t current_disk, current_block;
  get_location(start_addr, &current_disk, &current_block);
  uint16_t offset = 0;
  
  while (current_disk <= ldisk_id) {                                          //Loops through disk
    uint32_t op = (0 | JBOD_SEEK_TO_DISK << 12 | current_disk);
    jbod_operation(op, NULL);

    int tester;
    if (current_disk == ldisk_id) {
      tester = lblock_id;
    }
    else {
      tester = JBOD_NUM_BLOCKS_PER_DISK-1;
    }
    while (current_block <= tester) {                                   //Loops through blocks
      op = (0 | JBOD_SEEK_TO_BLOCK << 12 | current_block << 4);
      jbod_operation(op, NULL);

      uint8_t temp_buf[JBOD_BLOCK_SIZE];
      op = (0 | JBOD_READ_BLOCK << 12);
      jbod_operation(op, temp_buf);

      int start_backup = 0;                                             //Gets orginal block address from read
      int end_backup = JBOD_BLOCK_SIZE-1;                               //Gets orgianal block end address from read
      if ((current_disk == sdisk_id) && (current_block == sblock_id)) start_backup = srem_bytes;
      if ((current_disk == ldisk_id) && (current_block == lblock_id)) end_backup = lrem_bytes;
      memcpy(&temp_buf[start_backup], write_buf+offset, (end_backup-start_backup+1));              //Copies write_buf to temp_buf according to where startaddr is to where end address is

      op = (0 | JBOD_SEEK_TO_DISK << 12) | (current_disk);
      jbod_operation(op, NULL);

      uint32_t op = (0 | JBOD_SEEK_TO_BLOCK << 12) | (current_block << 4);
      jbod_operation(op, NULL);

      op = (0 |JBOD_WRITE_BLOCK << 12);
      jbod_operation(op, temp_buf);
      
      offset += end_backup-start_backup+1;                                           //Offset for write_buf
      current_block++;                                                               //Increments current block
    }
    current_block = 0;                    
    current_disk++;                                                                 ///Increments current disk
  }
  return write_len;
}