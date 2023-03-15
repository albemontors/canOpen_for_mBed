#include "JAcan.h"

Mutex lock;
Ticker timer;
Thread dumper;
Thread receiver;
Thread sender;
EventFlags dump;
Mail<CANMessage, 16> inboundBox;
Mail<CANMessage, 16> outboundBox;
interface::CAN can(RD, TD, FREQUENCY);

PDO_Dictionary_Entry PDO_Dictionary[NODE_NUMBER];
bool isMaster;

void can_setup(PDO_Dictionary_Entry* PDO_Dictionary_init){
// ====== PROGRAM INIT SECTION ========// nothing needs touching below here
    can.attach(can_irq);
    can.monitor(0);
    dumper.start(can_dumper);
    receiver.start(can_sorter);
    sender.start(can_sender);
    for(int i = 0; i < NODE_NUMBER; i++) PDO_Dictionary[i] = PDO_Dictionary_init[i];
}

CANMessage* can_allocate(){
    return outboundBox.try_alloc();
}

void can_send(CANMessage* msg){
    outboundBox.put(msg);
}

void pdoHandler(CANMessage* inputMsg){

    printf("Started PDO runtime! \n");
    PDO_Data data;
    if(!inputMsg->type){
        for(int i = 0; i < 8; i++) data.u8[i] = inputMsg->data[i];
        CAN_Id id;
        id.raw = inputMsg->id;
        for(int i = 0; i < NODE_NUMBER; i++)
            if(id.bd.id == PDO_Dictionary[i].id)
                for(int j = 0; j < 4; j++){
                    if(id.bd.code == PDO_Dictionary[i].pdo[j].code){
                        PDO_Dictionary[i].pdo[j].data->u64 = data.u64;
                        printf("Memory has been written! \n");
                        break;
                    }
                break;
                }
    } else {
        CAN_Id id;
        id.raw = inputMsg->id;
        for(int i = 0; i < NODE_NUMBER; i++)
            if(id.bd.id == PDO_Dictionary[i].id)
                for(int j = 0; j < 4; j++){
                    if(id.bd.code == PDO_Dictionary[i].pdo[j].code){
                        data.u64 = PDO_Dictionary[i].pdo[j].data->u64;
                        break;
                    }
                    break;
                }
        isMaster ? id.bd.code-- : id.bd.code++;
        inputMsg->id = id.raw;
        CANMessage* outputMsg = outboundBox.try_alloc_for(Kernel::wait_for_u32_forever);
        *outputMsg = *inputMsg;
        outputMsg->type = CANData;
        outboundBox.put(outputMsg);
    }

}

void can_dumper(){
    while(1){
        dump.wait_any(1);
        dump.clear(1);
        CANMessage* msg = inboundBox.try_alloc_for(Kernel::wait_for_u32_forever);
        if(msg) can.read(*msg);
        else printf("Mail alloc error! \n");
        inboundBox.put(msg);
    }
}

void can_sorter(){
    // INIT
    CANMessage* inputMsg;
    CANMessage* outputMsg;
    CAN_Id id;

    // CODE
    while(1){
        inputMsg = inboundBox.try_get_for(Kernel::wait_for_u32_forever);
        if(!inputMsg) printf("Mail read error! \n");

        id.raw = inputMsg->id;
        
        //code to sort the can packets
        switch(id.bd.code){
            case 0x00: // NMT
                ;//nmtHandler(inputMsg);
                break;
            case 0x01: // SYNC OR EMCY
                if(!id.bd.id) // SYNC EVENT
                    ;//syncHandler(inputMsg);
                else // EMCY EVENT
                    ;//emcyHandler(inputMsg);
                break;
            case 0x02: // TIME
                ;//timeHandler(inputMsg);
                break;
            case 0x03:
            case 0x04:
            case 0x05:
            case 0x06:
            case 0x07:
            case 0x08:
            case 0x09:
            case 0x0A:
                pdoHandler(inputMsg);
                break;
            case 0x0B:
            case 0x0C:
                ;//sdoHandler(inputMsg);
                break;
            case 0x0E:
                ;//hbHandler(inputMsg);
                break;
            default:
                break;
        }
        inboundBox.free(inputMsg);
    }

}

void can_sender(){
    CANMessage* outputMsg;
    while(1){
        outputMsg = outboundBox.try_get_for(Kernel::wait_for_u32_forever);
        if(!outputMsg) printf("Mail read error! \n");
        can.write(*outputMsg);
        can.monitor(0);
        outboundBox.free(outputMsg);
    }
}

void can_irq(){
    dump.set(1);
}
