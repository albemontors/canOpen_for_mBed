#ifndef _JACAN_H
#define _JACAN_H

#include "mbed.h"
#include <cstdint>

#define NODE_NUMBER 16

#define RD PB_8
#define TD PB_9
#define FREQUENCY 1000000 //in Hz

#define SYNC 0x01

#define TPDO1 0x03
#define RPDO1 0x04
#define TPDO2 0x05
#define RPDO2 0x06
#define TPDO3 0x07
#define RPDO3 0x08
#define TPDO4 0x09
#define RPDO4 0x0A

#define PDO1 0x03
#define PDO2 0x05
#define PDO3 0x07
#define PDO4 0x09

typedef struct {
    union{
        uint64_t u64;
        uint32_t u32[2];
        uint16_t u16[4];
        uint8_t u8[8];
    };
} PDO_Data;

typedef struct {
    uint16_t id : 7;
    uint16_t code : 4;
    uint16_t reserved : 5;
} CAN_Id_Breakdown;

typedef struct {
    union {
        uint16_t raw;
        CAN_Id_Breakdown bd;
    };
} CAN_Id;

typedef struct{
    uint8_t code;
    PDO_Data* data;
} PDO_Entry;

typedef struct {   
    uint8_t id;
    PDO_Entry pdo[8];
} PDO_Dictionary_Entry;

typedef struct {
    int16_t deviceId;
    int16_t PDOId;
} Dictionary_Id;

void can_init();
void can_setup(PDO_Dictionary_Entry* PDO_Dictionary_init, bool isMaster_init);
/**
 * Function to allocate a frame in memory
 * @return pointer to the allocated frames
 * @return 0 if send buffer is full
 * @note after editing the frame invoke can_send(CANMessage*);
 */
CANMessage* can_allocate();
/**
 * Function to send a frame
 * @param msg pointer to the frame to be sent
 */
void can_send(CANMessage* msg);
void can_irq();
void can_dumper();
void can_sorter();
void can_sender();
//void nmtHandler(CANMessage* inputMsg);
//void syncHandler(CANMessage* inputMsg);
//void emcyHandler(CANMessage* inputMsg);
//void timeHandler(CANMessage* inputMsg);
void pdoHandler(CANMessage* inputMsg);
//void sdoHandler(CANMessage* inputMsg);
//void hbHandler(CANMessage* inputMsg);

/**
 * Function to send PDO packets from dictionary entries
 * @param id Packet id [TPDO1, RPDO1, TPDO2...]
 * @note Requested pdo needs to be set in dictionary
 * @note Only sends allowed types
 */
void pdoSender(CAN_Id id);
/**
 * Function to send PDO RTR packets
 * @param id Packet id [PDO1, PDO2, PDO3, PDO4]
 * @note sorts the pdo type automatically
 */
void pdoRequest(CAN_Id id);
Dictionary_Id dictionaryResolver(CAN_Id id);

#endif
