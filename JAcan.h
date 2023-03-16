#ifndef _JACAN_H
#define _JACAN_H

#include "mbed.h"
#include <cstdint>

#define NODE_NUMBER 16

#define RD PB_8
#define TD PB_9
#define FREQUENCY 1000000 //in Hz

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
    uint16_t code : 4;
    uint16_t id : 7;
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

void can_init();
void can_setup(PDO_Dictionary_Entry* PDO_Dictionary_init);
CANMessage* can_allocate();
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
void pdoSender(CAN_Id id);
void pdoRequest(CAN_Id id);

#endif
