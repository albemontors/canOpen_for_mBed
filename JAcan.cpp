#include "JAcan.h"
#include <cstdint>

Mutex lock;
Ticker timer;
Thread dumper;
Thread receiver;
Thread sender;
EventFlags flag;
Mail<CANMessage, 16> inboundBox;
Mail<CANMessage, 16> outboundBox;
interface::CAN can(RD, TD, FREQUENCY);

PDO_Dictionary_Entry PDO_Dictionary[NODE_NUMBER];
Dictionary_Id Sync_Dictionary[8];

bool isMaster;
bool verbose = 0;
bool verbose_low_level = 0;

void can_setup(PDO_Dictionary_Entry* PDO_Dictionary_init, bool isMaster_init, Dictionary_Id* Sync_Dictionary_init){
// ====== PROGRAM INIT SECTION ========// nothing needs touching below here
    can.frequency(FREQUENCY); 
    can.attach(can_irq);
    //can.monitor(1);
    dumper.start(can_dumper);
    dumper.set_priority(osPriorityHigh2);
    receiver.start(can_sorter);
    receiver.set_priority(osPriorityHigh1);
    sender.start(can_sender);
    sender.set_priority(osPriorityHigh);
    isMaster = isMaster_init;
    for(int i = 0; i < NODE_NUMBER; i++) PDO_Dictionary[i] = PDO_Dictionary_init[i];
    for(int i = 0; i < SYNC_NUMBER; i++) Sync_Dictionary[i] = Sync_Dictionary_init[i];
    if(verbose_low_level) printf("(S) can_setup: Setup function have finished execution \n");
}

CANMessage* can_allocate(){
    return outboundBox.try_alloc();
}

void can_send(CANMessage* msg){
    outboundBox.put(msg);
}

void pdoHandler(CANMessage* inputMsg){

    CAN_Id id;
    id.raw = inputMsg->id;
    if(isMaster == (id.bd.code % 2)) { if(verbose_low_level) printf("(E) pdoHandler: Invalid PDO received \n"); return; }

    flag.set(SYNCdictionaryResolver(id));

    if(verbose_low_level) printf("(S) pdoHandler: Started PDO sorting routine \n");
    if(!inputMsg->type){
        Dictionary_Id dicId = PDOdictionaryResolver(id);
        if(dicId.deviceId != -1) {
            for(int i = 0; i < 8; i++) PDO_Dictionary[dicId.deviceId].pdo[dicId.PDOId].data->u8[i] = inputMsg->data[i];
            if(verbose_low_level) printf("(S) pdoHandler: Memory written from PDO data \n");
        } else {
            if(verbose_low_level) printf("(S) pdoHandler: PDO not recognized from dictionary \n");
        }
    } else {
        isMaster ? id.bd.code-- : id.bd.code++;
        if(verbose_low_level) printf("(S) pdoHandler: Responding an RTR request \n");
        pdoSender(id);
    }
    if(verbose_low_level) printf("(S) pdoHandler: Ended PDO sorting routine \n");

}

void can_dumper(){
    while(1){
        flag.wait_any(1);
        flag.clear(1);
        CANMessage* msg = inboundBox.try_alloc_for(Kernel::wait_for_u32_forever);
        if(msg) can.read(*msg);
        else if(verbose_low_level) printf("(!) can_dumper: Mail alloc error \n");
        if(verbose_low_level) printf("(S) can_dumper: A frame has been received from the bus \n");
        inboundBox.put(msg);
    }
}

void can_sorter(){
    // INIT
    CANMessage* inputMsg;
    CAN_Id id;

    // CODE
    while(1){
        inputMsg = inboundBox.try_get_for(Kernel::wait_for_u32_forever);
        if(!inputMsg) { printf("(!) can_sorter: Mail read error! \n"); continue; } //this should never happen, but better safe than sorry

        id.raw = inputMsg->id;

        if(verbose){      
            PDO_Data data;
            for(int i = 0; i < 8; i++) data.u8[i] = inputMsg->data[i];
            printf("(S) can_sorter: Received a frame from %d with code %d \n", id.bd.id, id.bd.code);
            printf("(S) can_sorter: data contained is %04X %04X %04X %04X \n",
                data.u16[0],
                data.u16[1],
                data.u16[2],
                data.u16[3]);
        }
        if(verbose_low_level) printf("(S) can_sorter: Sorting a frame \n");
        
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
        if(!outputMsg) if(verbose_low_level) printf("(!) can_sender: Mail read error! \n");
        can.mode(CAN::Normal);
        can.write(*outputMsg);
        //can.monitor(1);
        outboundBox.free(outputMsg);
        if(verbose) printf("(S) can_sender: A frame was sent on the bus \n");
    }
}

void can_irq(){
    flag.set(1);
}

void pdoSender(CAN_Id id){

    if(isMaster != (id.bd.code % 2)) { if(verbose) printf("(E) pdoSender: Invalid Request, no PDO sent \n"); return; }

    CANMessage* outputMsg = outboundBox.try_alloc();
    if(!outputMsg) if(verbose_low_level) { printf("(!) pdoSender: Mail read error, no PDO sent \n"); return; }
    Dictionary_Id dicId = PDOdictionaryResolver(id);
    if(dicId.deviceId != -1) {
        for(int i = 0; i < NODE_NUMBER; i++) outputMsg->data[i] = PDO_Dictionary[dicId.deviceId].pdo[dicId.PDOId].data->u8[i];
        if(verbose) printf("(S) pdoSender: Memory read from %d %d \n", dicId.deviceId, dicId.PDOId); 
        if(verbose) printf("(S) pdoSender: Memory read is %d %d %d %d %d %d %d %d \n", 
            outputMsg->data[0], 
            outputMsg->data[1], 
            outputMsg->data[2], 
            outputMsg->data[3], 
            outputMsg->data[4], 
            outputMsg->data[5], 
            outputMsg->data[6], 
            outputMsg->data[7]);
        if(verbose) printf("(S) pdoSender: PDO packed with data from the memory \n");
    } else {
        if(verbose) printf("(E) pdoSender: PDO not recognized from dictionary \n");
    }   
    outputMsg->id = id.raw;
    outputMsg->type = CANData;
    outputMsg->format = CANStandard;
    outputMsg->len = 8;
    if(verbose_low_level) printf("(S) pdoSender: A PDO has been routed \n");
    outboundBox.put(outputMsg);

}

void pdoRequest(CAN_Id id){
    
    if(!isMaster) id.bd.code++;
    if(isMaster != (id.bd.code % 2)) { if(verbose) printf("(E) pdoRequest: Invalid Request, no PDO sent \n"); return; }
    CANMessage* outputMsg = outboundBox.try_alloc();
    if(!outputMsg) if(verbose_low_level) { printf("(!) pdoRequest: Mail read error, no PDO sent \n"); return; }
    outputMsg->id = id.raw;
    outputMsg->type = CANRemote;
    outputMsg->format = CANStandard;
    outputMsg->len = 0;
    if(verbose_low_level) printf("(S) pdoRequest: A PDO has been routed \n");
    outboundBox.put(outputMsg);

}

void executeSync(){
    CANMessage* sync = can_allocate();
    CAN_Id id;
    id.bd.id = 0x00;    //put the correct node id
    id.bd.code = SYNC; //put the correct PDO id
    sync->id = id.raw;
    sync->type = CANData;           //set this to "CANRemote" to send an RTR packet
    sync->format = CANStandard;     //this sets the 11 bit address mode
    sync->len = 0;
    can_send(sync);
    // wait for all flags from pdos to come back
    //flag.wait_any_for(pow(SYNC_NUMBER + 1, 2)-2, (Kernel::Clock::duration_u32) 100ms, true);
    //flag.clear(pow(SYNC_NUMBER + 1, 2)-2);
    //flag.wait_all(pow(SYNC_NUMBER + 1, 2)-2);
}

Dictionary_Id PDOdictionaryResolver(CAN_Id id){
    Dictionary_Id return_value;
    for(int i = 0; i < NODE_NUMBER; i++){
        if(PDO_Dictionary[i].id == id.bd.id){
            for(int j = 0; j < 8; j++){
                if(PDO_Dictionary[i].pdo[j].code == id.bd.code){
                    return_value.deviceId = i;
                    return_value.PDOId = j;
                    return return_value;
                }
            }
            break;
        }
    }
    return_value.deviceId = -1;
    return_value.PDOId = -1;
    return return_value;
}

uint32_t SYNCdictionaryResolver(CAN_Id id){
    for(int i = 0; i < SYNC_NUMBER; i++){
        if(Sync_Dictionary[i].deviceId == id.bd.id){
            if(Sync_Dictionary[i].PDOId == id.bd.code){
                    return pow(i+1, 2);
            }
        }
    }
    return 0;
}