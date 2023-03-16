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

bool verbose = 1;

void can_setup(PDO_Dictionary_Entry* PDO_Dictionary_init){
// ====== PROGRAM INIT SECTION ========// nothing needs touching below here
    can.attach(can_irq);
    can.monitor(0);
    dumper.start(can_dumper);
    receiver.start(can_sorter);
    sender.start(can_sender);
    for(int i = 0; i < NODE_NUMBER; i++) PDO_Dictionary[i] = PDO_Dictionary_init[i];
    if(verbose) printf("(S) can_setup: Setup function have finished execution \n");
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
    if(!(isMaster xor (id.bd.code % 2))) { if(verbose) printf("(!) pdoHandler: Invalid PDO received \n"); return; }

    if(verbose) printf("(S) pdoHandler: Started PDO sorting routine \n");
    PDO_Data data;
    if(!inputMsg->type){
        for(int i = 0; i < 8; i++) data.u8[i] = inputMsg->data[i];
        for(int i = 0; i < NODE_NUMBER; i++) // check the IDs
            if(id.bd.id == PDO_Dictionary[i].id)
                for(int j = 0; j < 8; j++){ // check the PDOs
                    if(id.bd.code == PDO_Dictionary[i].pdo[j].code){
                        PDO_Dictionary[i].pdo[j].data->u64 = data.u64;
                        if(verbose) printf("(S) pdoHandler: Memory has been updated with PDO data \n");
                        break;
                    }
                break;
                }
    } else {
        isMaster ? id.bd.code-- : id.bd.code++;
        if(verbose) printf("(S) pdoHandler: Responding an RTR request \n");
        pdoSender(id);
    }
    if(verbose) printf("(S) pdoHandler: Ended PDO sorting routine \n");

}

void can_dumper(){
    while(1){
        dump.wait_any(1);
        dump.clear(1);
        CANMessage* msg = inboundBox.try_alloc_for(Kernel::wait_for_u32_forever);
        if(msg) can.read(*msg);
        else if(verbose) printf("(!) can_dumper: Mail alloc error \n");
        if(verbose) printf("(S) can_sender: A frame has been received from the bus \n");
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
        if(!inputMsg) printf("(!) can_sorter: Mail read error! \n");

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
        if(!outputMsg) if(verbose) printf("(!) can_sender: Mail read error! \n");
        can.write(*outputMsg);
        can.monitor(0);
        outboundBox.free(outputMsg);
        if(verbose) printf("(S) can_sender: A frame was sent on the bus \n");
    }
}

void can_irq(){
    dump.set(1);
}

void pdoSender(CAN_Id id){

    if(!(isMaster xor (id.bd.code % 2))) { if(verbose) printf("(!) pdoSender: Invalid Request, no PDO sent \n"); return; }

    CANMessage* outputMsg = outboundBox.try_alloc();
    if(!outputMsg) if(verbose) { printf("(!) pdoSender: Mail read error, no PDO sent \n"); return; }
    for(int i = 0; i < 4; i++)
        if(id.bd.id == PDO_Dictionary[i].id)
            for(int j = 0; j < 4; j++)
                if(id.bd.code == PDO_Dictionary[i].pdo[j].code)
                    for(int k = 0; k < NODE_NUMBER; k++) outputMsg->data[i] = PDO_Dictionary[i].pdo[j].data->u8[k];
    outputMsg->id = id.raw;
    outputMsg->type = CANData;
    outputMsg->format = CANStandard;
    outboundBox.put(outputMsg);
    if(verbose) printf("(!) pdoSender: A PDO has been routed \n");

}

void pdoRequest(CAN_Id id){
    
    if(!(isMaster xor (id.bd.code % 2))) { if(verbose) printf("(!) pdoRequest: Invalid Request, no PDO sent \n"); return; }
    CANMessage* outputMsg = outboundBox.try_alloc();
    if(!outputMsg) if(verbose) { printf("(!) pdoRequest: Mail read error, no PDO sent \n"); return; }
    outputMsg->id = id.raw;
    outputMsg->type = CANRemote;
    outputMsg->format = CANStandard;
    outputMsg->len = 0;
    outboundBox.put(outputMsg);
    if(verbose) printf("(!) pdoRequest: A PDO has been routed \n");

}