#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "mdadm.h"
#include "jbod.h"

int mdadm_mount(void) {
  // Complete your code here
  int reply = jbod_operation(JBOD_MOUNT,NULL);  // Create the mount command. Since JBOD_MOUNT = 0, we don't have to shift bits.
  if (reply == 0){
    return 1;
  }

  return -1;
}

int mdadm_unmount(void) {
  uint32_t unmount = (0 | (uint8_t)JBOD_UNMOUNT << 12);   // Create the unmount command. We must shift 12 bits to go to the opcode. 
  int reply =  jbod_operation(unmount,NULL);
  if (reply == 0) {
    return 1;
  }
  return -1;
}

int mdadm_write_permission(void){
  uint32_t write_permission = (0 | (uint8_t)JBOD_WRITE_PERMISSION << 12);   // Create the unmount command. We must shift 12 bits to go to the opcode. 
  int reply =  jbod_operation(write_permission,NULL);
  if (reply == 0) {
    return 1;
  }
  return -1;
}


int mdadm_revoke_write_permission(void){

  uint32_t revoke_write_permission = (0 | (uint8_t)JBOD_REVOKE_WRITE_PERMISSION << 12);   // Create the unmount command. We must shift 12 bits to go to the opcode. 
  int reply =  jbod_operation(revoke_write_permission,NULL);
  if (reply == 0) {
    return 1;
  }
  return -1;
 
}


int mdadm_read(uint32_t start_addr, uint32_t read_len, uint8_t *read_buf)  {
  if (read_len > 1024) {                                                                       // Test case to see if read len is greater than 1024
    return -1;
  }
  if ((start_addr + read_len) > (JBOD_NUM_DISKS * JBOD_DISK_SIZE)) {                             // Test case to see if there is overflow in reading
    return -1;
    
  }
  if(!read_buf && read_len > 0) {                                                          // Test case to see if there is no read_buf specified
    return -1;
  } 
  
  if( jbod_operation(JBOD_ALREADY_UNMOUNTED << 12,NULL) == -1) {                           //Checks to see if JBOD is already mounted 
    return -1;
  }


  int bytes_read = 0; 
  int offset = (start_addr%JBOD_DISK_SIZE) % JBOD_BLOCK_SIZE;  

  while (bytes_read < read_len) {
    uint32_t currentDisk = (0 | (uint32_t)JBOD_SEEK_TO_DISK << 12 | (uint8_t) (start_addr/JBOD_DISK_SIZE));                                   // Seeks Disk based on start_addr
    uint32_t currentBlock = (0 | (uint32_t)JBOD_SEEK_TO_BLOCK << 12 | (uint8_t)((start_addr%JBOD_DISK_SIZE)/JBOD_BLOCK_SIZE) << 4);            // Seeks Block based on start_addr 
    uint8_t temp_Buf[256];
    if (jbod_operation(currentDisk, NULL) == -1) {                                                                                    //Runs Seek Disk command
      return -1;
    }
    if(jbod_operation(currentBlock, NULL) == -1) {                                                                                    //Runs Seek Block command
      return -1;
    }
    if (jbod_operation(0 | (uint8_t)JBOD_READ_BLOCK << 12, temp_Buf) == -1) {                                                          //Runs Read command into temp_Buf
      return -1;
    }

    int len_of_read = 0;
    
    if (read_len - bytes_read > JBOD_BLOCK_SIZE || offset != 0) {
      len_of_read = JBOD_BLOCK_SIZE-offset;
    }
    else {
      len_of_read = (read_len - bytes_read) ;
    }                                                                                                              

    memcpy((read_buf + bytes_read) , (temp_Buf+offset), len_of_read);                                                                                  // Copies over temp_Buf into read_buf with offset included
    offset = 0;

    // Variable setting
    bytes_read += len_of_read;                                                                                                // Increases offset by bytes in current read_buf
    start_addr += len_of_read;                                                                                                        // Adds length of read to start address
                                                       
  } 
  return read_len;  
}


int mdadm_write(uint32_t start_addr, uint32_t write_len, const uint8_t *write_buf) {
  if (write_len > 1024) {                                                                       // Test case to see if write len is greater than 1024
    return -1;
  }
  if (start_addr + write_len > JBOD_NUM_DISKS * JBOD_DISK_SIZE) {                             // Test case to see if there is overflow in reading
    return -1;
  }
  if(!write_buf && write_len > 0) {                                                          // Test case to see if there is no read_buf specified
    return -1;
  }
  if( jbod_operation(JBOD_ALREADY_UNMOUNTED << 12,NULL) == -1) {                           //Checks to see if JBOD is already mounted 
    return -1;
  }



  int bytes_wrote = 0;
  int offset = (start_addr%JBOD_DISK_SIZE) % JBOD_BLOCK_SIZE; 
  
  while (bytes_wrote < write_len) {
    uint8_t buffer[256];
    //Read current next 256 bytes into buffer
    mdadm_read(start_addr, JBOD_BLOCK_SIZE, buffer);

    // Find the current disk and block intiliazer
    uint32_t currentDisk = (0 | (uint32_t)JBOD_SEEK_TO_DISK << 12 | (uint8_t) (start_addr/JBOD_DISK_SIZE));                                   
    uint32_t currentBlock = (0 | (uint32_t)JBOD_SEEK_TO_BLOCK << 12 | (uint8_t)((start_addr%JBOD_DISK_SIZE)/JBOD_BLOCK_SIZE) << 4);            


    //Find current disk and block after read fucntion call
    if (jbod_operation(currentDisk, NULL) == -1) {                                                                                    
      return -1;
    }
    if(jbod_operation(currentBlock, NULL) == -1) {                                                                                   
      return -1;
    }

    //Find len of write based on size of block left and offset of first block
    int len_of_write = 0;
    
    if ((write_len - bytes_wrote) > JBOD_BLOCK_SIZE || offset != 0) {
      len_of_write = JBOD_BLOCK_SIZE-offset;
    }
    else {
      len_of_write = (write_len - bytes_wrote);
    }                                                                                                            

    memcpy((buffer + offset), (write_buf + bytes_wrote), len_of_write);                                                                                  
    offset = 0;


    if (jbod_operation(0 | (uint32_t)JBOD_WRITE_BLOCK << 12, buffer) == -1) {
      return -1;
    }

    // Variable setting
    bytes_wrote += len_of_write;                                                                                               
    start_addr += len_of_write;                                                                                                        
                                                     
  } 
	return write_len;
}